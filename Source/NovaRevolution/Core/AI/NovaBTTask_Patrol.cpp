// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaBTTask_Patrol.h"
#include "Core/AI/NovaAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "NovaRevolution.h"
#include "Navigation/PathFollowingComponent.h"
#include "Core/NovaUnit.h"
#include "GAS/NovaAttributeSet.h"
#include "GAS/NovaGameplayTags.h"
#include "AbilitySystemComponent.h"

UNovaBTTask_Patrol::UNovaBTTask_Patrol()
{
	NodeName = TEXT("Nova Patrol Task");
	bNotifyTick = true;
	bCreateNodeInstance = true;

	// 블랙보드 키 필터링
	TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Patrol, TargetLocationKey));
	PatrolOriginKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Patrol, PatrolOriginKey));
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Patrol, TargetActorKey), AActor::StaticClass());

	AbilityTag = NovaGameplayTags::Ability_Attack;
}

EBTNodeResult::Type UNovaBTTask_Patrol::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ANovaAIController* AIC = Cast<ANovaAIController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !BB || !AIC->GetPawn()) return EBTNodeResult::Failed;

	// 타겟 키 바인딩 확인
	if (TargetActorKey.IsNone())
	{
		NOVA_SCREEN(Warning, "Patrol Task: TargetActorKey is NOT set in Behavior Tree Editor!");
	}

	FVector CurrentLocation = AIC->GetPawn()->GetActorLocation();
	BB->SetValueAsVector(PatrolOriginKey.SelectedKeyName, CurrentLocation);

	bMovingToOrigin = false; 
	CombatOrigin = FVector::ZeroVector;
	
	FVector Goal = BB->GetValueAsVector(TargetLocationKey.SelectedKeyName);
	AIC->MoveToLocationOptimized(Goal, AcceptanceRadius);

	return EBTNodeResult::InProgress;
}

void UNovaBTTask_Patrol::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	ANovaAIController* AIC = Cast<ANovaAIController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ANovaUnit* MyUnit = Cast<ANovaUnit>(AIC->GetPawn());
	
	if (!AIC || !BB || !MyUnit)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 1. 적 발견 시 교전 및 추격 로직
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (Target && !Target->IsPendingKillPending())
	{
		// 타겟 사망 여부 확인
		if (ANovaUnit* TargetUnit = Cast<ANovaUnit>(Target))
		{
			if (TargetUnit->IsDead())
			{
				BB->ClearValue(TargetActorKey.SelectedKeyName);
				CombatOrigin = FVector::ZeroVector;
				return;
			}
		}

		// 교전 시작 지점 기록
		if (CombatOrigin.IsZero())
		{
			CombatOrigin = MyUnit->GetActorLocation();
		}

		float DistFromCombatOriginSq = FVector::DistSquared(MyUnit->GetActorLocation(), CombatOrigin);
		float LeashSq = FMath::Square(ChaseDistanceLimit);

		if (DistFromCombatOriginSq <= LeashSq)
		{
			float Range = GetAttackRange(MyUnit);
			float DistToTargetSq = FVector::DistSquared(MyUnit->GetActorLocation(), Target->GetActorLocation());

			if (DistToTargetSq <= FMath::Square(Range))
			{
				if (AIC->IsMoveInProgress())
				{
					AIC->StopMovementOptimized();
				}
				
				float CurrentTime = GetWorld()->GetTimeSeconds();
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
			else
			{
				AIC->MoveToActorOptimized(Target, 10.0f);
			}
			return; 
		}
		else
		{
			// 추격 한계선 벗어남
			BB->ClearValue(TargetActorKey.SelectedKeyName);
			CombatOrigin = FVector::ZeroVector;
			
			FVector Goal = bMovingToOrigin ? BB->GetValueAsVector(PatrolOriginKey.SelectedKeyName) : BB->GetValueAsVector(TargetLocationKey.SelectedKeyName);
			AIC->MoveToLocationOptimized(Goal, AcceptanceRadius);
		}
	}
	else
	{
		CombatOrigin = FVector::ZeroVector;
	}

	// 2. 순찰 이동 로직 (Target이 없을 때만 실행)
	if (!AIC->IsMoveInProgress())
	{
		// 도착 시 지점 교체
		bMovingToOrigin = !bMovingToOrigin;
		FVector NextGoal = bMovingToOrigin ? BB->GetValueAsVector(PatrolOriginKey.SelectedKeyName) : BB->GetValueAsVector(TargetLocationKey.SelectedKeyName);
		AIC->MoveToLocationOptimized(NextGoal, AcceptanceRadius);
	}
}

float UNovaBTTask_Patrol::GetAttackRange(ANovaUnit* Unit) const
{
	if (Unit && Unit->GetAbilitySystemComponent())
	{
		const UNovaAttributeSet* AS = Cast<UNovaAttributeSet>(Unit->GetAbilitySystemComponent()->GetAttributeSet(UNovaAttributeSet::StaticClass()));
		if (AS) return AS->GetRange();
	}
	return 500.0f;
}
