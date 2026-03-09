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
	// 비헤이비어 트리 실행
	if (BehaviorTreeAsset)
	{
		bool bSuccess = RunBehaviorTree(BehaviorTreeAsset);
		if (bSuccess)
		{
			NOVA_LOG(Log, "AIController: Successfully started Behavior Tree [%s] for Pawn [%s]", *BehaviorTreeAsset->GetName(), *InPawn->GetName());
		}
		else
		{
			NOVA_LOG(Error, "AIController: Failed to run Behavior Tree [%s] for Pawn [%s]", *BehaviorTreeAsset->GetName(), *InPawn->GetName());
		}
	}
	else
	{
		NOVA_LOG(Warning, "AIController: BehaviorTreeAsset is NOT assigned in %s! (Pawn: %s)", *GetName(), InPawn ? *InPawn->GetName() : TEXT("NULL"));
	}
	
	Super::OnPossess(InPawn);
}

void ANovaAIController::IssueCommand(const FCommandData& CommandData)
{
	if (BlackboardComponent && BlackboardComponent->GetBlackboardAsset())
	{
		APawn* MyPawn = GetPawn();
		if (!MyPawn) return;

		// 1. [Fix] 자기 자신을 대상으로 하는 공격 명령 차단
		if (CommandData.CommandType == ECommandType::Attack && CommandData.TargetActor == MyPawn)
		{
			NOVA_LOG(Warning, "AIController: Self-attack command ignored.");
			return;
		}

		// 2. 블랙보드 데이터 업데이트
		BlackboardComponent->SetValueAsEnum(CommandTypeKey, (uint8)CommandData.CommandType);
		
		switch (CommandData.CommandType)
		{
		case ECommandType::Move:
		case ECommandType::Patrol:
			// 이미 해당 위치(최소 사거리) 근처에 있다면 불필요한 이동 명령 방지
			if (FVector::DistSquared(MyPawn->GetActorLocation(), CommandData.TargetLocation) < FMath::Square(MinMoveDistance))
			{
				BlackboardComponent->SetValueAsEnum(CommandTypeKey, (uint8)ECommandType::None);
				return;
			}
			BlackboardComponent->SetValueAsVector(TargetLocationKey, CommandData.TargetLocation);
			BlackboardComponent->ClearValue(TargetActorKey);
			NOVA_LOG(Log, "AIController: %s command synced to BB (%s)", 
				CommandData.CommandType == ECommandType::Move ? TEXT("Move") : TEXT("Patrol"),
				*CommandData.TargetLocation.ToString());
			break;

		case ECommandType::Attack:
			if (CommandData.TargetActor)
			{
				BlackboardComponent->SetValueAsObject(TargetActorKey, CommandData.TargetActor);
				BlackboardComponent->SetValueAsVector(TargetLocationKey, CommandData.TargetActor->GetActorLocation());
				NOVA_LOG(Log, "AIController: Attack command (Actor) synced to BB (Target: %s)", *CommandData.TargetActor->GetName());
			}
			else
			{
				BlackboardComponent->SetValueAsVector(TargetLocationKey, CommandData.TargetLocation);
				BlackboardComponent->ClearValue(TargetActorKey);
				NOVA_LOG(Log, "AIController: Attack command (Location) synced to BB (%s)", *CommandData.TargetLocation.ToString());
			}
			break;

		case ECommandType::Stop:
		case ECommandType::Hold:
		case ECommandType::Halt:
			BlackboardComponent->ClearValue(TargetLocationKey);
			BlackboardComponent->ClearValue(TargetActorKey);
			StopMovement();
			NOVA_LOG(Log, "AIController: %s command synced to BB.", 
				CommandData.CommandType == ECommandType::Stop ? TEXT("Stop") : 
				(CommandData.CommandType == ECommandType::Hold ? TEXT("Hold") : TEXT("Halt")));
			break;

		case ECommandType::Spread:
			BlackboardComponent->SetValueAsVector(TargetLocationKey, CommandData.TargetLocation);
			BlackboardComponent->ClearValue(TargetActorKey);
			NOVA_LOG(Log, "AIController: Spread command synced to BB.");
			break;

		default:
			break;
		}

		// 2. 비헤이비어 트리 강제 재시작 (즉각적인 반응성 확보)
		if (BehaviorTreeComponent)
		{
			BehaviorTreeComponent->RestartLogic();
		}
	}
}
