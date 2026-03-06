// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaBTTask_Patrol.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "NovaRevolution.h"
#include "Navigation/PathFollowingComponent.h"

UNovaBTTask_Patrol::UNovaBTTask_Patrol()
{
	NodeName = TEXT("Nova Patrol Task");
	bNotifyTick = true;

	// 블랙보드 키 필터링 (Vector 타입만 선택 가능하게 함)
	TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Patrol, TargetLocationKey));
	PatrolOriginKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Patrol, PatrolOriginKey));
}

EBTNodeResult::Type UNovaBTTask_Patrol::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !BB || !AIC->GetPawn()) return EBTNodeResult::Failed;

	// 순찰 시작 지점(Origin)을 현재 위치로 초기화 (순찰 명령이 시작될 때마다 갱신)
	FVector CurrentLocation = AIC->GetPawn()->GetActorLocation();
	BB->SetValueAsVector(PatrolOriginKey.SelectedKeyName, CurrentLocation);

	bMovingToOrigin = false; // 처음에는 입력받은 TargetLocation으로 이동
	
	FVector Goal = BB->GetValueAsVector(TargetLocationKey.SelectedKeyName);
	AIC->MoveToLocation(Goal, AcceptanceRadius);

	return EBTNodeResult::InProgress;
}

void UNovaBTTask_Patrol::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !BB || !AIC->GetPathFollowingComponent())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (AIC->GetPathFollowingComponent()->GetStatus() == EPathFollowingStatus::Idle)
	{
		if (AIC->GetPathFollowingComponent()->DidMoveReachGoal())
		{
			// 목표 지점 도착 시 목표 반전
			bMovingToOrigin = !bMovingToOrigin;
			
			FVector NextGoal = bMovingToOrigin ? 
				BB->GetValueAsVector(PatrolOriginKey.SelectedKeyName) : 
				BB->GetValueAsVector(TargetLocationKey.SelectedKeyName);
			
			AIC->MoveToLocation(NextGoal, AcceptanceRadius);
			
			NOVA_LOG(Log, "Patrol: Reached goal, switching to %s", bMovingToOrigin ? TEXT("Origin") : TEXT("Target"));
		}
		else
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		}
	}
}
