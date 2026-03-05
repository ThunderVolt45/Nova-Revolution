// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaBTTask_Attack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/NovaUnit.h"
#include "Core/NovaTypes.h"
#include "AbilitySystemComponent.h"
#include "GAS/NovaAttributeSet.h"
#include "NovaRevolution.h"
#include "Navigation/PathFollowingComponent.h"

UNovaBTTask_Attack::UNovaBTTask_Attack()
{
	NodeName = TEXT("Nova Attack Task");
	bNotifyTick = true;

	// 블랙보드 키 필터링
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Attack, TargetActorKey), AActor::StaticClass());
	TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Attack, TargetLocationKey));
	CommandTypeKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Attack, CommandTypeKey), StaticEnum<ECommandType>());
}

EBTNodeResult::Type UNovaBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !BB) return EBTNodeResult::Failed;

	ANovaUnit* MyUnit = Cast<ANovaUnit>(AIC->GetPawn());
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));

	if (!MyUnit || !Target)
	{
		// 타겟이 없다면 즉시 Idle 전환
		BB->SetValueAsEnum(CommandTypeKey.SelectedKeyName, (uint8)ECommandType::None);
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

void UNovaBTTask_Attack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	ANovaUnit* MyUnit = Cast<ANovaUnit>(AIC->GetPawn());
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));

	// 타겟이 사라졌거나 죽은 경우
	if (!MyUnit || !Target || Target->IsPendingKillPending())
	{
		AIC->StopMovement();
		
		// 상태를 Idle로 초기화
		BB->SetValueAsEnum(CommandTypeKey.SelectedKeyName, (uint8)ECommandType::None);
		BB->ClearValue(TargetActorKey.SelectedKeyName);
		BB->ClearValue(TargetLocationKey.SelectedKeyName);

		NOVA_LOG(Log, "Unit %s target lost or destroyed. Transitioning to Idle.", *MyUnit->GetName());
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
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
		AIC->MoveToActor(Target, Range * 0.98f); 
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
