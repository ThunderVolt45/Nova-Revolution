// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovaSelectionSlotWidget.generated.h"

class UTextBlock;
class UProgressBar;
class ANovaUnit;

/**
 * 다중 유닛 선택 시 개별 유닛의 간략 정보를 표시하는 슬롯 위젯
 * 유닛의 속성 변경 델리게이트를 구독하여 실시간으로 체력바를 갱신합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaSelectionSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 슬롯의 정보를 업데이트하고 유닛의 델리게이트를 구독합니다. */
	void UpdateSlot(ANovaUnit* Unit, int32 SlotIndex);

	/** 슬롯을 비우고 감시 중인 유닛의 델리게이트 구독을 해제합니다. */
	void ClearSlot();

protected:
	/** 유닛의 속성이 변경되었을 때 호출되는 콜백 함수 */
	UFUNCTION()
	void OnAttributeChanged(ANovaUnit* Unit);

	/** "BU1", "BU2" 등의 슬롯 번호 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SlotNameText;

	/** 유닛의 현재 체력바 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

private:
	/** 현재 이 슬롯이 감시 중인 유닛 (델리게이트 해제용) */
	UPROPERTY()
	TObjectPtr<ANovaUnit> ObservedUnit;
};
