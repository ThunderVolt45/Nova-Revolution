// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaBTTask_MoveToLocation.h"

#include "NavigationSystem.h"
#include "NovaRevolution.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/AI/NovaAIController.h"
#include "Core/NovaUnit.h"
#include "Core/NovaTypes.h"

UNovaBTTask_MoveToLocation::UNovaBTTask_MoveToLocation()
{
	NodeName = TEXT("Nova Move To Location");
	bNotifyTick = true;

	// 블랙보드 키 필터링
	TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_MoveToLocation, TargetLocationKey));
	CommandTypeKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_MoveToLocation, CommandTypeKey), StaticEnum<ECommandType>());
}

EBTNodeResult::Type UNovaBTTask_MoveToLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ANovaAIController* AIC = Cast<ANovaAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn) return EBTNodeResult::Failed;

	ANovaUnit* MyUnit = Cast<ANovaUnit>(Pawn);
	ENovaMovementType MoveType = MyUnit ? MyUnit->GetMovementType() : ENovaMovementType::Ground;

	// 목표 지점 읽기
	FVector GoalLocation = OwnerComp.GetBlackboardComponent()->GetValueAsVector(TargetLocationKey.SelectedKeyName);

	// [수정] 지상 유닛인 경우 도달 불가능한 위치 보정
	if (MoveType == ENovaMovementType::Ground)
	{
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if (NavSys)
		{
			FNavLocation ProjectedLocation;
			// 수평 1000, 수직 2000 범위 내에서 가장 가까운 유효한 NavMesh 지점을 탐색합니다.
			if (NavSys->ProjectPointToNavigation(GoalLocation, ProjectedLocation, FVector(1000.f, 1000.f, 2000.f)))
			{
				GoalLocation = ProjectedLocation.Location;
			}
			else
			{
				// 투영에 실패했다는 것은 근처에 NavMesh가 아예 없다는 의미일 수 있음
				// 이때는 태스크를 실패시키기보다 최소한 현재 위치에서 그 방향으로 조금이라도 시도하도록 GoalLocation 유지
				// (엔진의 AllowPartialPath가 나머지를 처리)
			}
		}
	}

	// 컨트롤러 통합 이동 함수 호출 (지상/공중 자동 분기)
	AIC->MoveToLocationOptimized(GoalLocation, AcceptanceRadius);

	return EBTNodeResult::InProgress;
}

void UNovaBTTask_MoveToLocation::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	ANovaAIController* AIC = Cast<ANovaAIController>(OwnerComp.GetAIOwner());
	if (!AIC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 이동이 완료되었는지 확인 (수동 이동 포함)
	if (!AIC->IsMoveInProgress())
	{
		if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
		{
			BB->SetValueAsEnum(CommandTypeKey.SelectedKeyName, (uint8)ECommandType::None);
			BB->ClearValue(TargetLocationKey.SelectedKeyName);
		}
		
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		NOVA_LOG(Warning, "FinishLatentTask MoveToLocation");
	}
}
