// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaBTTask_Spread.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Core/NovaUnit.h"
#include "Core/NovaTypes.h"
#include "AbilitySystemComponent.h"
#include "GAS/NovaAttributeSet.h"
#include "NovaRevolution.h"

UNovaBTTask_Spread::UNovaBTTask_Spread()
{
	NodeName = TEXT("Nova Spread Task");
	bNotifyTick = true;

	// 블랙보드 키 필터링
	TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Spread, TargetLocationKey));
	CommandTypeKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Spread, CommandTypeKey), StaticEnum<ECommandType>());
}

EBTNodeResult::Type UNovaBTTask_Spread::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !AIC->GetPawn() || !BB) return EBTNodeResult::Failed;

	ANovaUnit* MyUnit = Cast<ANovaUnit>(AIC->GetPawn());
	if (!MyUnit) return EBTNodeResult::Failed;

	// 1. 산개 중심점(명령 지점) 읽기
	FVector SpreadOrigin = BB->GetValueAsVector(TargetLocationKey.SelectedKeyName);
	FVector CurrentPos = MyUnit->GetActorLocation();

	// 2. 산개 거리 결정 (Max(MinRange, DefaultMinSpreadDistance))
	float MinRange = 0.0f;
	if (UAbilitySystemComponent* ASC = MyUnit->GetAbilitySystemComponent())
	{
		MinRange = ASC->GetNumericAttribute(UNovaAttributeSet::GetMinRangeAttribute());
	}
	float FinalSpreadDistance = FMath::Max(MinRange, DefaultMinSpreadDistance);

	// 3. 산개 방향 결정 (중심점으로부터 멀어지는 방향)
	FVector SpreadDir = CurrentPos - SpreadOrigin;
	SpreadDir.Z = 0.0f;

	if (SpreadDir.IsNearlyZero())
	{
		// 유닛이 중심점에 정확히 있다면 무작위 방향 선택
		SpreadDir = FMath::VRand();
		SpreadDir.Z = 0.0f;
	}
	SpreadDir.Normalize();

	// 4. 최종 이동 지점 계산
	FVector MoveTarget = SpreadOrigin + (SpreadDir * FinalSpreadDistance);

	// 5. 네비메쉬 투영
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		FNavLocation ProjectedLocation;
		if (NavSys->ProjectPointToNavigation(MoveTarget, ProjectedLocation, FVector(500.f, 500.f, 500.f)))
		{
			MoveTarget = ProjectedLocation.Location;
		}
	}

	AIC->MoveToLocation(MoveTarget, AcceptanceRadius);
	
	NOVA_LOG(Log, "Unit %s SPREADING from command point. Distance: %.1f", *MyUnit->GetName(), FinalSpreadDistance);

	return EBTNodeResult::InProgress;
}

void UNovaBTTask_Spread::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !AIC->GetPathFollowingComponent() || !BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (AIC->GetPathFollowingComponent()->GetStatus() == EPathFollowingStatus::Idle)
	{
		// 산개 이동 완료 후 상태를 Idle(None)으로 전환
		BB->SetValueAsEnum(CommandTypeKey.SelectedKeyName, (uint8)ECommandType::None);
		BB->ClearValue(TargetLocationKey.SelectedKeyName); // 위치 정보도 정리

		NOVA_LOG(Log, "Unit %s finished SPREAD. Returning to IDLE.", *AIC->GetPawn()->GetName());
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
