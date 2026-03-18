// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/Preview/PreviewUnit.h"

#include "NovaRevolution.h"
#include "Core/NovaPart.h"
#include "Core/NovaPartData.h"
#include "Components/SceneComponent.h"
#include "Core/NovaLog.h"

APreviewUnit::APreviewUnit()
{
    // 더미 액터이므로 틱을 꺼서 성능을 최적화합니다.
    PrimaryActorTick.bCanEverTick = false;

    // 유닛의 기준점이 될 루트 컴포넌트 생성
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // 기본 무기 소켓 이름 (에디터 디테일 패널에서 수정 가능)
    WeaponSocketNames.Add(TEXT("WeaponSocket_L"));
    WeaponSocketNames.Add(TEXT("WeaponSocket_R"));
}

void APreviewUnit::ApplyAssemblyData(const FNovaUnitAssemblyData& Data)
{
    // 1. 기존 부품들을 완전히 제거하여 겹침 현상 방지
    ClearParts();

    // 2. 다리(Legs) 생성 및 루트에 부착
    CurrentLegs = SpawnPart(Data.LegsClass);
    if (CurrentLegs)
    {
        CurrentLegs->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
        // 에셋 방향 보정 (언리얼 전방 벡터 기준 -90도 회전이 필요한 경우가 많습니다)
        CurrentLegs->SetActorRelativeRotation(FRotator(0.f, -90.f, 0.f));
    }

    // 3. 몸통(Body) 생성 및 다리에 부착
    if (CurrentLegs && Data.BodyClass)
    {
        CurrentBody = SpawnPart(Data.BodyClass);
        if (CurrentBody)
        {
            if (UPrimitiveComponent* LegsMesh = CurrentLegs->GetMainMesh())
            {
                CurrentBody->AttachToComponent(LegsMesh, 
                    FAttachmentTransformRules::SnapToTargetIncludingScale, BodySocketName);
            }
        }
    }

    // 4. 무기(Weapon) 생성 및 몸통 소켓들에 부착
    if (CurrentBody && Data.WeaponClass)
    {
        if (UPrimitiveComponent* BodyMesh = CurrentBody->GetMainMesh())
        {
            for (const FName& SocketName : WeaponSocketNames)
            {
                if (BodyMesh->DoesSocketExist(SocketName))
                {
                    ANovaPart* NewWeapon = SpawnPart(Data.WeaponClass);
                    if (NewWeapon)
                    {
                        NewWeapon->AttachToComponent(BodyMesh, 
                            FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
                        CurrentWeapons.Add(NewWeapon);
                    }
                }
            }
        }
    }

    NOVA_LOG(Log, "Preview Unit updated with Dummy logic: %s", *Data.UnitName);
}

void APreviewUnit::ClearParts()
{
    if (CurrentLegs) { CurrentLegs->Destroy(); CurrentLegs = nullptr; }
    if (CurrentBody) { CurrentBody->Destroy(); CurrentBody = nullptr; }

    for (ANovaPart* Weapon : CurrentWeapons)
    {
        if (Weapon) Weapon->Destroy();
    }
    CurrentWeapons.Empty();
}

ANovaPart* APreviewUnit::SpawnPart(TSubclassOf<ANovaPart> PartClass)
{
    if (!PartClass) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // 더미용이므로 오브젝트 풀링 없이 즉각적인 스폰/파괴 방식을 사용합니다.
    ANovaPart* NewPart = GetWorld()->SpawnActor<ANovaPart>(PartClass, Params);
    if (NewPart)
    {
        NewPart->SetOwner(this);
        // 부품 정보 조회를 위한 데이터 테이블 설정
        NewPart->SetPartDataTable(PartSpecDataTable);
    }

    return NewPart;
}

