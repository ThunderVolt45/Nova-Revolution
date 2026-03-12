// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NovaObjectPoolSubsystem.generated.h"

USTRUCT()
struct FNovaObjectPool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> AvailableActors;
};

/**
 * Nova Revolution 오브젝트 풀 서브시스템
 * INovaObjectPoolable 인터페이스를 구현하는 액터들을 풀링합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaObjectPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/**
	 * 특정 클래스의 액터를 풀에서 꺼내거나 새로 생성합니다.
	 * 클래스는 반드시 INovaObjectPoolable 인터페이스를 구현해야 합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|ObjectPool", meta = (DeterminesOutputType = "Class"))
	AActor* SpawnFromPool(TSubclassOf<AActor> Class, const FTransform& Transform);

	/**
	 * 위치와 회전을 지정하여 풀에서 꺼내거나 생성합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|ObjectPool", meta = (DeterminesOutputType = "Class"))
	AActor* SpawnFromPoolAtLocation(TSubclassOf<AActor> Class, FVector Location, FRotator Rotation = FRotator::ZeroRotator);

	/**
	 * 액터를 풀로 반환합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|ObjectPool")
	void ReturnToPool(AActor* Actor);

	/** 풀을 미리 채워둡니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|ObjectPool")
	void PreloadPool(TSubclassOf<AActor> Class, int32 Count);

private:
	UPROPERTY()
	TMap<TSubclassOf<AActor>, FNovaObjectPool> ObjectPools;
};