// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/Preview/PreviewUnit.h"
#include "Core/NovaPart.h"
#include "Core/NovaObjectPoolSubsystem.h"
#include "Kismet/GameplayStatics.h"

APreviewUnit::APreviewUnit()
{
    PrimaryActorTick.bCanEverTick = false;
    
    // 유닛의 중심점이 될 루트 컴포넌트 생성
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewRoot"));

    // 기본 무기 소켓 이름 설정 (몸통 부품의 스켈레탈 메시 소켓 이름과 일치해야 함)
    WeaponSocketNames.Empty();
    WeaponSocketNames.Add(TEXT("Socket_Weapon_L"));
    WeaponSocketNames.Add(TEXT("Socket_Weapon_R"));
}

void APreviewUnit::ApplyAssemblyData(const FNovaUnitAssemblyData& Data)
{
    // 1. 각 부위별로 클래스가 변경되었을 때만 새로 생성하거나 교체 (풀링 활용)
    UpdatePart(Data.LegsClass, CurrentLegs);
    UpdatePart(Data.BodyClass, CurrentBody);

    // 무기는 리스트 구조이므로 별도 처리 (단일 무기 클래스를 설정된 소켓 개수만큼 복제 생성)
    if (Data.WeaponClass)
    {
        // 소켓 개수만큼 무기 참조 슬롯 확보
        while (CurrentWeapons.Num() < WeaponSocketNames.Num())
        {
            CurrentWeapons.Add(nullptr);
        }

        for (int32 i = 0; i < WeaponSocketNames.Num(); ++i)
        {
            UpdatePart(Data.WeaponClass, CurrentWeapons[i]);
        }
    }

    // 2. [핵심] 부품들 간의 계층 구조 및 소켓 부착 관계 재정립
    // 이 함수가 다리 -> 몸통 -> 무기 순으로 부품들을 체인처럼 연결합니다.
    RefreshAttachments();
}

ANovaPart* APreviewUnit::UpdatePart(TSubclassOf<ANovaPart> NewPartClass, TObjectPtr<ANovaPart>& CurrentPart)
{
    UNovaObjectPoolSubsystem* Pool = GetWorld()->GetSubsystem<UNovaObjectPoolSubsystem>();
    if (!Pool || !NewPartClass) return nullptr;

    // 현재 장착된 부품과 새로 요청된 클래스가 다를 때만 실제 교체 작업 수행
    if (!CurrentPart || CurrentPart->GetClass() != NewPartClass)
    {
        // 기존 부품이 있다면 풀로 반환하여 메모리 해제 방지
        if (CurrentPart)
        {
            Pool->ReturnToPool(CurrentPart);
        }

        // 새로운 클래스의 부품을 풀에서 가져오거나 생성
        CurrentPart = Cast<ANovaPart>(Pool->SpawnFromPool(NewPartClass, GetActorTransform()));
        
        if (CurrentPart)
        {
            if (ANovaPart* DefaultPart = NewPartClass.GetDefaultObject())
            {
                CurrentPart->SetPartID(DefaultPart->GetPartID());
            }
            
            // 부품에 필요한 기본 데이터 및 소유자 설정
            CurrentPart->SetPartDataTable(PartDataTable);
            CurrentPart->InitializePartSpec();
            CurrentPart->SetOwner(this);
        }
        
        // // --- 로비 전용 비주얼 보정 로직 ---
        // // 인게임에서는 작게(0.25배) 표현되는 다리 파츠를 로비 프리뷰에서는 크게 보여주기 위한 배율 조정입니다.
        // if (CurrentPart->GetPartSpec().PartType == ENovaPartType::Legs)
        // {
        //     // 다리 파츠만 4배로 확대 (인게임 0.25 * 4.0 = 실질적 1.0 배율 효과)
        //     CurrentPart->SetActorScale3D(FVector(4.0f));
        // }
        // else
        // {
        //     // 나머지 몸통 및 무기 파츠는 표준 배율(1.0) 유지
        //     CurrentPart->SetActorScale3D(FVector(1.0f));
        // }
    }
    
    return CurrentPart;
}

void APreviewUnit::RefreshAttachments()
{
    // 1. 다리(Legs) -> 유닛 루트에 부착 및 오프셋 적용
    if (CurrentLegs)
    {
        CurrentLegs->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
        
        // 2. 부착이 완료된 후, 파츠 타입이 "다리(Legs)"인 경우에만 로비 전용 배율인 4배를 다시 적용합니다.
        // UpdatePart에서 주입된 스펙 정보를 바탕으로 타입을 정확히 판별합니다.
        if (CurrentLegs->GetPartSpec().PartType == ENovaPartType::Legs)
        {
            CurrentLegs->SetActorScale3D(FVector(4.0f));
        }

        // [인게임 로직 복제 1] 다리는 유닛 중심에서 바닥으로 내려야 하므로 Z축 -90 오프셋을 적용합니다.
        // ANovaUnit의 LegsOffset 설정값과 동일하게 맞추어 로비-인게임 간 이질감을 없앱니다.
        CurrentLegs->SetActorRelativeLocation(FVector(0.f, 0.f, -90.f));
        CurrentLegs->SetActorRelativeRotation(FRotator(0.f, -90.f, 0.f));

        if (UPrimitiveComponent* LegsMesh = CurrentLegs->GetMainMesh())
        {
            // 부모의 좌표를 즉시 월드에 갱신해야, 자식(몸통)이 올바른 다리 소켓 위치를 참조할 수 있습니다.
            LegsMesh->UpdateComponentToWorld();

            // 2. 몸통(Body) -> 다리의 소켓에 부착
            if (CurrentBody)
            {
                CurrentBody->AttachToComponent(LegsMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, BodySocketName);

                if (UPrimitiveComponent* BodyMesh = CurrentBody->GetMainMesh())
                {
                    // [인게임 로직 복제 2] 핵심!! 액터 부착 후 액터 내부의 메쉬 컴포넌트를 소켓의 (0,0,0) 위치로 강제 고정합니다.
                    // 이 작업을 생략하면 부품 스폰 시의 미세한 좌표 오차가 조립된 유닛에 그대로 남게 됩니다.
                    BodyMesh->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
                    BodyMesh->UpdateComponentToWorld();

                    // 3. 무기(Weapon) -> 몸통 소켓들에 부착
                    for (int32 i = 0; i < CurrentWeapons.Num(); ++i)
                    {
                        if (CurrentWeapons[i] && WeaponSocketNames.IsValidIndex(i))
                        {
                            CurrentWeapons[i]->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponSocketNames[i]);

                            if (UPrimitiveComponent* WeaponMesh = CurrentWeapons[i]->GetMainMesh())
                            {
                                // [인게임 로직 복제 3] 무기 메쉬도 몸통 소켓 위치에 딱 붙도록 로컬 트랜스폼을 리셋합니다.
                                WeaponMesh->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
                            }
                        }
                    }
                }
            }
        }
    }
}

void APreviewUnit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ReturnPartsToPool();
    
    Super::EndPlay(EndPlayReason);
}

void APreviewUnit::ReturnPartsToPool()
{
    // 풀링 서브시스템 참조 획득
    UNovaObjectPoolSubsystem* Pool = GetWorld()->GetSubsystem<UNovaObjectPoolSubsystem>();
    if (!Pool) return;

    // 1. 다리(Legs) 반환
    if (CurrentLegs) 
    {
        // 반납 전 스케일 리셋 로직은 ReturnToPool 내부 혹은 직전에 수행됨을 전제로 합니다.
        Pool->ReturnToPool(CurrentLegs);
        CurrentLegs = nullptr;
    }

    // 2. 몸통(Body) 반환
    if (CurrentBody) 
    {
        Pool->ReturnToPool(CurrentBody);
        CurrentBody = nullptr;
    }

    // 3. 무기(Weapons) 리스트 반환
    for (ANovaPart* Weapon : CurrentWeapons) 
    {
        if (Weapon) 
        {
            Pool->ReturnToPool(Weapon);
        }
    }
    CurrentWeapons.Empty(); // 리스트 비우기
}

