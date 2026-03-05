// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaBTTask_MoveToLocation.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "NovaRevolution.h"
#include "Core/NovaUnit.h"
#include "Core/NovaTypes.h"
#include "Navigation/PathFollowingComponent.h"

UNovaBTTask_MoveToLocation::UNovaBTTask_MoveToLocation()
{
	NodeName = TEXT("Nova Move To Location");
	bNotifyTick = true; // 이동 상태 감시를 위해 틱 활성화

	// 블랙보드 키 필터링 (Vector 타입만 선택 가능하게 함)
	TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_MoveToLocation, TargetLocationKey));
	CommandTypeKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_MoveToLocation, CommandTypeKey), StaticEnum<ECommandType>());
}

EBTNodeResult::Type UNovaBTTask_MoveToLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn) return EBTNodeResult::Failed;

	// 목표 지점 읽기
	FVector RawTargetLocation = OwnerComp.GetBlackboardComponent()->GetValueAsVector(TargetLocationKey.SelectedKeyName);
	FVector FinalGoal = RawTargetLocation;

	// 1. 네비메쉬 위로 투영 (기존 ANovaUnit::MoveToLocation의 로직 흡수)
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		FNavLocation ProjectedLocation;
		if (NavSys->ProjectPointToNavigation(RawTargetLocation, ProjectedLocation, FVector(1000.f, 1000.f, 1000.f)))
		{
			FinalGoal = ProjectedLocation.Location;
		}
	}

	// 2. 이동 요청 설정
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(FinalGoal);
	MoveRequest.SetAcceptanceRadius(AcceptanceRadius);
	MoveRequest.SetAllowPartialPath(true);
	MoveRequest.SetProjectGoalLocation(true);
	MoveRequest.SetRequireNavigableEndLocation(false);

	FPathFollowingRequestResult Result = AIC->MoveTo(MoveRequest);

	// 결과 처리
	switch (Result.Code)
	{
	case EPathFollowingRequestResult::Failed:
		NOVA_LOG(Warning, "MoveToTask Failed! Unit: %s", *Pawn->GetName());
		return EBTNodeResult::Failed;

	case EPathFollowingRequestResult::AlreadyAtGoal:
		// 이미 도착한 경우 즉시 상태 초기화
		if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
		{
			BB->SetValueAsEnum(CommandTypeKey.SelectedKeyName, (uint8)ECommandType::None);
			BB->ClearValue(TargetLocationKey.SelectedKeyName);
		}
		return EBTNodeResult::Succeeded;

	case EPathFollowingRequestResult::RequestSuccessful:
		return EBTNodeResult::InProgress; // 틱에서 완료 여부 체크
	}

	return EBTNodeResult::Failed;
}

void UNovaBTTask_MoveToLocation::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC || !AIC->GetPathFollowingComponent())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 이동이 완료되었는지 확인
	EPathFollowingStatus::Type MoveStatus = AIC->GetPathFollowingComponent()->GetStatus();
	
	if (MoveStatus == EPathFollowingStatus::Idle)
	{
		// 경로 추종이 멈춘 경우 (도착 또는 중단)
		if (AIC->GetPathFollowingComponent()->DidMoveReachGoal())
		{
			// 이동 완료 후 상태 초기화
			if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
			{
				BB->SetValueAsEnum(CommandTypeKey.SelectedKeyName, (uint8)ECommandType::None);
				BB->ClearValue(TargetLocationKey.SelectedKeyName);
				NOVA_LOG(Log, "Unit %s reached destination, transitioning to Idle.", *AIC->GetPawn()->GetName());
			}
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		else
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		}
	}
}
