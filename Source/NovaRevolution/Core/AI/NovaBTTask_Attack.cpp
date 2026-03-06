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
	bCreateNodeInstance = true;

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
	if (!MyUnit) return EBTNodeResult::Failed;

	// 타겟 액터와 목표 지점 모두 없는 경우에만 실패 처리
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	FVector GoalLocation = BB->GetValueAsVector(TargetLocationKey.SelectedKeyName);

	if (!Target && GoalLocation.IsZero())
	{
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
	FVector GoalLocation = BB->GetValueAsVector(TargetLocationKey.SelectedKeyName);

	if (!MyUnit)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 1. 우선순위: 타겟 액터가 있는 경우 (추격 및 공격)
	if (Target && !Target->IsPendingKillPending())
	{
		float DistanceSq = FVector::DistSquared(MyUnit->GetActorLocation(), Target->GetActorLocation());
		float Range = GetAttackRange(MyUnit);
		float RangeSq = FMath::Square(Range);

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
			// 이미 이 타겟으로 이동 중인지 확인하여 중복 명령 방지 가능 (MoveToActor가 내부적으로 처리하긴 함)
			AIC->MoveToActor(Target, Range * 0.95f); 
		}
	}
	// 2. 타겟 액터가 없지만 목표 지점이 있는 경우 (공격 이동 중)
	else if (!GoalLocation.IsZero())
	{
		// 목표 지점으로 이동 중인지 확인
		if (AIC->GetMoveStatus() != EPathFollowingStatus::Moving)
		{
			AIC->MoveToLocation(GoalLocation, 10.0f);
		}

		// 목표 지점에 거의 도달했는지 확인
		if (AIC->GetPathFollowingComponent()->DidMoveReachGoal())
		{
			// 적을 발견하지 못하고 지점에 도달했으므로 Idle 전환
			BB->SetValueAsEnum(CommandTypeKey.SelectedKeyName, (uint8)ECommandType::None);
			BB->ClearValue(TargetLocationKey.SelectedKeyName);
			
			NOVA_LOG(Log, "Unit %s finished Attack-Move to location. No target found, transitioning to Idle.", *MyUnit->GetName());
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
	// 3. 둘 다 없는 경우 (타겟 소실 및 지점 도달 등)
	else
	{
		AIC->StopMovement();
		BB->SetValueAsEnum(CommandTypeKey.SelectedKeyName, (uint8)ECommandType::None);
		BB->ClearValue(TargetActorKey.SelectedKeyName);

		NOVA_LOG(Log, "Unit %s attack command finished (No target/location).", *MyUnit->GetName());
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
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
