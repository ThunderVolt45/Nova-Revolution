// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaBTTask_Attack.h"
#include "Core/AI/NovaAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/NovaUnit.h"
#include "Core/NovaTypes.h"
#include "AbilitySystemComponent.h"
#include "GAS/NovaAttributeSet.h"
#include "GAS/NovaGameplayTags.h"
#include "NovaRevolution.h"
#include "Navigation/PathFollowingComponent.h"

UNovaBTTask_Attack::UNovaBTTask_Attack()
{
	NodeName = TEXT("Nova Attack Task");
	bNotifyTick = true;
	bCreateNodeInstance = true;

	// 블랙보드 키 필터링
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Attack, TargetActorKey), AActor::StaticClass());
	TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Attack, TargetLocationKey));
	CommandTypeKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Attack, CommandTypeKey), StaticEnum<ECommandType>());

	AbilityTag = NovaGameplayTags::Ability_Attack;
}

EBTNodeResult::Type UNovaBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ANovaAIController* AIC = Cast<ANovaAIController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !BB) return EBTNodeResult::Failed;

	ANovaUnit* MyUnit = Cast<ANovaUnit>(AIC->GetPawn());
	if (!MyUnit) return EBTNodeResult::Failed;

	// 타겟 액터와 목표 지점 모두 없는 경우에만 실패 처리
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	FVector GoalLocation = BB->GetValueAsVector(TargetLocationKey.SelectedKeyName);

	if (!Target && GoalLocation.IsZero())
	{
		BB->SetValueAsEnum(CommandTypeKey.SelectedKeyName, (uint8)ECommandType::None);
		return EBTNodeResult::Failed;
	}

	// 새로운 명령이 하달되어 ExecuteTask가 실행될 때, 즉시 이동을 시작하여 반응성 확보
	if (!Target && !GoalLocation.IsZero())
	{
		AIC->MoveToLocationOptimized(GoalLocation, 10.0f);
	}

	return EBTNodeResult::InProgress;
}

void UNovaBTTask_Attack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	ANovaAIController* AIC = Cast<ANovaAIController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	ANovaUnit* MyUnit = Cast<ANovaUnit>(AIC->GetPawn());
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	FVector GoalLocation = BB->GetValueAsVector(TargetLocationKey.SelectedKeyName);

	// 유닛이 유효하지 않으면 종료 (사망 시 처리는 AIController에서 StopTree 호출로 처리됨)
	if (!MyUnit)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 1. 우선순위: 타겟 액터가 있는 경우 (추격 및 공격)
	if (Target)
	{
		// 타겟의 사망 여부를 확인하고 죽었다면 Task를 즉시 성공시킵니다.
		bool bTargetIsDead = false;
		if (ANovaUnit* TargetUnit = Cast<ANovaUnit>(Target))
		{
			bTargetIsDead = TargetUnit->IsDead();
		}
		
		if (bTargetIsDead || Target->IsPendingKillPending())
		{
			NOVA_LOG(Log, "Unit %s Is Dead. Stop attack.", *Target->GetName());
			
			BB->ClearValue(TargetActorKey.SelectedKeyName);
			BB->ClearValue(TargetLocationKey.SelectedKeyName);
			BB->SetValueAsEnum(CommandTypeKey.SelectedKeyName, (uint8)ECommandType::None);

			// 이동 중단 명령 명시적 호출
			AIC->StopMovementOptimized();

			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
		
		float Range = GetAttackRange(MyUnit);

		// [수정] 캡슐 기반 사거리 판정 함수 활용
		if (MyUnit->IsTargetInRange(Target, Range))
		{
			// 최소 사거리 제한 추가
			if (MyUnit->IsTargetTooClose(Target))
			{
				// 사거리 안이지만 너무 가까움. 타겟과 반대 방향으로 물러나기 (중앙 집중화된 함수 사용)
				// 이제 정밀 계산이 동반되므로 여유값(Buffer)만 50 유닛 정도로 주면 됨
				AIC->RetreatFromTarget(Target, 50.0f);
			}
			else
			{
				// 사거리 내라면 즉시 이동 중단 후 공격 수행
				if (AIC->IsMoveInProgress())
				{
					AIC->StopMovementOptimized();
				}
				
				// 쿨다운 검사는 AIController 내부에서 통합 처리함
				AIC->ActivateAbilityByTag(AbilityTag, Target);
			}
			
			// 타겟의 사망 여부를 확인하고 죽었다면 Task를 즉시 성공시킵니다.
			if (ANovaUnit* TargetUnit = Cast<ANovaUnit>(Target))
			{
				bTargetIsDead = TargetUnit->IsDead();
			}
		
			if (bTargetIsDead || Target->IsPendingKillPending())
			{
				NOVA_LOG(Log, "Unit %s Is Dead. Stop attack.", *Target->GetName());
			
				BB->ClearValue(TargetActorKey.SelectedKeyName);
				BB->ClearValue(TargetLocationKey.SelectedKeyName);
				BB->SetValueAsEnum(CommandTypeKey.SelectedKeyName, (uint8)ECommandType::None);

				// 이동 중단 명령 명시적 호출
				AIC->StopMovementOptimized();

				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
				return;
			}
		}
		else
		{
			// 추격 함수 호출
			AIC->MoveToActorOptimized(Target, 10.0f); 
		}

		return; // 타겟 액터 로직을 수행했으므로 하단의 지점 이동 로직은 실행하지 않음
	}

	// 2. 타겟 액터가 없지만 목표 지점이 있는 경우 (공격 이동 중)
	if (!GoalLocation.IsZero())
	{
		// 단순히 Moving 상태인지 체크하는 대신, 현재 경로의 도착지가 목표 지점과 일치하는지 확인하거나 
		// 혹은 Tick에서 주기적으로 MoveToLocation을 재호출하여 갱신을 보장합니다.
		if (AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			AIC->MoveToLocationOptimized(GoalLocation, 10.0f);
		}

		// 목표 지점에 거의 도달했는지 확인
		if (AIC->GetPathFollowingComponent()->DidMoveReachGoal())
		{
			// 적을 발견하지 못하고 지점에 도달했으므로 Idle 전환
			BB->SetValueAsEnum(CommandTypeKey.SelectedKeyName, (uint8)ECommandType::None);
			BB->ClearValue(TargetLocationKey.SelectedKeyName);
			
			// NOVA_LOG(Log, "Unit %s finished Attack-Move to location. No target found, transitioning to Idle.", *MyUnit->GetName());
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
	// 3. 둘 다 없는 경우 (타겟 소실 및 지점 도달 등)
	else
	{
		BB->SetValueAsEnum(CommandTypeKey.SelectedKeyName, (uint8)ECommandType::None);
		BB->ClearValue(TargetActorKey.SelectedKeyName);
		BB->ClearValue(TargetLocationKey.SelectedKeyName);

		AIC->StopMovementOptimized();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		// NOVA_LOG(Log, "Unit %s attack command finished (No target/location).", *MyUnit->GetName());
	}
}

float UNovaBTTask_Attack::GetAttackRange(ANovaUnit* Unit) const
{
	if (UAbilitySystemComponent* ASC = Unit->GetAbilitySystemComponent())
	{
		return ASC->GetNumericAttribute(UNovaAttributeSet::GetRangeAttribute());
	}
	return 100.0f;
}
