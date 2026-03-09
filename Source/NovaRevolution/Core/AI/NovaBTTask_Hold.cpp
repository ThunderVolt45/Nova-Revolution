// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaBTTask_Hold.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/NovaUnit.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
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
	AAIController* AIC = OwnerComp.GetAIOwner();
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

	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !BB) return;

	ANovaUnit* MyUnit = Cast<ANovaUnit>(AIC->GetPawn());
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));

	if (!MyUnit) return;

	// Hold 상태에서는 절대 이동하지 않음 (강제 정지 상태 유지)
	AIC->StopMovement();

	if (Target && !Target->IsPendingKillPending())
	{
		float DistanceSq = FVector::DistSquared(MyUnit->GetActorLocation(), Target->GetActorLocation());
		float Range = GetAttackRange(MyUnit);
		float RangeSq = FMath::Square(Range);

		// 사거리 내에 타겟이 있을 때만 공격 시도
		if (DistanceSq <= RangeSq)
		{
			float CurrentTime = GetWorld()->GetTimeSeconds();
			if (CurrentTime - LastAttackTime >= AttackInterval)
			{
				PerformAttack(MyUnit, Target);
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

void UNovaBTTask_Hold::PerformAttack(ANovaUnit* Unit, AActor* Target)
{
	if (Unit && Target && AbilityTag.IsValid())
	{
		FGameplayEventData Payload;
		Payload.Instigator = Unit;
		Payload.Target = Target;
		Payload.EventTag = AbilityTag;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Unit, AbilityTag, Payload);

		NOVA_LOG(Log, "Unit %s is attacking %s (from HOLD state) via GAS Event (%s)!", *Unit->GetName(), *Target->GetName(), *AbilityTag.ToString());
	}
}
