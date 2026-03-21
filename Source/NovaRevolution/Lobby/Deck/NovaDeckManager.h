// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/NovaAssemblyTypes.h"
#include "NovaDeckManager.generated.h"

/**
 * ANovaDeckManager
 * 격납고(Hangar) 전체의 덱 슬롯들을 관리하고, 각 슬롯 위에 프리뷰 유닛을 스폰/갱신하는 관리자 액터입니다.
 * 로비 매니저의 명령을 받아 실제 월드 상의 시각적 배치를 담당합니다.
 */
UCLASS()
class NOVAREVOLUTION_API ANovaDeckManager : public AActor
{
	GENERATED_BODY()

public:
	ANovaDeckManager();

public:
	/** * 특정 슬롯의 유닛 외형을 갱신합니다. 
	 * 해당 슬롯에 유닛이 없다면 새로 스폰하고, 있다면 조립 데이터만 업데이트합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Deck")
	void UpdateSlotUnit(int32 SlotIndex, const FNovaUnitAssemblyData& UnitData);

	/** 특정 슬롯에 배치된 유닛을 제거하고 풀로 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Deck")
	void ClearSlotUnit(int32 SlotIndex);

protected:
	/** * 레벨에 물리적으로 배치된 10개의 슬롯 액터 리스트입니다. 
	 * 에디터의 디테일 패널에서 직접 드래그하여 할당합니다. 
	 */
	UPROPERTY(EditInstanceOnly, Category = "Nova|Deck")
	TArray<TObjectPtr<class ANovaDeckSlot>> DeckSlots;

	/** * 슬롯 인덱스별로 현재 월드에 스폰되어 있는 프리뷰 유닛들의 참조를 관리합니다. 
	 * Key: SlotIndex, Value: PreviewUnit Actor
	 */
	UPROPERTY()
	TMap<int32, TObjectPtr<class APreviewUnit>> SpawnedUnits;

	/** 실제로 스폰할 프리뷰 유닛의 블루프린트 클래스 (BP_PreviewUnit) */
	UPROPERTY(EditAnywhere, Category = "Nova|Deck")
	TSubclassOf<class APreviewUnit> PreviewUnitClass;
};
