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
	// 블랙보드 데이터 갱신 (BT에서 이를 보고 상태 전환)
	if (BlackboardComponent && BlackboardComponent->GetBlackboardAsset())
	{
		BlackboardComponent->SetValueAsEnum(CommandTypeKey, (uint8)CommandData.CommandType);
		
		// 명령 타입에 따른 추가 데이터 설정
		switch (CommandData.CommandType)
		{
		case ECommandType::Move:
			BlackboardComponent->SetValueAsVector(TargetLocationKey, CommandData.TargetLocation);
			BlackboardComponent->ClearValue(TargetActorKey);
			NOVA_LOG(Log, "AIController: Move command synced to BB (%s)", *CommandData.TargetLocation.ToString());
			break;

		case ECommandType::Attack:
			if (CommandData.TargetActor)
			{
				BlackboardComponent->SetValueAsObject(TargetActorKey, CommandData.TargetActor);
				BlackboardComponent->SetValueAsVector(TargetLocationKey, CommandData.TargetActor->GetActorLocation());
				NOVA_LOG(Log, "AIController: Attack command synced to BB (Target: %s)", *CommandData.TargetActor->GetName());
			}
			break;

		case ECommandType::Stop:
		case ECommandType::Hold:
			BlackboardComponent->ClearValue(TargetLocationKey);
			BlackboardComponent->ClearValue(TargetActorKey);
			StopMovement(); // 물리적 정지는 즉시 수행
			NOVA_LOG(Log, "AIController: Stop command synced to BB.");
			break;

		default:
			break;
		}
	}
}
