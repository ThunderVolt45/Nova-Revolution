// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaBTTask_Hold.h"
#include "Core/AI/NovaAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/NovaUnit.h"
#include "AbilitySystemComponent.h"
#include "GAS/NovaAttributeSet.h"
#include "GAS/NovaGameplayTags.h"
#include "NovaRevolution.h"

UNovaBTTask_Hold::UNovaBTTask_Hold()
{
	NodeName = TEXT("Nova Hold Task");
	bNotifyTick = true;

	// 블랙보드 키 필터링
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Hold, TargetActorKey), AActor::StaticClass());

	AbilityTag = NovaGameplayTags::Ability_Attack;
}

EBTNodeResult::Type UNovaBTTask_Hold::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ANovaAIController* AIC = Cast<ANovaAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;

	// 초기 상태: 이동 즉시 중단 (수동 이동 포함)
	AIC->StopMovementOptimized();
	
	// NOVA_LOG(Log, "Unit %s entered HOLD state.", *AIC->GetPawn()->GetName());

	// Hold 상태를 유지 (명령이 바뀌기 전까지)
	return EBTNodeResult::InProgress;
}

void UNovaBTTask_Hold::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	ANovaAIController* AIC = Cast<ANovaAIController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !BB) return;

	ANovaUnit* MyUnit = Cast<ANovaUnit>(AIC->GetPawn());
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));

	// 유닛이 유효하지 않으면 종료
	if (!MyUnit) return;

	// Hold 상태에서는 절대 이동하지 않음 (강제 정지 상태 유지)
	if (AIC->IsMoveInProgress())
	{
		AIC->StopMovementOptimized();
	}

	if (Target && !Target->IsPendingKillPending())
	{
		// [추가] 방어 로직: 타겟이 자기 자신인 경우 즉시 취소
		if (Target == MyUnit)
		{
			BB->ClearValue(TargetActorKey.SelectedKeyName);
			return;
		}

		// 타겟이 유닛인 경우 사망 여부 확인
		if (ANovaUnit* TargetUnit = Cast<ANovaUnit>(Target))
		{
			if (TargetUnit->IsDead())
			{
				BB->ClearValue(TargetActorKey.SelectedKeyName);
				return;
			}
		}

		float Range = GetAttackRange(MyUnit);

		// [수정] 캡슐 기반 사거리 판정 함수 활용
		if (MyUnit->IsTargetInRange(Target, Range))
		{
			// [추가] 최소 사거리 내에 있는 경우 공격 무시
			if (MyUnit->IsTargetTooClose(Target))
			{
				return;
			}

			// 쿨다운 검사는 AIController 내부에서 통합 처리함
			AIC->ActivateAbilityByTag(AbilityTag, Target);
		}
	}
}

float UNovaBTTask_Hold::GetAttackRange(ANovaUnit* Unit) const
{
	if (UAbilitySystemComponent* ASC = Unit->GetAbilitySystemComponent())
	{
		return ASC->GetNumericAttribute(UNovaAttributeSet::GetRangeAttribute());
	}
	return 100.0f;
}
