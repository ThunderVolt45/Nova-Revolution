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

UBTService_AnalyzeStrategy::UBTService_AnalyzeStrategy()
{
	NodeName = TEXT("Analyze Strategy");
	Interval = 1.0f; // 전략 분석은 조금 더 긴 주기로 수행
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
	if (!AIC) return;

	ANovaPlayerState* PS = AIC->GetPlayerState<ANovaPlayerState>();
	if (!PS) return;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	// 1. 공격 개시 여부 및 상태 업데이트
	bool bWasAttacking = bIsAttacking;
	bool bCurrentShouldAttack = ShouldStartAttack(AIC, PS);
	BB->SetValueAsBool(bShouldAttackKey.SelectedKeyName, bCurrentShouldAttack);
	bIsAttacking = bCurrentShouldAttack;

	bool bAttackingJustStarted = !bWasAttacking && bIsAttacking;

	// 2. 생산 유닛 결정
	int32 BestUnitSlot = AnalyzeProduction(AIC, PS);
	BB->SetValueAsInt(RecommendedUnitSlotKey.SelectedKeyName, BestUnitSlot);

	// 3. 사용 스킬 및 타겟 결정
	AActor* TargetActor = nullptr;
	FVector TargetLocation = FVector::ZeroVector;
	int32 BestSkillSlot = AnalyzeSkills(AIC, PS, TargetActor, TargetLocation, bAttackingJustStarted);
	
	BB->SetValueAsInt(RecommendedSkillSlotKey.SelectedKeyName, BestSkillSlot);
	if (TargetActor) BB->SetValueAsObject(SkillTargetActorKey.SelectedKeyName, TargetActor);
	else BB->ClearValue(SkillTargetActorKey.SelectedKeyName);
	
	BB->SetValueAsVector(SkillTargetLocationKey.SelectedKeyName, TargetLocation);
}

void UBTService_AnalyzeStrategy::CalculateUnitPerformance(const FNovaUnitAssemblyData& AssemblyData, FNovaPartSpecRow& OutStats)
{
	if (!PartSpecDataTable) return;

	auto GetSpec = [&](TSubclassOf<ANovaPart> PartClass) -> const FNovaPartSpecRow* {
		if (!PartClass) return nullptr;
		
		// CDO에서 PartID를 가져와서 검색 시도
		ANovaPart* DefaultPart = PartClass->GetDefaultObject<ANovaPart>();
		if (DefaultPart)
		{
			FName RowName = DefaultPart->GetPartID();
			if (!RowName.IsNone())
			{
				const FNovaPartSpecRow* Found = PartSpecDataTable->FindRow<FNovaPartSpecRow>(RowName, TEXT("AnalyzeStrategy"));
				if (Found) return Found;
			}
		}

		// 실패 시 클래스 이름으로 시도 (블루프린트 접미사 제거)
		FString ClassName = PartClass->GetName();
		ClassName.RemoveFromEnd(TEXT("_C"));
		return PartSpecDataTable->FindRow<FNovaPartSpecRow>(FName(*ClassName), TEXT("AnalyzeStrategy"));
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

int32 UBTService_AnalyzeStrategy::AnalyzeProduction(ANovaAIPlayerController* AIC, ANovaPlayerState* PS)
{
	ANovaBase* MyBase = AIC->GetManagedBase();
	if (!MyBase || !PartSpecDataTable) return -1;

	FNovaDeckInfo AISlotInfo = MyBase->GetProductionDeckInfo();
	
	// 1. 적 전황 파악
	TArray<AActor*> AllUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaUnit::StaticClass(), AllUnits);
	
	bool bEnemyHasAir = false;
	float MaxEnemyDefense = 0.0f;
	ENovaMovementType ToughestType = ENovaMovementType::Ground;

	for (AActor* Actor : AllUnits)
	{
		ANovaUnit* Unit = Cast<ANovaUnit>(Actor);
		if (Unit && Unit->GetTeamID() != PS->GetTeamID() && !Unit->IsDead())
		{
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
	}

	// 2. 슬롯별 점수 산정
	TArray<float> SlotScores;
	SlotScores.Init(0.0f, AISlotInfo.Units.Num());
	const FNovaAIUnitComposition& FavoredComp = AIC->GetSelectedComposition();

	for (int32 i = 0; i < AISlotInfo.Units.Num(); ++i)
	{
		FNovaPartSpecRow SlotStats;
		CalculateUnitPerformance(AISlotInfo.Units[i], SlotStats);

		// 가중치 A: 선호 구성 (Profile Base)
		if (FavoredComp.SlotTargetWeights.Contains(i)) SlotScores[i] += FavoredComp.SlotTargetWeights[i] * 20.0f;

		// 가중치 B: 대공 대응 (적 공중 있을 시 대공 가능 유닛 가점)
		if (bEnemyHasAir)
		{
			if (SlotStats.TargetType == ENovaTargetType::AirOnly || SlotStats.TargetType == ENovaTargetType::All)
				SlotScores[i] += 50.0f;
		}

		// 가중치 C: 방어력 카운터 (고방어 적 타입 타격 가능 + 고공격력 가점)
		bool bCanHitToughest = (ToughestType == ENovaMovementType::Air) ? 
			(SlotStats.TargetType == ENovaTargetType::AirOnly || SlotStats.TargetType == ENovaTargetType::All) :
			(SlotStats.TargetType == ENovaTargetType::GroundOnly || SlotStats.TargetType == ENovaTargetType::All);

		if (bCanHitToughest) SlotScores[i] += (SlotStats.Attack * 0.5f);
	}

	int32 BestSlot = -1;
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

int32 UBTService_AnalyzeStrategy::AnalyzeSkills(ANovaAIPlayerController* AIC, ANovaPlayerState* PS, 
	AActor*& OutTargetActor, FVector& OutTargetLocation, bool bAttackingJustStarted)
{
	// 1. 오버워크 & 기지 소환 상황 (기지 방어 우선)
	ANovaBase* MyBase = AIC->GetManagedBase();
	ANovaPlayerState* EnemyPS = nullptr;
	TArray<AActor*> FoundPlayerStates;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaPlayerState::StaticClass(), FoundPlayerStates);
	
	for (AActor* Actor : FoundPlayerStates)
	{
		ANovaPlayerState* Temp = Cast<ANovaPlayerState>(Actor);
		if (Temp && Temp->GetTeamID() != PS->GetTeamID()) { EnemyPS = Temp; break; }
	}

	bool bBaseUnderAttack = false;
	if (MyBase && MyBase->GetAttributeSet() && MyBase->GetAttributeSet()->GetHealth() < MyBase->GetAttributeSet()->GetMaxHealth() * 0.95f)
	{
		bBaseUnderAttack = true;
	}

	// 오버워크 조건: (공격 개시) OR (자산 열세 && 기지 공격받음)
	if (bAttackingJustStarted || (EnemyPS && PS->GetCurrentWatt() < EnemyPS->GetCurrentWatt() && bBaseUnderAttack))
	{
		return OverworkSkillSlot;
	}

	// 2. 리싸이클: HP < 20% 고가치 유닛
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
				OutTargetActor = Unit;
				return RecycleSkillSlot;
			}
		}
	}

	// 3. 프리즈: 적 유닛 밀집 구역 (단순화: 적 유닛 반경 내 3기 이상)
	for (AActor* Actor : AllUnits)
	{
		ANovaUnit* Unit = Cast<ANovaUnit>(Actor);
		
		if (Unit && Unit->GetTeamID() != PS->GetTeamID() && !Unit->IsDead())
		{
			TArray<FOverlapResult> Overlaps;
			GetWorld()->OverlapMultiByChannel(Overlaps, Unit->GetActorLocation(), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(500.0f));
			int32 EnemyCount = 0;
			
			for (auto& Overlap : Overlaps)
			{
				ANovaUnit* U = Cast<ANovaUnit>(Overlap.GetActor());
				if (U && U->GetTeamID() != PS->GetTeamID()) EnemyCount++;
			}
			
			if (EnemyCount >= 3)
			{
				OutTargetLocation = Unit->GetActorLocation(); 
				return FreezeSkillSlot;
			} 
		}
	}

	// 4. 기지 소환: 공격 부대 손실 50% 이상
	if (bIsAttacking && PS->GetCurrentPopulation() < LastAttackStartedUnitCount * 0.5f) return BaseSummonSkillSlot;

	// 5. 자원 강화: 와트 자산 20% 우세 시 발동
	if (EnemyPS && PS->GetCurrentWatt() > EnemyPS->GetCurrentWatt() * 1.2f) return ResourceBoostSkillSlot;

	return -1;
}

bool UBTService_AnalyzeStrategy::ShouldStartAttack(ANovaAIPlayerController* AIC, ANovaPlayerState* PS)
{
	// 현재 인구수가 최대 인구수의 70% 이상이면 공격 개시
	if (!bIsAttacking && PS->GetCurrentPopulation() > PS->GetMaxPopulation() * 0.7f)
	{
		LastAttackStartedUnitCount = PS->GetCurrentPopulation();
		return true;
	}
	
	// 공격 중에는 모든 유닛을 잃을 때까지 지속 (혹은 기지 소환 전까지)
	if (bIsAttacking && PS->GetCurrentPopulation() <= 0)
	{
		return false;
	}

	return bIsAttacking;
}
