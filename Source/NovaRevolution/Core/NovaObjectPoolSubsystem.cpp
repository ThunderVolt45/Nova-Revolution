// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaObjectPoolSubsystem.h"
#include "Core/NovaInterfaces.h"
#include "Core/NovaPart.h"
#include "Core/NovaUnit.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "NovaRevolution.h"

void UNovaObjectPoolSubsystem::Deinitialize()
{
	ObjectPools.Empty();
	Super::Deinitialize();
}

AActor* UNovaObjectPoolSubsystem::SpawnFromPool(TSubclassOf<AActor> Class, const FTransform& Transform, bool bAutoActivate)
{
	if (!Class)
	{
		NOVA_LOG(Warning, "SpawnFromPool called with null Class");
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World) return nullptr;

	// 클래스가 INovaObjectPoolable을 구현하는지 확인
	if (!Class->ImplementsInterface(UNovaObjectPoolable::StaticClass()))
	{
		NOVA_LOG(Warning, "Class %s does not implement INovaObjectPoolable!", *Class->GetName());
		return nullptr;
	}

	FNovaObjectPool& Pool = ObjectPools.FindOrAdd(Class);
	
	AActor* SpawnedActor = nullptr;

	// 풀에 사용 가능한 액터가 있는지 확인 (유효성 검사 포함)
	while (Pool.AvailableActors.Num() > 0)
	{
		SpawnedActor = Pool.AvailableActors.Pop();
		if (IsValid(SpawnedActor))
		{
			break;
		}
		SpawnedActor = nullptr; // 무효한 포인터면 다시 null로
	}

	if (!SpawnedActor)
	{
		// 풀에 가용한 액터가 없으면 새로 생성
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnedActor = World->SpawnActor<AActor>(Class, Transform, SpawnParams);
	}
	else
	{
		// 풀에서 꺼낸 액터 위치, 회전 적용 및 활성화
		SpawnedActor->SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
		SpawnedActor->SetActorHiddenInGame(false);
		SpawnedActor->SetActorEnableCollision(true);
		SpawnedActor->SetActorTickEnabled(true);
	}

	if (SpawnedActor && bAutoActivate)
	{
		INovaObjectPoolable::Execute_OnSpawnFromPool(SpawnedActor);
	}

	return SpawnedActor;
}

AActor* UNovaObjectPoolSubsystem::SpawnFromPoolAtLocation(TSubclassOf<AActor> Class, FVector Location, FRotator Rotation, bool bAutoActivate)
{
	return SpawnFromPool(Class, FTransform(Rotation, Location), bAutoActivate);
}

void UNovaObjectPoolSubsystem::ReturnToPool(AActor* Actor)
{
	if (!IsValid(Actor)) return;

	TSubclassOf<AActor> Class = Actor->GetClass();

	if (!Class->ImplementsInterface(UNovaObjectPoolable::StaticClass()))
	{
		NOVA_LOG(Warning, "Actor %s does not implement INovaObjectPoolable! Destroying it instead.", *Actor->GetName());
		Actor->Destroy();
		return;
	}

	// 풀에 이미 존재하는지 확인 (중복 반환 방지)
	FNovaObjectPool& Pool = ObjectPools.FindOrAdd(Class);
	if (Pool.AvailableActors.Contains(Actor))
	{
		return;
	}

	// 인터페이스 함수 호출 (상태 리셋)
	INovaObjectPoolable::Execute_OnReturnToPool(Actor);

	// 액터 비활성화
	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);

	// 풀에 추가
	Pool.AvailableActors.Add(Actor);
}

void UNovaObjectPoolSubsystem::PreloadPool(TSubclassOf<AActor> Class, int32 Count)
{
	if (!Class || Count <= 0) return;
	
	UWorld* World = GetWorld();
	if (!World) return;

	if (!Class->ImplementsInterface(UNovaObjectPoolable::StaticClass()))
	{
		NOVA_LOG(Warning, "Class %s does not implement INovaObjectPoolable!", *Class->GetName());
		return;
	}

	FNovaObjectPool& Pool = ObjectPools.FindOrAdd(Class);

	// 이미 풀에 있는 개수 확인
	int32 CurrentCount = Pool.AvailableActors.Num();
	int32 NeedToSpawn = Count - CurrentCount;

	if (NeedToSpawn <= 0) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 화면 밖 안전한 곳에 생성 (비활성화 상태로)
	FTransform SpawnTransform = FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, -10000.f));

	for (int32 i = 0; i < NeedToSpawn; ++i)
	{
		AActor* SpawnedActor = World->SpawnActor<AActor>(Class, SpawnTransform, SpawnParams);
		if (SpawnedActor)
		{
			// 생성 직후 OnReturnToPool을 호출하여 리셋 및 비활성화 상태로 만듦
			INovaObjectPoolable::Execute_OnReturnToPool(SpawnedActor);
			
			SpawnedActor->SetActorHiddenInGame(true);
			SpawnedActor->SetActorEnableCollision(false);
			SpawnedActor->SetActorTickEnabled(false);

			Pool.AvailableActors.Add(SpawnedActor);
		}
	}
	
	NOVA_LOG(Log, "Preloaded %d actors of class %s (Total in pool: %d)", NeedToSpawn, *Class->GetName(), Pool.AvailableActors.Num());
}

void UNovaObjectPoolSubsystem::PreloadDecks(const TMap<int32, FNovaDeckInfo>& Decks, float MaxWattCap, float MaxPopCap)
{
	if (Decks.Num() == 0 || MaxWattCap <= 0 || MaxPopCap <= 0) return;

	// 1. 유닛 베이스 클래스 로드
	UClass* UnitBaseClass = LoadClass<ANovaUnit>(nullptr, TEXT("/Game/_BP/Units/BP_NovaUnitBase.BP_NovaUnitBase_C"));
	if (!UnitBaseClass) UnitBaseClass = ANovaUnit::StaticClass();

	// 모든 플레이어가 최대 인구수만큼 유닛을 뽑을 수 있으므로 통합 수량 계산
	int32 TotalBaseUnits = FMath::CeilToInt(MaxPopCap * Decks.Num());
	PreloadPool(UnitBaseClass, TotalBaseUnits);

	// 2. 부품별 최소 와트 및 최대 무기 소켓 수 분석
	// 각 팀별로 해당 부품이 필요할 수 있는 최대 수량을 합산하기 위해 맵 사용
	TMap<TSubclassOf<ANovaPart>, int32> IntegratedPartDemandMap;

	if (!PartDataTable)
	{
		NOVA_LOG(Warning, "PreloadDecks: PartDataTable is not set! Call SetPartDataTable first.");
		return;
	}

	// 각 부품의 와트 값을 구하는 람다 함수
	auto GetWatt = [&](TSubclassOf<ANovaPart> PartClass) -> float {
		if (!PartClass) return 0.0f;
		ANovaPart* PartCDO = Cast<ANovaPart>(PartClass->GetDefaultObject());
		if (!PartCDO) return 0.0f;
		FName PartID = PartCDO->GetPartID();
		FNovaPartSpecRow* Row = PartDataTable->FindRow<FNovaPartSpecRow>(PartID, TEXT("PreloadDecks"));
		return Row ? Row->Watt : 0.0f;
	};

	ANovaUnit* UnitCDO = Cast<ANovaUnit>(UnitBaseClass->GetDefaultObject());

	// 각 팀 별로 부품 요구량 계산
	for (auto& DeckPair : Decks)
	{
		const FNovaDeckInfo& Deck = DeckPair.Value;
		
		// 이 팀에서 각 부품별로 필요한 최소 와트 유닛 찾기
		TMap<TSubclassOf<ANovaPart>, float> TeamPartMinWattMap;
		TMap<TSubclassOf<ANovaPart>, int32> TeamPartMaxSocketMap;

		for (const FNovaUnitAssemblyData& UnitData : Deck.Units)
		{
			if (!UnitData.LegsClass || !UnitData.BodyClass || !UnitData.WeaponClass) continue;

			float UnitTotalWatt = GetWatt(UnitData.LegsClass) + GetWatt(UnitData.BodyClass) + GetWatt(UnitData.WeaponClass);
			if (UnitTotalWatt <= 0) UnitTotalWatt = 100.0f;

			auto UpdateTeamStats = [&](TSubclassOf<ANovaPart> PartClass, int32 Sockets = 1) {
				if (!PartClass) return;
				float* MinWatt = TeamPartMinWattMap.Find(PartClass);
				if (!MinWatt || UnitTotalWatt < *MinWatt) TeamPartMinWattMap.Add(PartClass, UnitTotalWatt);
				
				int32* MaxSockets = TeamPartMaxSocketMap.Find(PartClass);
				if (!MaxSockets || Sockets > *MaxSockets) TeamPartMaxSocketMap.Add(PartClass, Sockets);
			};

			int32 SocketCount = UnitCDO ? UnitCDO->GetWeaponSocketNames().Num() : 2;
			UpdateTeamStats(UnitData.LegsClass);
			UpdateTeamStats(UnitData.BodyClass);
			UpdateTeamStats(UnitData.WeaponClass, SocketCount);
		}

		// 이 팀의 요구량을 전체 맵에 합산
		for (auto& MinWattPair : TeamPartMinWattMap)
		{
			TSubclassOf<ANovaPart> PartClass = MinWattPair.Key;
			float MinWatt = MinWattPair.Value;
			int32 SocketCount = TeamPartMaxSocketMap.Contains(PartClass) ? TeamPartMaxSocketMap[PartClass] : 1;

			int32 CountForThisTeam = FMath::Min(FMath::CeilToInt(MaxWattCap / MinWatt), FMath::CeilToInt(MaxPopCap));
			IntegratedPartDemandMap.FindOrAdd(PartClass) += (CountForThisTeam * SocketCount);
		}
	}

	// 3. 통합된 요구량으로 Preload 수행
	for (auto& Pair : IntegratedPartDemandMap)
	{
		// 최대 인구수 제한의 4배(최대 무기 소켓 고려) 정도로 캡을 씌워 안전장치 마련
		int32 FinalCount = FMath::Clamp(Pair.Value, 1, FMath::CeilToInt(MaxPopCap * 4.0f * Decks.Num()));
		PreloadPool(Pair.Key, FinalCount);
	}
}