#include "Core/Production/NovaUnitFactory.h"
#include "Core/NovaGameMode.h"
#include "Core/NovaInterfaces.h"
#include "Core/NovaPlayerState.h"
#include "Core/NovaUnit.h"
#include "Core/NovaResourceComponent.h"
#include "Core/NovaPartData.h"
#include "Kismet/GameplayStatics.h"
#include "NovaRevolution.h"
#include "Core/NovaObjectPoolSubsystem.h"
#include "Core/NovaPart.h"
#include "Player/NovaPlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Core/Production/NovaFactorySettings.h"

bool UNovaUnitFactory::RequestSpawnUnitFromDeck(int32 SlotIndex, AActor* Spawner, const FVector& RallyPoint)
{
	// NOVA_SCREEN(Log, "[SpawnTrace] 1. RequestSpawnUnitFromDeck Started - Slot: %d", SlotIndex);

	// 1. 매개변수 유효성 검사
	if (!Spawner) 
	{
		// NOVA_SCREEN(Error, "[SpawnTrace] Spawner is NULL!");
		return false;
	}

	// 2. 팀 식별자 결정 (Spawner가 INovaTeamInterface를 구현하고 있다면 해당 팀 ID 사용)
	int32 TargetTeamID = NovaTeam::Team1;
	if (INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(Spawner))
	{
		TargetTeamID = TeamInterface->GetTeamID();
	}

	// 3. 해당 팀의 PlayerState 찾기
	ANovaPlayerState* PS = nullptr;
	UWorld* World = GetWorld();
	if (World)
	{
		if (AGameStateBase* GameState = World->GetGameState())
		{
			for (APlayerState* PlayerStateObj : GameState->PlayerArray)
			{
				if (ANovaPlayerState* TempPS = Cast<ANovaPlayerState>(PlayerStateObj))
				{
					if (INovaTeamInterface* PSTeamInterface = Cast<INovaTeamInterface>(TempPS))
					{
						if (PSTeamInterface->GetTeamID() == TargetTeamID)
						{
							PS = TempPS;
							break;
						}
					}
				}
			}
		}
	}

	// 4. GameMode에서 해당 팀의 덱 정보 획득
	ANovaGameMode* GM = Cast<ANovaGameMode>(UGameplayStatics::GetGameMode(World));
	if (!GM) return false;

	// TargetTeamID를 인자로 전달하여 해당 팀의 덱 데이터를 안전하게 불러옵니다.
	FNovaDeckInfo CurrentDeck = GM->GetPlayerDeck(TargetTeamID);

	// 5. 덱 슬롯 유효성 검사
	if (!CurrentDeck.Units.IsValidIndex(SlotIndex))
	{
		NOVA_LOG(Warning, "Factory: Invalid Deck Slot %d for Team %d", SlotIndex, TargetTeamID);
		return false;
	}

	const FNovaUnitAssemblyData& TargetData = CurrentDeck.Units[SlotIndex];
	// NOVA_SCREEN(Log, "[SpawnTrace] 2. Deck Valid - UnitName: %s", *TargetData.UnitName);

	// 6. 자원 검사 및 소비 로직
	// 데이터 테이블을 참조하여 실제 유닛 생산 비용을 계산합니다.
	float ProductionCost = CalculateTotalWattCost(TargetData);
	if (!CheckAndConsumeResources(PS, ProductionCost))
	{
		// NOVA_SCREEN(Warning, "[SpawnTrace] Insufficient Resources! CheckAndConsumeResources failed.");
		return false;
	}

	// NOVA_SCREEN(Log, "[SpawnTrace] 3. Resources Consumed - Cost: %.1f", ProductionCost);

	// 7. 스폰 위치 결정 (기지 전방 오프셋)
	FTransform SpawnTransform = Spawner->GetActorTransform();
	FVector SpawnLocation = SpawnTransform.GetLocation() + Spawner->GetActorForwardVector() * 350.f;
	SpawnTransform.SetLocation(SpawnLocation);
	
	// 유닛의 크기가 기지(Spawner)의 스케일을 따라가지 않도록 (1, 1, 1)로 초기화
	SpawnTransform.SetScale3D(FVector::OneVector);

	// 8. 실제 유닛 즉시 스폰 및 데이터 주입
	// NOVA_SCREEN(Log, "[SpawnTrace] 4. Calling ExecuteUnitProduction");
	ANovaUnit* NewUnit = ExecuteUnitProduction(TargetData, SpawnTransform, TargetTeamID, RallyPoint, SlotIndex);

	if (NewUnit)
	{
		NOVA_LOG(Log, "Factory: Unit '%s' (Cost: %.f) successfully spawned for Team %d.",
		       *TargetData.UnitName, ProductionCost, TargetTeamID);
		
		// 생성된 유닛 즉시 부대 지정 편입
		if (PS)
		{
			if (ANovaPlayerController* PC = Cast<ANovaPlayerController>(PS->GetPlayerController()))
			{
				PC->OnUnitProduced(NewUnit, SlotIndex);
			}
		}
		
		return true;
	}

	return false;
}

float UNovaUnitFactory::CalculateTotalWattCost(const FNovaUnitAssemblyData& AssemblyData) const
{
	float TotalWatt = 0.0f;

	const UNovaFactorySettings* FactorySettings = GetDefault<UNovaFactorySettings>();
	UDataTable* Table = FactorySettings->PartSpecDataTable.LoadSynchronous();
	
	if (!Table)
	{
		NOVA_LOG(Warning, "Factory: PartSpecDataTable is not assigned or could not be loaded in NovaFactorySettings!");
		return 100.0f;
	}

	auto GetWattFromPartClass = [&](TSubclassOf<ANovaPart> PartClass) -> float
	{
		if (!PartClass) return 0.0f;
		if (ANovaPart* DefaultPart = PartClass.GetDefaultObject())
		{
			FName PartID = DefaultPart->GetPartID();
			if (const FNovaPartSpecRow* Row = Table->FindRow<FNovaPartSpecRow>(PartID, TEXT("")))
			{
				return Row->Watt;
			}
		}
		return 0.0f;
	};

	TotalWatt += GetWattFromPartClass(AssemblyData.LegsClass);
	TotalWatt += GetWattFromPartClass(AssemblyData.BodyClass);
	TotalWatt += GetWattFromPartClass(AssemblyData.WeaponClass); // 무기는 일단 1개분 비용만 합산

	return TotalWatt;
}

class ANovaUnit* UNovaUnitFactory::ExecuteUnitProduction(const FNovaUnitAssemblyData& AssemblyData,
                                                         const FTransform& SpawnTransform, int32 TeamID,
                                                         const FVector& RallyPoint,
                                                         int32 SlotIndex)
{
	// 실제 유닛 블루프린트 클래스 로드 (세팅에서 가져옴)
	const UNovaFactorySettings* FactorySettings = GetDefault<UNovaFactorySettings>();
	UClass* UnitClass = FactorySettings->DefaultUnitClass.LoadSynchronous();
	
	if (!UnitClass) 
	{
		// NOVA_SCREEN(Error, "[SpawnTrace] Execute: DefaultUnitClass is NULL in Settings! Fallback to ANovaUnit.");
		UnitClass = ANovaUnit::StaticClass();
	}
	
	// NOVA_SCREEN(Log, "[SpawnTrace] Execute: UnitClass Loaded - %s", *UnitClass->GetName());
	ANovaUnit* NewUnit = nullptr;

	// 1. 오브젝트 풀 서브시스템에서 유닛을 가져옵니다. (자동 활성화를 끕니다)
	if (UNovaObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UNovaObjectPoolSubsystem>())
	{
		NewUnit = Cast<ANovaUnit>(PoolSubsystem->SpawnFromPool(UnitClass, SpawnTransform, false));
		// NOVA_SCREEN(Log, "[SpawnTrace] Execute: SpawnFromPool result - %s", NewUnit ? *NewUnit->GetName() : TEXT("Failed/Null"));
	}
	else
	{
		// 풀이 없으면 기존 방식대로 스폰 (Fallback)
		NewUnit = GetWorld()->SpawnActorDeferred<ANovaUnit>(UnitClass, SpawnTransform,
		                                                    nullptr, nullptr,
		                                                    ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		// NOVA_SCREEN(Log, "[SpawnTrace] Execute: SpawnActorDeferred result - %s", NewUnit ? *NewUnit->GetName() : TEXT("Failed/Null"));
	}

	if (NewUnit)
	{
		// 2. 조립 데이터 및 정보 주입
		NewUnit->SetAssemblyData(AssemblyData);
		NewUnit->SetTeamID(TeamID);
		NewUnit->SetInitialRallyLocation(RallyPoint);

		// 유닛에게 생산 슬롯 번호 주입
		NewUnit->SetProductionSlotIndex(SlotIndex);

		// 3. 지연 스폰 또는 풀 부활 후의 마무리 작업 수행
		// 만약 풀에서 나온 유닛이라면 이미 BeginPlay가 호출되었을 것이므로 FinishSpawningActor가 작동하지 않음
		if (NewUnit->IsActorInitialized())
		{
			// 풀에서 나온 유닛은 강제로 재조립 및 스탯 초기화가 필요함
			NewUnit->OnSpawnFromPool_Implementation();
			
			// NOVA_SCREEN(Log, "[SpawnTrace] Execute: Initialized. Calling OnSpawnFromPool_Implementation.");
		}
		else
		{
			// 새로 생성된 유닛은 지연 스폰 마무리
			UGameplayStatics::FinishSpawningActor(NewUnit, SpawnTransform);
			
			// NOVA_SCREEN(Log, "[SpawnTrace] Execute: Not Initialized. Calling FinishSpawningActor.");
		}
		
		// NOVA_SCREEN(Log, "[SpawnTrace] Execute: Final Result - Spawn Completed Successfully.");
	}

	return NewUnit;
}

bool UNovaUnitFactory::CheckAndConsumeResources(class ANovaPlayerState* PS, float Cost)
{
	if (!PS)
	{
		NOVA_LOG(Error, "CheckAndConsumeResources failed: PS is null.");
		return false;
	}

	UNovaResourceComponent* ResourceComp = PS->FindComponentByClass<UNovaResourceComponent>();
	if (!ResourceComp)
	{
		NOVA_LOG(Error, "CheckAndConsumeResources failed: ResourceComp not found on PS (%s).", *PS->GetName());
		return false;
	}

	// 자원 및 인구수 체크 (유닛 생산 시 Watt 비용만 전달)
	bool bCanAfford = ResourceComp->CanAfford(Cost, 0.0f);
	bool bCanSpawn = ResourceComp->CanSpawnUnit(Cost);

	if (bCanAfford && bCanSpawn)
	{
		// 자원 소모
		ResourceComp->ConsumeResources(Cost, 0.0f);

		// 인구수 업데이트 (+1 유닛, +Cost 와트 합계)
		ResourceComp->UpdatePopulation(1.0f, Cost);

		return true;
	}
	
	// NOVA_LOG(Warning, "CheckAndConsumeResources failed: Afford(%d), Spawn(%d). CurrentWatt: %.1f", 
	// 	bCanAfford, bCanSpawn, PS->GetCurrentWatt());
	
	return false;
}
