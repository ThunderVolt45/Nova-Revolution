// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaAIController.h"
#include "VisualLogger/VisualLogger.h"
#include "Navigation/PathFollowingComponent.h"
#include "NovaRevolution.h"
#include "Core/NovaUnit.h"

ANovaAIController::ANovaAIController()
{
	bSetControlRotationFromPawnOrientation = true;
}

void ANovaAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void ANovaAIController::IssueCommand(const FCommandData& CommandData)
{
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
	ANovaUnit* NovaUnit = Cast<ANovaUnit>(GetPawn());
	check(NovaUnit); // NovaAIController는 ANovaUnit 전용이어야 합니다.

	NovaUnit->MoveToLocation(TargetLocation);
	NOVA_LOG(Log, "AIController: Move command issued via NovaUnit to %s", *TargetLocation.ToString());
}

void ANovaAIController::HandleAttackCommand(AActor* TargetActor)
{
	if (!TargetActor) return;

	ANovaUnit* NovaUnit = Cast<ANovaUnit>(GetPawn());
	check(NovaUnit);

	// 공격 대상에게 다가가기 위해 FAIMoveRequest 설정 (AcceptanceRadius는 임시 사거리)
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalActor(TargetActor);
	MoveRequest.SetAcceptanceRadius(100.0f);
	MoveRequest.SetAllowPartialPath(true);
	MoveRequest.SetProjectGoalLocation(true);
	MoveRequest.SetRequireNavigableEndLocation(false);

	MoveTo(MoveRequest);
	
	NOVA_LOG(Log, "AIController: Attacking target %s", *TargetActor->GetName());
}

void ANovaAIController::HandleStopCommand()
{
	StopMovement();
	NOVA_LOG(Log, "AIController: Movement Stopped.");
}
