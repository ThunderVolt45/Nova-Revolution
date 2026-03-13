// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Targeting/NovaTargetActor_GroundRadius.h"
#include "Abilities/GameplayAbility.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h" // 추가: APlayerState를 사용하기 위해 필수
#include "Core/NovaInterfaces.h" // TeamID 확인을 위한 인터페이스
#include "Core/NovaTypes.h" // 추가: NovaTeam::None 사용을 위해 필수
#include "Engine/OverlapResult.h"
#include "Engine/World.h"


ANovaTargetActor_GroundRadius::ANovaTargetActor_GroundRadius()
{
    // 마우스 추적을 위해 틱 활성화
    PrimaryActorTick.bCanEverTick = true;
    // Confirm(클릭) 시 액터를 바로 파괴할지 여부 (보통 Task에서 관리하므로 기본값 유지)
    bDestroyOnConfirmation = false;
}

void ANovaTargetActor_GroundRadius::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // 매 프레임 마우스 위치를 계산하여 액터의 위치를 업데이트합니다.
    FVector MouseLoc = GetMouseLocationOnGround();
    SetActorLocation(MouseLoc);
}

void ANovaTargetActor_GroundRadius::ConfirmTargetingAndContinue()
{
    // 추가: PrimaryPC가 없으면 타겟팅을 수행할 수 없음
    if (!PrimaryPC)
    {
        CancelTargeting();
        return;
    }
    
    // 1. 현재 마우스 위치에서 범위 내 유닛 검색 (Overlap 쿼리)
    TArray<FOverlapResult> Overlaps;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // 자기 자신은 제외
    
    // 추가: PrimaryPC가 조종하는 Pawn도 검색에서 제외할지 결정 (기지 소환의 경우 필요할 수 있음) -> 제외해야함, CameraPawn은 검색에서 제외
    if (PrimaryPC->GetPawn()) 
    {
        Params.AddIgnoredActor(PrimaryPC->GetPawn());
    }

    // Pawn 채널을 대상으로 구체 범위 내 모든 액터 검색
    GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn, Sphere, Params);

    // 2. 내 팀 ID 확인
    int32 MyTeamID = NovaTeam::None;
    if (PrimaryPC)
    {
        // 방법 A: PlayerState가 인터페이스를 구현하고 있으므로 PS를 먼저 확인
        if (INovaTeamInterface* TeamInterface = PrimaryPC->GetPlayerState<INovaTeamInterface>())
        {
            MyTeamID = TeamInterface->GetTeamID();
        }
        // 방법 B (Fallback): 만약 Controller에도 구현해두었다면 직접 확인
        else if (INovaTeamInterface* PCTeamInterface = Cast<INovaTeamInterface>(PrimaryPC))
        {
            MyTeamID = PCTeamInterface->GetTeamID();
        }
    }

    // 3. 필터링 로직 수행 및 타겟 리스트 구성
    TArray<TWeakObjectPtr<AActor>> FilteredActors;
    for (const FOverlapResult& Result : Overlaps)
    {
        AActor* OverlappedActor = Result.GetActor();
        if (INovaTeamInterface* UnitTeam = Cast<INovaTeamInterface>(OverlappedActor))
        {
            // 팀 ID를 비교하여 타겟에 포함할지 결정
            if (IsValidTarget(MyTeamID, UnitTeam->GetTeamID()))
            {
                FilteredActors.Add(OverlappedActor);
            }
        }
    }

    // 4. 결과가 있다면 데이터를 Gameplay Ability로 전송
    if (FilteredActors.Num() > 0)
    {
        // 유닛 리스트를 포함한 TargetDataHandle 생성
        FGameplayAbilityTargetDataHandle Handle = StartLocation.MakeTargetDataHandleFromActors(FilteredActors);

        // 델리게이트를 통해 GA에게 데이터가 준비되었음을 알림
        TargetDataReadyDelegate.Broadcast(Handle);
    }
    else
    {
        // 대상이 하나도 없으면 타겟팅 취소 처리
        CancelTargeting();
    }
}

bool ANovaTargetActor_GroundRadius::IsValidTarget(int32 MyTeamID, int32 TargetTeamID) const
{
    // NovaTeam 네임스페이스와 비교하여 필터링 수행
    switch (TargetingFilter)
    {
    case ENovaTargetingFilter::Ally:
        return MyTeamID == TargetTeamID;

    case ENovaTargetingFilter::Enemy:
        // 내 팀이 아니고, 중립/없음 상태가 아닌 경우 적군으로 간주
        return (MyTeamID != TargetTeamID) && (TargetTeamID != NovaTeam::None);

    case ENovaTargetingFilter::Both:
        return true;
    }

    return false;
}

FVector ANovaTargetActor_GroundRadius::GetMouseLocationOnGround() const
{
    if (PrimaryPC)
    {
        FHitResult Hit;
        // Visibility 채널을 사용하여 지면(Terrain) 위치를 감지
        if (PrimaryPC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
        {
            return Hit.ImpactPoint;
        }
    }
    return FVector::ZeroVector;
}

