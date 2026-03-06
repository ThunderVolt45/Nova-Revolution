#include "Core/Production/NovaUnitFactory.h"

#include "Core/NovaGameMode.h"
#include "Core/NovaInterfaces.h"
#include "Core/NovaPlayerState.h"
#include "Core/NovaUnit.h"
#include "Core/NovaResourceComponent.h"
#include "Core/NovaPartData.h"
#include "Kismet/GameplayStatics.h"
#include "NovaRevolution.h"
#include "Core/NovaPart.h"

bool UNovaUnitFactory::RequestSpawnUnitFromDeck(int32 SlotIndex, AActor* Spawner, const FVector& RallyPoint)
{
	// 1. 매개변수 유효성 검사
   if (!Spawner) return false;

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
       for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
       {
           if (APlayerController* PC = Iterator->Get())
           {
               if (ANovaPlayerState* TempPS = PC->GetPlayerState<ANovaPlayerState>())
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
         UE_LOG(LogTemp, Warning, TEXT("Factory: Invalid Deck Slot %d for Team %d"), SlotIndex, TargetTeamID);
         return false;
   }

   const FNovaUnitAssemblyData& TargetData = CurrentDeck.Units[SlotIndex];

	// 6. 자원 검사 및 소비 로직
   // 데이터 테이블을 참조하여 실제 유닛 생산 비용을 계산합니다.
   float ProductionCost = CalculateTotalWattCost(TargetData);
   if (!CheckAndConsumeResources(PS, ProductionCost))
   {
         UE_LOG(LogTemp, Warning, TEXT("Factory: Insufficient Resources (Cost: %.f) for Team %d"), ProductionCost, TargetTeamID);
         return false;
   }

	// 7. 스폰 위치 결정 (기지 전방 오프셋)
   FTransform SpawnTransform = Spawner->GetActorTransform();
   FVector SpawnLocation = SpawnTransform.GetLocation() + Spawner->GetActorForwardVector() * 350.f;
   SpawnTransform.SetLocation(SpawnLocation);
   // 유닛의 크기가 기지(Spawner)의 스케일을 따라가지 않도록 (1, 1, 1)로 초기화
   SpawnTransform.SetScale3D(FVector::OneVector);

   // 8. 실제 유닛 즉시 스폰 및 데이터 주입
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

	UDataTable* Table = PartSpecDataTable.Get();
	if (!Table)
	{
		Table = LoadObject<UDataTable>(nullptr, TEXT("/Game/_BP/Data/DT_PartSpecData.DT_PartSpecData"));
		if (!Table) return 100.0f;
	}

	auto GetWattFromPartClass = [&](TSubclassOf<ANovaPart> PartClass) -> float {
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

class ANovaUnit* UNovaUnitFactory::ExecuteUnitProduction(const FNovaUnitAssemblyData& AssemblyData, const FTransform& SpawnTransform, int32 TeamID, const FVector& RallyPoint)
{
    // 실제 유닛 블루프린트 클래스 로드 (프로젝트 경로에 맞춰 수정 필요할 수 있음)
    UClass* UnitClass = LoadClass<ANovaUnit>(nullptr, TEXT("/Game/_BP/Units/BP_NovaUnitBase.BP_NovaUnitBase_C"));
    if (!UnitClass) UnitClass = ANovaUnit::StaticClass();

	// 지연 스폰을 사용하여 BeginPlay 이전에 데이터 주입
	ANovaUnit* NewUnit = GetWorld()->SpawnActorDeferred<ANovaUnit>(UnitClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (NewUnit)
	{
		NewUnit->SetAssemblyData(AssemblyData);
		NewUnit->SetTeamID(TeamID);
		NewUnit->SetInitialRallyLocation(RallyPoint);
		
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
