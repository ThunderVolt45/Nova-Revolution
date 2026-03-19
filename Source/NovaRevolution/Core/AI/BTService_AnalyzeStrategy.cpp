// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/BTService_AnalyzeStrategy.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/AI/NovaAIPlayerController.h"
#include "Core/NovaPlayerState.h"
#include "Core/NovaUnit.h"
#include "Core/NovaPart.h"
#include "Core/NovaBase.h"
#include "Core/NovaLog.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "GAS/NovaAttributeSet.h"
#include "NovaRevolution.h"

UBTService_AnalyzeStrategy::UBTService_AnalyzeStrategy()
{
	NodeName = TEXT("Analyze Strategy");
	Interval = 1.0f; // 전략 분석 주기
	RandomDeviation = 0.2f;

	RecommendedUnitSlotKey.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_AnalyzeStrategy, RecommendedUnitSlotKey));
	RecommendedSkillSlotKey.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_AnalyzeStrategy, RecommendedSkillSlotKey));
	SkillTargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_AnalyzeStrategy, SkillTargetActorKey), AActor::StaticClass());
	SkillTargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_AnalyzeStrategy, SkillTargetLocationKey));
	bShouldAttackKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_AnalyzeStrategy, bShouldAttackKey));
}

void UBTService_AnalyzeStrategy::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	ANovaAIPlayerController* AIC = Cast<ANovaAIPlayerController>(OwnerComp.GetAIOwner());
	if (!AIC)
	{
		NOVA_LOG(Error, "AI Strategy: Casting to ANovaAIPlayerController Failed!");
		return;
	}

	ANovaPlayerState* PS = AIC->GetPlayerState<ANovaPlayerState>();
	if (!PS)
	{
		NOVA_LOG(Error, "AI Strategy: Getting AI PlayerState Failed!");
		return;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		NOVA_LOG(Error, "AI Strategy: Getting Blackboard Failed!");
		return;
	}

	ANovaBase* MyBase = AIC->GetManagedBase();
	if (!MyBase)
	{
		NOVA_LOG(Error, "AI Strategy: Getting AI Base Failed!");
		return;
	}

	// 적 기지 위치가 아직 (0,0,0)이라면 다시 찾기 시도 (스폰 순서 이슈 대응)
	FVector EnemyBaseLoc = BB->GetValueAsVector(ANovaAIPlayerController::EnemyBaseLocationKey);
	if (EnemyBaseLoc.IsZero())
	{
		TArray<AActor*> FoundBases;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaBase::StaticClass(), FoundBases);
		NOVA_LOG(Log, "AI Strategy: Searching for enemy base. Total bases found in world: %d", FoundBases.Num());
		for (AActor* Actor : FoundBases)
		{
			ANovaBase* OtherBase = Cast<ANovaBase>(Actor);
			if (OtherBase)
			{
				NOVA_LOG(Log, "AI Strategy: Found base [%s] with TeamID: %d (AI TeamID: %d)", *OtherBase->GetName(), OtherBase->GetTeamID(), PS->GetTeamID());
				if (OtherBase->GetTeamID() != PS->GetTeamID())
				{
					BB->SetValueAsVector(ANovaAIPlayerController::EnemyBaseLocationKey, OtherBase->GetActorLocation());
					NOVA_LOG(Log, "AI Strategy: Set EnemyBaseLocation to %s", *OtherBase->GetActorLocation().ToString());
					break;
				}
			}
		}
	}

	// 매 틱 시작 시 이전 지시들을 기본 상태로 초기화 (태스크들이 잘못 반응하지 않도록 방지)
	BB->SetValueAsInt(RecommendedUnitSlotKey.SelectedKeyName, -1);
	BB->SetValueAsInt(RecommendedSkillSlotKey.SelectedKeyName, -1);
	BB->SetValueAsBool(bShouldAttackKey.SelectedKeyName, false);

	// 1. 위협 감지 (Threat Assessment)
	bool bEnemyNearBase = false;
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByChannel(Overlaps, MyBase->GetActorLocation(), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(ThreatDetectionRadius));
	
	for (auto& Overlap : Overlaps)
	{
		ANovaUnit* U = Cast<ANovaUnit>(Overlap.GetActor());
		if (U && U->GetTeamID() != PS->GetTeamID() && !U->IsDead())
		{
			bEnemyNearBase = true;
			break;
		}
	}

	AIC->SetEmergencyDefenseActive(bEnemyNearBase);

	// 2. 긴급 방어 수행
	if (AIC->IsEmergencyDefenseActive())
	{
		int32 DefenseSlot = AnalyzeDynamicCounter(AIC, PS, true);
		BB->SetValueAsInt(RecommendedUnitSlotKey.SelectedKeyName, DefenseSlot);
		// 방어 중이므로 진군/한타 공격 취소
		BB->SetValueAsBool(bShouldAttackKey.SelectedKeyName, false);
		return;
	}

	// 3. 자율 스킬 체계 (기지 방어, 리싸이클 등 예외 상황 처리)
	AnalyzeOccasionalSkills(AIC, PS, BB);

	// 4. 빌드 오더 수행 로직
	const FNovaAIBuildStep* CurrentStep = AIC->GetCurrentBuildStep();
	if (!CurrentStep) return;

	switch (CurrentStep->ActionType)
	{
		case ENovaAIBuildStepType::ProduceUnit:
		{
			int32 OverrideSlot = CurrentStep->TargetSlot;

			// 매크로 루프일 경우에만 동적 카운터 믹싱
			if (AIC->IsMacroLooping())
			{
				int32 CounterSlot = AnalyzeDynamicCounter(AIC, PS, false, CurrentStep->TargetSlot);
				if (CounterSlot != -1) OverrideSlot = CounterSlot;
			}

			BB->SetValueAsInt(RecommendedUnitSlotKey.SelectedKeyName, OverrideSlot);

			// 목표 수량 도달 시 다음 스텝으로
			if (CountUnitsOfSlot(PS, OverrideSlot) >= CurrentStep->TargetCount)
			{
				AIC->AdvanceBuildStep();
			}
			break;
		}
		case ENovaAIBuildStepType::UseSkill:
		{
			// 스킬을 시전하라는 추천을 띄움 (실제 전진은 BTTask_AIUseSkill에서 성공 시 수행)
			BB->SetValueAsInt(RecommendedSkillSlotKey.SelectedKeyName, CurrentStep->TargetSlot);
			break;
		}
		case ENovaAIBuildStepType::CommandAttack:
		{
			BB->SetValueAsBool(bShouldAttackKey.SelectedKeyName, true);
			AIC->AdvanceBuildStep();
			break;
		}
		case ENovaAIBuildStepType::Wait:
		{
			// Wait의 TargetCount를 '초 단위 대기 시간'으로 평가
			float Elapsed = GetWorld()->GetTimeSeconds() - AIC->GetLastStepStartTime();
			if (Elapsed >= (float)CurrentStep->TargetCount)
			{
				AIC->AdvanceBuildStep();
			}
			break;
		}
	}
}

int32 UBTService_AnalyzeStrategy::CountUnitsOfSlot(ANovaPlayerState* PS, int32 SlotIndex)
{
	int32 Count = 0;
	TArray<AActor*> AllUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaUnit::StaticClass(), AllUnits);

	for (AActor* Actor : AllUnits)
	{
		ANovaUnit* Unit = Cast<ANovaUnit>(Actor);
		if (Unit && Unit->GetTeamID() == PS->GetTeamID() && !Unit->IsDead() && Unit->GetProductionSlotIndex() == SlotIndex)
		{
			Count++;
		}
	}
	
	return Count;
}

int32 UBTService_AnalyzeStrategy::AnalyzeDynamicCounter(ANovaAIPlayerController* AIC, ANovaPlayerState* PS, bool bIsEmergency, int32 DefaultSlot)
{
	ANovaBase* MyBase = AIC->GetManagedBase();
	if (!MyBase || !PartSpecDataTable) return DefaultSlot;

	FNovaDeckInfo AISlotInfo = MyBase->GetProductionDeckInfo();
	if (AISlotInfo.Units.Num() == 0) return DefaultSlot;

	// 1. 적군 정보 취합
	TArray<AActor*> AllUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaUnit::StaticClass(), AllUnits);

	bool bEnemyHasAir = false;
	float MaxEnemyDefense = 0.0f;
	ENovaMovementType ToughestType = ENovaMovementType::Ground;

	for (AActor* Actor : AllUnits)
	{
		ANovaUnit* Unit = Cast<ANovaUnit>(Actor);
		
		if (!Unit) continue;
		if (Unit->GetTeamID() == PS->GetTeamID()) continue;
		if (Unit->IsDead()) continue;
		
		if (Unit->GetMovementType() == ENovaMovementType::Air) bEnemyHasAir = true;

		if (UNovaAttributeSet* AS = Unit->GetAttributeSet())
		{
			float Def = AS->GetDefense();
			if (Def > MaxEnemyDefense)
			{
				MaxEnemyDefense = Def;
				ToughestType = Unit->GetMovementType();
			}
		}
	}

	// 2. 슬롯별 점수 산정 (스코어링)
	TArray<float> SlotScores;
	SlotScores.Init(0.0f, AISlotInfo.Units.Num());

	for (int32 i = 0; i < AISlotInfo.Units.Num(); ++i)
	{
		FNovaPartSpecRow SlotStats;
		CalculateUnitPerformance(AISlotInfo.Units[i], SlotStats);

		// 기본: 자원(Watt) 대비 가성비 점수 (저렴할수록 점수 높음)
		SlotScores[i] += FMath::Max(200.0f - SlotStats.Watt, 0.0f);

		// 가중치 A: 적 공중 유닛 대응
		if (bEnemyHasAir)
		{
			if (SlotStats.TargetType == ENovaTargetType::AirOnly || SlotStats.TargetType == ENovaTargetType::All)
			{
				SlotScores[i] += 100.f;
			}
		}

		// 가중치 B: 가장 단단한 적으로부터 공격 가능 여부 판별 (카운터 화력 보정)
		bool bCanHitToughest = (ToughestType == ENovaMovementType::Air) ?
			(SlotStats.TargetType == ENovaTargetType::AirOnly || SlotStats.TargetType == ENovaTargetType::All) :
			(SlotStats.TargetType == ENovaTargetType::GroundOnly || SlotStats.TargetType == ENovaTargetType::All);

		if (bCanHitToughest)
		{
			SlotScores[i] += 100.f;
		}
		// 가중치 C: 가장 단단한 적 (방어력 상성)
		if (MaxEnemyDefense > 0.0f)
		{
			// 아군 공격력이 적 방어력보다 높을수록 가점 (최대 50점)
			float AttackAdvantage = SlotStats.Attack - MaxEnemyDefense;
			if (AttackAdvantage > 0)
			{
				SlotScores[i] += FMath::Min(AttackAdvantage * 2.5f, 50.0f);
			}
			else
			{
				// 공격력이 방어력보다 낮으면 뽑지 않는다 (방어력을 뚫지 못함)
				SlotScores[i] -= 10000.f;
			}
		}

		// 매크로 루프 모드 카운터 믹싱일 때, 원래 지정된 선호 유닛에 높은 기본 점수 부여
		if (!bIsEmergency && DefaultSlot != -1 && i == DefaultSlot)
		{
			// 선호 유닛은 +150점 보정. 단 대공(Air)이나 극단적 방어력 카운터 필요 시 스코어가 역전될 수 있음 
			SlotScores[i] += 250.0f;
		}
	}

	// 최고 점수 슬롯 반환
	int32 BestSlot = 0;
	float BestScore = -1.0f;
	for (int32 i = 0; i < SlotScores.Num(); ++i)
	{
		if (SlotScores[i] > BestScore)
		{
			BestScore = SlotScores[i];
			BestSlot = i;
		}
	}

	return BestSlot;
}

void UBTService_AnalyzeStrategy::CalculateUnitPerformance(const FNovaUnitAssemblyData& AssemblyData, FNovaPartSpecRow& OutStats)
{
	if (!PartSpecDataTable) return;

	auto GetSpec = [&](TSubclassOf<ANovaPart> PartClass) -> const FNovaPartSpecRow* {
		if (!PartClass) return nullptr;

		ANovaPart* DefaultPart = PartClass->GetDefaultObject<ANovaPart>();
		if (DefaultPart)
		{
			FName RowName = DefaultPart->GetPartID();
			if (!RowName.IsNone())
			{
				return PartSpecDataTable->FindRow<FNovaPartSpecRow>(RowName, TEXT("AnalyzeStrategy"));
			}
		}
		return nullptr;
	};

	const FNovaPartSpecRow* Legs = GetSpec(AssemblyData.LegsClass);
	const FNovaPartSpecRow* Body = GetSpec(AssemblyData.BodyClass);
	const FNovaPartSpecRow* Weapon = GetSpec(AssemblyData.WeaponClass);

	if (Legs)
	{
		OutStats.Watt += Legs->Watt;
		OutStats.Health += Legs->Health;
		OutStats.Defense += Legs->Defense;
		OutStats.Speed += Legs->Speed;
		OutStats.MovementType = Legs->MovementType;
	}
	if (Body)
	{
		OutStats.Watt += Body->Watt;
		OutStats.Health += Body->Health;
		OutStats.Defense += Body->Defense;
	}
	if (Weapon)
	{
		OutStats.Watt += Weapon->Watt;
		OutStats.Attack += Weapon->Attack;
		OutStats.TargetType = Weapon->TargetType;
		OutStats.Range += Weapon->Range;
	}
}

void UBTService_AnalyzeStrategy::AnalyzeOccasionalSkills(ANovaAIPlayerController* AIC, ANovaPlayerState* PS, UBlackboardComponent* BB)
{
	// 1. 리싸이클 (체력 20% 이하의 고가치 유닛 구제)
	TArray<AActor*> AllUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaUnit::StaticClass(), AllUnits);

	for (AActor* Actor : AllUnits)
	{
		ANovaUnit* Unit = Cast<ANovaUnit>(Actor);
		if (Unit && Unit->GetTeamID() == PS->GetTeamID() && !Unit->IsDead())
		{
			UNovaAttributeSet* AS = Unit->GetAttributeSet();
			if (AS && (AS->GetHealth() / AS->GetMaxHealth()) < 0.2f && AS->GetWatt() >= HighValueWattThreshold)
			{
				BB->SetValueAsInt(RecommendedSkillSlotKey.SelectedKeyName, RecycleSkillSlot);
				BB->SetValueAsObject(SkillTargetActorKey.SelectedKeyName, Unit);
				return;
			}
		}
	}

	// 그 외 프리즈(밀집 지역 대응) 등은 스킬 구조가 확장되면 여기에 추가
}
