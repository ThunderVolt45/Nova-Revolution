// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaAIPlayerController.h"

#include "NovaRevolution.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/NovaBase.h"
#include "Core/NovaPlayerState.h"
#include "Core/NovaLog.h"
#include "Core/NovaUnit.h"
#include "Core/AI/NovaAIController.h"
#include "Core/NovaTypes.h"
#include "Kismet/GameplayStatics.h"

const FName ANovaAIPlayerController::WattKey = TEXT("CurrentWatt");
const FName ANovaAIPlayerController::PopulationKey = TEXT("CurrentPopulation");
const FName ANovaAIPlayerController::MaxPopulationKey = TEXT("MaxPopulation");
const FName ANovaAIPlayerController::RecommendedUnitSlotKey = TEXT("RecommendedUnitSlot");
const FName ANovaAIPlayerController::RecommendedSkillSlotKey = TEXT("RecommendedSkillSlot");
const FName ANovaAIPlayerController::EnemyBaseLocationKey = TEXT("EnemyBaseLocation");
const FName ANovaAIPlayerController::MyBaseLocationKey = TEXT("MyBaseLocation");

ANovaAIPlayerController::ANovaAIPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// AI 컨트롤러도 자원 및 팀 관리를 위해 PlayerState를 가지도록 설정
	bWantsPlayerState = true;

	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
}

void ANovaAIPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 1. 빌드 오더 선정
	RandomizeBuildOrder();

	// 2. 행동 트리 실행
	if (StrategyBTAsset)
	{
		RunBehaviorTree(StrategyBTAsset);
	}
	else
	{
		NOVA_LOG(Warning, "StrategyBTAsset is NOT assigned in %s!", *GetName());
	}
}

void ANovaAIPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANovaAIPlayerController::RandomizeBuildOrder()
{
	if (BuildOrderDataTable)
	{
		TArray<FNovaAIBuildOrderRow*> AllRows;
		BuildOrderDataTable->GetAllRows<FNovaAIBuildOrderRow>(TEXT("RandomizeBuildOrder"), AllRows);

		if (AllRows.Num() > 0)
		{
			int32 RandomIndex = FMath::RandRange(0, AllRows.Num() - 1);
			SelectedBuildOrder = AllRows[RandomIndex]->BuildOrder;
			
			NOVA_LOG(Log, "AI Player selected build order from DataTable: %s", *SelectedBuildOrder.ProfileName);
		}
		else
		{
			NOVA_LOG(Warning, "BuildOrderDataTable is empty in %s!", *GetName());
			SelectedBuildOrder.ProfileName = TEXT("Fallback (Empty DT)");
		}
	}
	else
	{
		NOVA_LOG(Warning, "BuildOrderDataTable is NOT assigned in %s!", *GetName());
		
		// 기본값 설정 (에디터에서 설정하지 않았을 경우를 대비한 최소한의 가동성 확보)
		SelectedBuildOrder.ProfileName = TEXT("Fallback (No DT)");
	}

	// 초기화
	CurrentBuildStepIndex = 0;
	bIsMacroLooping = false;
	bIsEmergencyDefenseActive = false;
}

const FNovaAIBuildStep* ANovaAIPlayerController::GetCurrentBuildStep() const
{
	// 긴급 방어 시 빌드 멈춤
	if (bIsEmergencyDefenseActive) return nullptr;

	const TArray<FNovaAIBuildStep>& TargetArray = bIsMacroLooping ? SelectedBuildOrder.MacroLoopSteps : SelectedBuildOrder.OpeningSteps;

	if (TargetArray.IsValidIndex(CurrentBuildStepIndex))
	{
		return &TargetArray[CurrentBuildStepIndex];
	}

	return nullptr;
}

void ANovaAIPlayerController::AdvanceBuildStep()
{
	if (bIsEmergencyDefenseActive) return;

	CurrentBuildStepIndex++;

	const TArray<FNovaAIBuildStep>& TargetArray = bIsMacroLooping ? SelectedBuildOrder.MacroLoopSteps : SelectedBuildOrder.OpeningSteps;

	// 배열 끝 도달 시
	if (CurrentBuildStepIndex >= TargetArray.Num())
	{
		if (!bIsMacroLooping)
		{
			// 오프닝 끝 -> 매크로 루프 시작
			bIsMacroLooping = true;
			CurrentBuildStepIndex = 0;
			NOVA_LOG(Log, "AI Player %s finished Opening, entering Macro Loop.", *GetName());
		}
		else
		{
			// 매크로 루프 끝 -> 처음으로 돌아가 무한 반복
			CurrentBuildStepIndex = 0;
		}
	}
}

void ANovaAIPlayerController::SetEmergencyDefenseActive(bool bActive)
{
	if (bIsEmergencyDefenseActive != bActive)
	{
		bIsEmergencyDefenseActive = bActive;
		if (bActive)
		{
			NOVA_LOG(Warning, "AI Player %s entered EMERGENCY DEFENSE override!", *GetName());
		}
		else
		{
			NOVA_LOG(Log, "AI Player %s emergency cleared, resuming build order.", *GetName());
		}
	}
}

void ANovaAIPlayerController::SetManagedBase(ANovaBase* InBase)
{
	ManagedBase = InBase;
	
	if (ManagedBase.IsValid())
	{
		// 1. 자신의 기지 위치 기록
		BlackboardComponent->SetValueAsVector(MyBaseLocationKey, ManagedBase->GetActorLocation());

		// 2. 적 기지 위치 캐싱
		TArray<AActor*> FoundBases;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaBase::StaticClass(), FoundBases);

		for (AActor* Actor : FoundBases)
		{
			ANovaBase* OtherBase = Cast<ANovaBase>(Actor);
			if (OtherBase && OtherBase->GetTeamID() != GetTeamID())
			{
				BlackboardComponent->SetValueAsVector(EnemyBaseLocationKey, OtherBase->GetActorLocation());
				break;
			}
		}
	}
}

int32 ANovaAIPlayerController::GetTeamID() const
{
	if (CachedTeamID != -1) return CachedTeamID;

	if (const ANovaPlayerState* PS = GetPlayerState<ANovaPlayerState>())
	{
		return PS->GetTeamID();
	}

	return -1;
}

void ANovaAIPlayerController::IssueCommandToAllUnits(const FCommandData& CommandData)
{
	TArray<AActor*> AllUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaUnit::StaticClass(), AllUnits);

	int32 MyTeamID = GetTeamID();
	int32 CommandCount = 0;

	for (AActor* Actor : AllUnits)
	{
		ANovaUnit* Unit = Cast<ANovaUnit>(Actor);
		if (Unit && Unit->GetTeamID() == MyTeamID && !Unit->IsDead())
		{
			if (ANovaAIController* UnitAIC = Cast<ANovaAIController>(Unit->GetController()))
			{
				UnitAIC->IssueCommand(CommandData);
				CommandCount++;
			}
		}
	}

	NOVA_LOG(Log, "AI Player issued command [%d] to %d units", 
		static_cast<int32>(CommandData.CommandType), CommandCount);
}

void ANovaAIPlayerController::LaunchGatheringWaves(const FCommandData& CommandData)
{
	int32 LaunchedWaveCount = 0;
	int32 LaunchedUnitCount = 0;

	for (FNovaAIWave& Wave : ActiveWaves)
	{
		if (Wave.CurrentState == ENovaAIWaveState::Gathering)
		{
			Wave.CurrentState = ENovaAIWaveState::Attacking;
			LaunchedWaveCount++;

			for (auto& WeakUnit : Wave.AssignedUnits)
			{
				if (ANovaUnit* Unit = WeakUnit.Get())
				{
					if (ANovaAIController* UnitAIC = Cast<ANovaAIController>(Unit->GetController()))
					{
						UnitAIC->IssueCommand(CommandData);
						LaunchedUnitCount++;
					}
				}
			}
		}
	}

	NOVA_LOG(Log, "AI Player %s launched Gathering Waves! Sent command to %d units.", *GetName(), LaunchedUnitCount);
}

void ANovaAIPlayerController::AddUnitToCurrentWave(ANovaUnit* NewUnit)
{
	if (!NewUnit) return;

	// 현재 Gathering 상태인 웨이브 찾기
	FNovaAIWave* GatheringWave = nullptr;
	for (FNovaAIWave& Wave : ActiveWaves)
	{
		if (Wave.CurrentState == ENovaAIWaveState::Gathering)
		{
			GatheringWave = &Wave;
			break;
		}
	}

	// 없다면 새로 생성
	if (!GatheringWave)
	{
		FNovaAIWave NewWave;
		NewWave.CurrentState = ENovaAIWaveState::Gathering;
		int32 Index = ActiveWaves.Add(NewWave);
		GatheringWave = &ActiveWaves[Index];
	}

	GatheringWave->AssignedUnits.Add(NewUnit);
	NOVA_LOG(Log, "AI Player %s: Added unit [%s] to wave. Current wave unit count: %d", *GetName(), *NewUnit->GetName(), GatheringWave->AssignedUnits.Num());
}

int32 ANovaAIPlayerController::CountUnitsOfSlot(int32 TargetSlot) const
{
	int32 Count = 0;
	TArray<AActor*> AllUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaUnit::StaticClass(), AllUnits);

	int32 MyTeamID = GetTeamID();

	for (AActor* Actor : AllUnits)
	{
		if (ANovaUnit* Unit = Cast<ANovaUnit>(Actor))
		{
			// 팀 ID가 같고 살아있으며 같은 슬롯 번호에서 생산된 유닛인지 확인
			if (Unit->GetTeamID() == MyTeamID && !Unit->IsDead() && Unit->GetProductionSlotIndex() == TargetSlot)
			{
				Count++;
			}
		}
	}

	return Count;
}
