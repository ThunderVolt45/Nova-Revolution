// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaBTTask_Attack.h"
#include "Core/AI/NovaAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
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
	IsFocusAttackKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_Attack, IsFocusAttackKey));

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

	ENovaMovementType MoveType = MyUnit ? MyUnit->GetMovementType() : ENovaMovementType::Ground;

	// 목표로 액터가 전달되었을 경우
	if (Target)
	{
		float Range = GetAttackRange(MyUnit);
		if (!MyUnit->IsTargetInRange(Target, Range))
		{
			// 오차 방지를 위해 사거리의 50% 지점까지만 이동 (충분히 안으로 들어오도록 유도)
			AIC->MoveToActorOptimized(Target, Range * 0.5f);
		}
	}
	// 목표로 위치가 전달되었을 경우
	else if (!GoalLocation.IsZero())
	{
		// 지상 유닛인 경우 도달 불가능한 위치 보정
		if (MoveType == ENovaMovementType::Ground)
		{
			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			if (NavSys)
			{
				FNavLocation ProjectedLocation;
				// 수평 1000, 수직 2000 범위 내에서 가장 가까운 유효한 NavMesh 지점을 탐색합니다.
				if (NavSys->ProjectPointToNavigation(GoalLocation, ProjectedLocation, FVector(1000.f, 1000.f, 2000.f)))
				{
					GoalLocation = ProjectedLocation.Location;
				}
				else
				{
					// 투영에 실패했다는 것은 근처에 NavMesh가 아예 없다는 의미일 수 있음
					// 이때는 태스크를 실패시키기보다 최소한 현재 위치에서 그 방향으로 조금이라도 시도하도록 GoalLocation 유지
					// (엔진의 AllowPartialPath가 나머지를 처리)
				}
			}
		}
		
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
		// 1-0. 타겟의 사망 여부를 확인하고 죽었다면 Task를 즉시 성공시킵니다.
		bool bTargetIsDead = false;
		if (ANovaUnit* TargetUnit = Cast<ANovaUnit>(Target))
		{
			bTargetIsDead = TargetUnit->IsDead();
		}
		
		if (bTargetIsDead || Target->IsPendingKillPending())
		{
			NOVA_LOG(Log, "Unit %s Is Dead. Stop attack.", *Target->GetName());
			
			BB->ClearValue(TargetActorKey.SelectedKeyName);
			
			// 강제 공격(Focus Attack)인 경우에만 모든 데이터를 초기화하고 태스크 종료
			if (BB->GetValueAsBool(IsFocusAttackKey.SelectedKeyName))
			{
				BB->ClearValue(TargetLocationKey.SelectedKeyName);
				BB->SetValueAsEnum(CommandTypeKey.SelectedKeyName, (uint8)ECommandType::None);

				// 이동 중단 명령 명시적 호출
				AIC->StopMovementOptimized();

				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			}

			// 공격 이동(Attack-Move)인 경우 타겟만 제거하고 태스크를 유지하여 원래 목적지 이동 재개 유도
			return;
		}
		
		float Range = GetAttackRange(MyUnit);
		float DistXY = FVector::DistXY(MyUnit->GetActorLocation(), Target->GetActorLocation());
		// NOVA_LOG(Log, "[Attack Task] Unit %s -> Target %s. DistXY: %f, Range: %f", *MyUnit->GetName(), *Target->GetName(), DistXY, Range);

		// 1-1. 목표가 사거리 내에 있는 경우
		if (MyUnit->IsTargetInRange(Target, Range))
		{
			// NOVA_LOG(Log, "[Attack Task] IsTargetInRange returned TRUE.");
			
			// 최소 사거리 제한 추가
			if (MyUnit->IsTargetTooClose(Target))
			{
				// NOVA_LOG(Log, "[Attack Task] Target is too close, retreating.");
				
				// 사거리 안이지만 너무 가까움. 타겟과 반대 방향으로 물러나기 (중앙 집중화된 함수 사용)
				// 이제 정밀 계산이 동반되므로 여유값(Buffer)만 50 유닛 정도로 주면 됨
				AIC->RetreatFromTarget(Target, 50.0f);
			}
			else
			{
				// NOVA_LOG(Log, "[Attack Task] Target inside range. Stopping movement and attempting attack.");
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

				// 강제 공격(Focus Attack)인 경우에만 모든 데이터를 초기화하고 태스크 종료
				if (BB->GetValueAsBool(IsFocusAttackKey.SelectedKeyName))
				{
					BB->ClearValue(TargetLocationKey.SelectedKeyName);
					BB->SetValueAsEnum(CommandTypeKey.SelectedKeyName, (uint8)ECommandType::None);

					// 이동 중단 명령 명시적 호출
					AIC->StopMovementOptimized();

					FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
				}

				// 공격 이동(Attack-Move)인 경우 타겟만 제거하고 태스크를 유지하여 원래 목적지 이동 재개 유도
				return;
			}
		}
		// 1-2. 목표가 사거리 내에 없는 경우
		else
		{
			if (!AIC->IsMoveInProgress())
			{
				// NOVA_LOG(Log, "[Attack Task] IsTargetInRange returned FALSE. Moving to target.");
				
				// 추격 함수 호출 (AcceptanceRadius를 사거리의 50%로 설정하여 확실하고 안정적인 사거리 진입 보장)
				AIC->MoveToActorOptimized(Target, Range * 0.5f);
			}
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
