// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovaUnitInfoWidget.generated.h"

// --- 전방 선언 ---
class UTextBlock;
class UProgressBar;
class UWidgetSwitcher;
class ANovaUnit;
class ANovaBase;
class UNovaMultiSelectionWidget;

/**
 * 유닛 선택 시 상세 정보를 표시하기 위한 위젯
 */
UCLASS()
class NOVAREVOLUTION_API UNovaUnitInfoWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// 플레이어 컨트롤러의 선택 변경 델리게이트에 바인딩될 함수
	UFUNCTION()
	void HandleSelectionChanged(const TArray<AActor*>& SelectedActors);

	// 유닛의 속성이 변경되었을 때 호출될 함수 (실시간 갱신용)
	UFUNCTION()
	void OnUnitAttributeChanged(ANovaUnit* Unit);
	
	// 기지의 속성이 변경되었을 때 호출될 함수 (실시간 갱신용)
	UFUNCTION()
	void OnBaseAttributeChanged(ANovaBase* Base);

	// UI 갱신 로직
	void UpdateUnitInfo(ANovaUnit* Unit);
	void UpdateBaseInfo(ANovaBase* Base);

	// --- UI 바인딩 ---
	/** 단일/다중 선택 UI 전환을 위한 스위처 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher;

	/** 다중 선택 정보 표시 위젯 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNovaMultiSelectionWidget> MultiSelectionWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> UnitNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AttackText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DefenseText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SpeedText;

private:
	// 현재 감시 중인 유닛 (델리게이트 해제용)
	UPROPERTY()
	TObjectPtr<ANovaUnit> CurrentObservedUnit;
	
	// 현재 감시 중인 기지 (델리게이트 해제용)
	UPROPERTY()
	TObjectPtr<ANovaBase> CurrentObservedBase;
};
