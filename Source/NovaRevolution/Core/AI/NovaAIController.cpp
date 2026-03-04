// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaAIController.h"
#include "VisualLogger/VisualLogger.h"
#include "Navigation/PathFollowingComponent.h"

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
	// 기존 이동 중지
	StopMovement();

	// 상세 설정과 함께 이동 명령 수행
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(TargetLocation);
	MoveRequest.SetAcceptanceRadius(5.0f);
	MoveRequest.SetAllowPartialPath(true); // 경로가 끊겨도 최대한 가까이 이동

	FPathFollowingRequestResult Result = MoveTo(MoveRequest);
	
	UE_LOG(LogTemp, Log, TEXT("AIController: Moving to %s (Result: %d)"), *TargetLocation.ToString(), (int32)Result.Code);
}

void ANovaAIController::HandleAttackCommand(AActor* TargetActor)
{
	if (!TargetActor) return;

	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalActor(TargetActor);
	MoveRequest.SetAcceptanceRadius(100.0f); // 임시 사거리

	MoveTo(MoveRequest);
	
	UE_LOG(LogTemp, Log, TEXT("AIController: Attacking target %s"), *TargetActor->GetName());
}

void ANovaAIController::HandleStopCommand()
{
	StopMovement();
	UE_LOG(LogTemp, Log, TEXT("AIController: Movement Stopped."));
}
