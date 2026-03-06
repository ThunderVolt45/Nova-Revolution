#include "Core/Production/NovaUnitFactory.h"

#include "Core/NovaGameMode.h"
#include "Core/NovaInterfaces.h"
#include "Core/NovaPlayerState.h"
#include "Core/NovaUnit.h"
#include "Core/NovaResourceComponent.h"
#include "Core/NovaPartData.h"
#include "Kismet/GameplayStatics.h"
#include "NovaRevolution.h"

bool UNovaUnitFactory::RequestSpawnUnitFromDeck(int32 SlotIndex, AActor* Spawner, const FVector& RallyPoint)
{
	// 1. 매개변수 유효성 검사 및 플레이어 상태 확인
   if (!Spawner) return false;

   // 생산을 요청한 주체(기지 등)를 소유한 플레이어의 PlayerState를 가져옵니다.
   APlayerController* PC = Spawner->GetNetOwningPlayer() ? Spawner->GetNetOwningPlayer()->GetPlayerController(GetWorld()) : nullptr;
   ANovaPlayerState* PS = PC ? PC->GetPlayerState<ANovaPlayerState>() : nullptr;

   // 2. 팀 식별자 결정 (프로젝트 표준 NovaTeam 상수 활용)
   // 기본값은 Team1(플레이어 팀)으로 설정하며, PS가 있다면 해당 인터페이스를 통해 실제 팀 ID를 가져옵니다.
   int32 TargetTeamID = NovaTeam::Team1;
   if (PS)
   {
         if (INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(PS))
         {
             TargetTeamID = TeamInterface->GetTeamID();
         }
   }
	
	// 3. GameMode에서 해당 팀의 덱 정보 획득
   ANovaGameMode* GM = Cast<ANovaGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
   if (!GM) return false;
	
	// TargetTeamID를 인자로 전달하여 해당 팀의 덱 데이터를 안전하게 불러옵니다.
    FNovaDeckInfo CurrentDeck = GM->GetPlayerDeck(TargetTeamID);

   // 4. 덱 슬롯 유효성 검사
   if (!CurrentDeck.Units.IsValidIndex(SlotIndex))
   {
         UE_LOG(LogTemp, Warning, TEXT("Factory: Invalid Deck Slot %d for Team %d"), SlotIndex, TargetTeamID);
         return false;
   }

   const FNovaUnitAssemblyData& TargetData = CurrentDeck.Units[SlotIndex];
	
	// 5. 자원 검사 및 소비 로직
   // 데이터 테이블을 참조하여 실제 유닛 생산 비용을 계산합니다.
   float ProductionCost = CalculateTotalWattCost(TargetData);
   if (!CheckAndConsumeResources(PS, ProductionCost))
   {
         UE_LOG(LogTemp, Warning, TEXT("Factory: Insufficient Resources (Cost: %.f) for Team %d"), ProductionCost, TargetTeamID);
         return false;
   }
	
	// 6. 스폰 위치 결정 (기지 전방 오프셋)
   FTransform SpawnTransform = Spawner->GetActorTransform();
   FVector SpawnLocation = SpawnTransform.GetLocation() + Spawner->GetActorForwardVector() * 350.f;
   SpawnTransform.SetLocation(SpawnLocation);

   // 7. 실제 유닛 즉시 스폰 및 데이터 주입
   ANovaUnit* NewUnit = ExecuteUnitProduction(TargetData, SpawnTransform, TargetTeamID, RallyPoint);

   if (NewUnit)
   {
        UE_LOG(LogTemp, Log, TEXT("Factory: Unit '%s' (Cost: %.f) successfully spawned for Team %d."), *TargetData.UnitName, ProductionCost, TargetTeamID);
         return true;
   }

    return false;
}

float UNovaUnitFactory::CalculateTotalWattCost(const FNovaUnitAssemblyData& AssemblyData) const
{
	float TotalWatt = 0.0f;

	// 데이터 테이블이 유효한지 확인
	UDataTable* Table = PartSpecDataTable.Get();
	if (!Table)
	{
		// 런타임에 데이터 테이블이 로드되지 않았다면 기본 경로에서 시도
		Table = LoadObject<UDataTable>(nullptr, TEXT("/Game/_BP/Data/DT_PartSpecData.DT_PartSpecData"));
		if (!Table)
		{
			NOVA_LOG(Error, "UNovaUnitFactory: PartSpecDataTable is NOT found!");
			return 100.0f; // 최후의 보루로 기본 비용 반환
		}
	}

	auto AddWattFromPartID = [&](const FName& PartID) {
		if (PartID.IsNone()) return;
		if (const FNovaPartSpecRow* Row = Table->FindRow<FNovaPartSpecRow>(PartID, TEXT("")))
		{
			TotalWatt += Row->Watt;
		}
	};

	// 다리, 몸통 비용 합산
	AddWattFromPartID(AssemblyData.LegsPartID);
	AddWattFromPartID(AssemblyData.BodyPartID);

	// 무기 슬롯 비용 합산
	for (const FNovaWeaponPartSlot& Slot : AssemblyData.WeaponSlots)
	{
		AddWattFromPartID(Slot.PartID);
	}

	return TotalWatt;
}

class ANovaUnit* UNovaUnitFactory::ExecuteUnitProduction(const FNovaUnitAssemblyData& AssemblyData, const FTransform& SpawnTransform, int32 TeamID, const FVector& RallyPoint)
{
	// 지연 스폰을 사용하여 BeginPlay 이전에 데이터 주입
	ANovaUnit* NewUnit = GetWorld()->SpawnActorDeferred<ANovaUnit>(ANovaUnit::StaticClass(), SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (NewUnit)
	{
		// 데이터 주입
		NewUnit->SetAssemblyData(AssemblyData);
		NewUnit->SetTeamID(TeamID);
		NewUnit->SetInitialRallyLocation(RallyPoint); // 랠리 포인트 설정
		
		UGameplayStatics::FinishSpawningActor(NewUnit, SpawnTransform);
	}
	
	return NewUnit;
}

bool UNovaUnitFactory::CheckAndConsumeResources(class ANovaPlayerState* PS, float Cost)
{
	if (!PS) return false;

	UNovaResourceComponent* ResourceComp = PS->FindComponentByClass<UNovaResourceComponent>();
	if (!ResourceComp) return false;

	// 자원 및 인구수 체크 (유닛 생산 시 Watt 비용만 전달)
	if (ResourceComp->CanAfford(Cost, 0.0f) && ResourceComp->CanSpawnUnit(Cost))
	{
		// 자원 소모
		ResourceComp->ConsumeResources(Cost, 0.0f);
		
		// 인구수 업데이트 (+1 유닛, +Cost 와트 합계)
		ResourceComp->UpdatePopulation(1.0f, Cost);
		
		return true;
	}

	return false;
}
