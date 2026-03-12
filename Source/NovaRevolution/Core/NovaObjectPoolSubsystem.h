// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/NovaAssemblyTypes.h"
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
	 * @param bAutoActivate 만약 false면 OnSpawnFromPool을 자동으로 호출하지 않습니다. (데이터 주입 후 직접 호출 필요 시 사용)
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|ObjectPool", meta = (DeterminesOutputType = "Class"))
	AActor* SpawnFromPool(TSubclassOf<AActor> Class, const FTransform& Transform, bool bAutoActivate = true);

	/**
	 * 위치와 회전을 지정하여 풀에서 꺼내거나 생성합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|ObjectPool", meta = (DeterminesOutputType = "Class"))
	AActor* SpawnFromPoolAtLocation(TSubclassOf<AActor> Class, FVector Location, FRotator Rotation = FRotator::ZeroRotator, bool bAutoActivate = true);

	/**
	 * 액터를 풀로 반환합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|ObjectPool")
	void ReturnToPool(AActor* Actor);

	/** 풀을 미리 채워둡니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|ObjectPool")
	void PreloadPool(TSubclassOf<AActor> Class, int32 Count);

	/** 
	 * 부품 데이터 테이블을 설정합니다. PreloadDecks 호출 전에 설정되어야 합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|ObjectPool")
	void SetPartDataTable(UDataTable* InDataTable) { PartDataTable = InDataTable; }

	/** 
	 * 여러 팀의 덱 정보를 분석하여 유닛 및 부품들을 통합적으로 미리 채워둡니다. 
	 * @param Decks 모든 팀의 덱 정보 맵
	 * @param MaxWattCap 팀별 최대 와트 총량
	 * @param MaxPopCap 팀별 최대 인구수 제한
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|ObjectPool")
	void PreloadDecks(const TMap<int32, FNovaDeckInfo>& Decks, float MaxWattCap, float MaxPopCap);

private:
	UPROPERTY()
	TMap<TSubclassOf<AActor>, FNovaObjectPool> ObjectPools;

	/** 부품 정보 조회를 위한 데이터 테이블 */
	UPROPERTY()
	TObjectPtr<UDataTable> PartDataTable;
};