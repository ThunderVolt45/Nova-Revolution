// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaAIController.h"
#include "VisualLogger/VisualLogger.h"
#include "Navigation/PathFollowingComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "NovaRevolution.h"
#include "Core/NovaUnit.h"

const FName ANovaAIController::TargetLocationKey(TEXT("TargetLocation"));
const FName ANovaAIController::TargetActorKey(TEXT("TargetActor"));
const FName ANovaAIController::CommandTypeKey(TEXT("CommandType"));

ANovaAIController::ANovaAIController()
{
	bSetControlRotationFromPawnOrientation = true;

	// 컴포넌트 초기화
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
}

void ANovaAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 비헤이비어 트리 실행
	if (BehaviorTreeAsset && BehaviorTreeAsset->BlackboardAsset)
	{
		InitializeBlackboard(*BlackboardComponent, *BehaviorTreeAsset->BlackboardAsset);
		RunBehaviorTree(BehaviorTreeAsset);
	}
	else
	{
		NOVA_LOG(Warning, "AIController: BehaviorTreeAsset is not assigned in %s!", *GetName());
	}
}

void ANovaAIController::IssueCommand(const FCommandData& CommandData)
{
	// 블랙보드 데이터 갱신
	if (BlackboardComponent && BlackboardComponent->GetBlackboardAsset())
	{
		BlackboardComponent->SetValueAsEnum(CommandTypeKey, (uint8)CommandData.CommandType);
	}

	switch (CommandData.CommandType)
	{
	case ECommandType::Move:
		HandleMoveCommand(CommandData.TargetLocation);
		break;

	case ECommandType::Attack:
		HandleAttackCommand(CommandData.TargetActor);
		break;

	case ECommandType::Stop:
	case ECommandType::Hold:
		HandleStopCommand();
		break;

	default:
		break;
	}
}

void ANovaAIController::HandleMoveCommand(const FVector& TargetLocation)
{
	// 블랙보드 데이터 갱신
	if (BlackboardComponent && BlackboardComponent->GetBlackboardAsset())
	{
		BlackboardComponent->SetValueAsVector(TargetLocationKey, TargetLocation);
		BlackboardComponent->ClearValue(TargetActorKey);
	}

	ANovaUnit* NovaUnit = Cast<ANovaUnit>(GetPawn());
	check(NovaUnit); 

	// 기존 직접 호출 로직 유지 (BT 태스크가 완성되면 이 로직은 BT 내부로 이동하게 됨)
	NovaUnit->MoveToLocation(TargetLocation);
	NOVA_LOG(Log, "AIController: Move command (BT-Synced) to %s", *TargetLocation.ToString());
}

void ANovaAIController::HandleAttackCommand(AActor* TargetActor)
{
	if (!TargetActor) return;

	// 블랙보드 데이터 갱신
	if (BlackboardComponent && BlackboardComponent->GetBlackboardAsset())
	{
		BlackboardComponent->SetValueAsObject(TargetActorKey, TargetActor);
		BlackboardComponent->SetValueAsVector(TargetLocationKey, TargetActor->GetActorLocation());
	}

	ANovaUnit* NovaUnit = Cast<ANovaUnit>(GetPawn());
	check(NovaUnit);

	// 기존 직접 호출 로직 유지
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalActor(TargetActor);
	MoveRequest.SetAcceptanceRadius(100.0f);
	MoveRequest.SetAllowPartialPath(true);
	MoveRequest.SetProjectGoalLocation(true);
	MoveRequest.SetRequireNavigableEndLocation(false);

	MoveTo(MoveRequest);
	
	NOVA_LOG(Log, "AIController: Attack command (BT-Synced) on target %s", *TargetActor->GetName());
}

void ANovaAIController::HandleStopCommand()
{
	// 블랙보드 초기화
	if (BlackboardComponent && BlackboardComponent->GetBlackboardAsset())
	{
		BlackboardComponent->ClearValue(TargetLocationKey);
		BlackboardComponent->ClearValue(TargetActorKey);
	}

	StopMovement();
	NOVA_LOG(Log, "AIController: Stop command (BT-Synced).");
}
