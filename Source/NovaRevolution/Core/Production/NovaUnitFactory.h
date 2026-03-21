// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/NovaAssemblyTypes.h"
#include "Core/NovaTypes.h"
#include "NovaUnitFactory.generated.h"

/**
  * 유닛 생산을 전담하는 월드 서브시스템  */
UCLASS()
class NOVAREVOLUTION_API UNovaUnitFactory : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
      * 덱 슬롯 번호를 이용한 유닛 생산 요청
      * @param SlotIndex: GameMode의 덱 정보 중 생산할 슬롯 번호 (0~9)
      * @param Spawner: 생산 주체 (기지 액터 등)
      * @param RallyPoint: 생산 후 이동할 목표 지점
      */
	UFUNCTION(BlueprintCallable, Category = "Nova|Factory")
	bool RequestSpawnUnitFromDeck(int32 SlotIndex, AActor* Spawner, const FVector& RallyPoint);

	/** 조립 데이터를 기반으로 유닛의 총 와트 비용을 계산합니다. */
	UFUNCTION(BlueprintPure, Category = "Nova|Factory")
	float CalculateTotalWattCost(const FNovaUnitAssemblyData& AssemblyData) const;

private:
	/** 실제 유닛 생성 및 조립 데이터 주입 */
	class ANovaUnit* ExecuteUnitProduction(const FNovaUnitAssemblyData& AssemblyData, const FTransform& SpawnTransform,
	                                       int32 TeamID, const FVector& RallyPoint, int32 SlotIndex);
	/** 자원(Watt) 확인 및 차감 로직 */
	bool CheckAndConsumeResources(class ANovaPlayerState* PS, float Cost);

};
