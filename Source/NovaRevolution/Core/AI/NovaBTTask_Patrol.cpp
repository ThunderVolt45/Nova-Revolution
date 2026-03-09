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

	// 타겟 키 바인딩 확인 (에디터 설정 누락 방지)
	if (TargetActorKey.IsNone())
	{
		NOVA_SCREEN(Warning, "Patrol Task: TargetActorKey is NOT set in Behavior Tree Editor!");
	}

	FVector CurrentLocation = AIC->GetPawn()->GetActorLocation();
	BB->SetValueAsVector(PatrolOriginKey.SelectedKeyName, CurrentLocation);

	bMovingToOrigin = false; 
	CombatOrigin = FVector::ZeroVector;
	
	FVector Goal = BB->GetValueAsVector(TargetLocationKey.SelectedKeyName);
	AIC->MoveToLocation(Goal, AcceptanceRadius);

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
		// 타겟이 유닛인 경우 사망 여부 확인
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
			NOVA_LOG(Log, "Patrol: Enemy spotted! CombatOrigin set to %s", *CombatOrigin.ToString());
		}

		float DistFromCombatOriginSq = FVector::DistSquared(MyUnit->GetActorLocation(), CombatOrigin);
		float LeashSq = FMath::Square(ChaseDistanceLimit);

		if (DistFromCombatOriginSq <= LeashSq)
		{
			float Range = GetAttackRange(MyUnit);
			float DistToTargetSq = FVector::DistSquared(MyUnit->GetActorLocation(), Target->GetActorLocation());

			if (DistToTargetSq <= FMath::Square(Range))
			{
				AIC->StopMovement();
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
			else
			{
				AIC->MoveToActor(Target, Range * 0.9f);
			}
			
			// 교전 중이므로 여기서 리턴하여 아래의 순찰 로직 실행 방지
			return; 
		}
		else
		{
			// 추격 한계선 벗어남
			BB->ClearValue(TargetActorKey.SelectedKeyName);
			CombatOrigin = FVector::ZeroVector;
			NOVA_SCREEN(Log, "Patrol: Target/Unit out of leash range (%f). Resuming patrol.", ChaseDistanceLimit);
			
			// 순찰 지점으로 다시 이동 명령 명시적 수행
			FVector Goal = bMovingToOrigin ? BB->GetValueAsVector(PatrolOriginKey.SelectedKeyName) : BB->GetValueAsVector(TargetLocationKey.SelectedKeyName);
			AIC->MoveToLocation(Goal, AcceptanceRadius);
		}
	}
	else
	{
		CombatOrigin = FVector::ZeroVector;
	}

	// 2. 순찰 이동 로직 (Target이 없을 때만 실행됨)
	if (AIC->GetPathFollowingComponent()->GetStatus() == EPathFollowingStatus::Idle)
	{
		if (AIC->GetPathFollowingComponent()->DidMoveReachGoal())
		{
			bMovingToOrigin = !bMovingToOrigin;
			FVector NextGoal = bMovingToOrigin ? BB->GetValueAsVector(PatrolOriginKey.SelectedKeyName) : BB->GetValueAsVector(TargetLocationKey.SelectedKeyName);
			AIC->MoveToLocation(NextGoal, AcceptanceRadius);
			NOVA_LOG(Log, "Patrol: Reached goal, switching to %s", bMovingToOrigin ? TEXT("Origin") : TEXT("Target"));
		}
		else
		{
			// 이동이 멈췄는데 목표에 도달하지 않은 경우 (교전 직후 또는 경로 막힘) 재시작
			FVector NextGoal = bMovingToOrigin ? BB->GetValueAsVector(PatrolOriginKey.SelectedKeyName) : BB->GetValueAsVector(TargetLocationKey.SelectedKeyName);
			AIC->MoveToLocation(NextGoal, AcceptanceRadius);
		}
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
