// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaBTTask_Attack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/NovaUnit.h"
#include "AbilitySystemComponent.h"
#include "GAS/NovaAttributeSet.h"
#include "NovaRevolution.h"
#include "Navigation/PathFollowingComponent.h"

UNovaBTTask_Attack::UNovaBTTask_Attack()
{
	NodeName = TEXT("Nova Attack Task");
	bNotifyTick = true;

	// 블랙보드 키 필터링 (Object 타입만 선택 가능하게 함)
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Attack, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UNovaBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	ANovaUnit* MyUnit = Cast<ANovaUnit>(AIC->GetPawn());
	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TargetActorKey.SelectedKeyName));

	if (!MyUnit || !Target) return EBTNodeResult::Failed;

	// 유닛이 살아있는지 확인 (생존 여부 체크 로직 필요)
	// 여기서는 단순히 IsPendingKill() 혹은 유사 로직을 사용하거나, 기획된 인터페이스 사용

	return EBTNodeResult::InProgress;
}

void UNovaBTTask_Attack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	ANovaUnit* MyUnit = Cast<ANovaUnit>(AIC->GetPawn());
	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TargetActorKey.SelectedKeyName));

	if (!MyUnit || !Target)
	{
		AIC->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	float DistanceSq = FVector::DistSquared(MyUnit->GetActorLocation(), Target->GetActorLocation());
	float Range = GetAttackRange(MyUnit);
	float RangeSq = FMath::Square(Range);

	// 1. 사거리 확인
	if (DistanceSq <= RangeSq)
	{
		// 사거리 내라면 이동 중단 후 공격
		AIC->StopMovement();
		
		// 공격 쿨타임 확인 (Attribute의 FireRate 활용 가능)
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastAttackTime >= AttackInterval)
		{
			PerformAttack(MyUnit, Target);
			LastAttackTime = CurrentTime;
		}
	}
	else
	{
		// 사거리 밖이라면 추적 이동
		AIC->MoveToActor(Target, Range * 0.8f); // 사거리의 80% 지점까지 접근
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

void UNovaBTTask_Attack::PerformAttack(ANovaUnit* Unit, AActor* Target)
{
	// TODO: GAS를 통한 공격 어빌리티 실행 로직 구현
	// 지금은 로그만 출력
	NOVA_LOG(Log, "Unit %s is attacking %s!", *Unit->GetName(), *Target->GetName());
	
	// GAS 연동 시 예시:
	// FGameplayTagContainer TagContainer;
	// TagContainer.AddTag(NovaGameplayTags::Ability_Attack);
	// Unit->GetAbilitySystemComponent()->TryActivateAbilitiesByTag(TagContainer);
}
