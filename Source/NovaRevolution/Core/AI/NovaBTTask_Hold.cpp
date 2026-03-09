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

	// 초기 상태: 이동 즉시 중단
	AIC->StopMovement();
	
	NOVA_LOG(Log, "Unit %s entered HOLD state.", *AIC->GetPawn()->GetName());

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
	AIC->StopMovement();

	if (Target && !Target->IsPendingKillPending())
	{
		// 타겟이 유닛인 경우 사망 여부 확인
		if (ANovaUnit* TargetUnit = Cast<ANovaUnit>(Target))
		{
			if (TargetUnit->IsDead())
			{
				BB->ClearValue(TargetActorKey.SelectedKeyName);
				return;
			}
		}

		float DistanceSq = FVector::DistSquared(MyUnit->GetActorLocation(), Target->GetActorLocation());
		float Range = GetAttackRange(MyUnit);
		float RangeSq = FMath::Square(Range);

		// 사거리 내에 타겟이 있을 때만 공격 시도
		if (DistanceSq <= RangeSq)
		{
			float CurrentTime = GetWorld()->GetTimeSeconds();

			// FireRate 연동 (수치가 작을수록 빠름, 100 = 1.0s, 50 = 0.5s)
			float CurrentAttackInterval = AttackInterval;
			if (UAbilitySystemComponent* ASC = MyUnit->GetAbilitySystemComponent())
			{
				float FireRateValue = ASC->GetNumericAttribute(UNovaAttributeSet::GetFireRateAttribute());
				if (FireRateValue > 0.0f)
				{
					CurrentAttackInterval = FireRateValue / 100.0f;
				}
			}

			if (CurrentTime - LastAttackTime >= CurrentAttackInterval)
			{
				AIC->ActivateAbilityByTag(AbilityTag, Target);
				LastAttackTime = CurrentTime;
			}
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
