// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovaMultiSelectionWidget.generated.h"

class UUniformGridPanel;
class UNovaSelectionSlotWidget;

/**
 * 2마리 이상의 유닛이 선택되었을 때, 4x5 그리드 형태로 정보를 표시하는 위젯
 * 미리 생성된 20개의 슬롯을 사용하여 레이아웃을 고정하고 데이터만 갱신합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaMultiSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 선택된 유닛 리스트로 그리드를 업데이트합니다. */
	void UpdateSelection(const TArray<AActor*>& SelectedUnits);

protected:
	virtual void NativeConstruct() override;

	/** 20개의 슬롯을 미리 생성하여 그리드에 고정 배치합니다. */
	void InitializeFixedSlots();

	/** 슬롯으로 사용할 위젯 클래스 (BP에서 할당) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|UI")
	TSubclassOf<UNovaSelectionSlotWidget> SlotWidgetClass;

	/** 4x5 배치를 위한 유니폼 그리드 패널 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> UnitGrid;

private:
	/** 미리 생성된 고정 슬롯들을 관리하는 배열 */
	UPROPERTY()
	TArray<TObjectPtr<UNovaSelectionSlotWidget>> FixedSlotPool;
};
