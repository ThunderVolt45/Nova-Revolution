// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaObjectPoolSubsystem.h"
#include "Core/NovaInterfaces.h"
#include "Engine/World.h"
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
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
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

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 화면 밖 안전한 곳에 생성 (비활성화 상태로)
	FTransform SpawnTransform = FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, -10000.f));

	for (int32 i = 0; i < Count; ++i)
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
}