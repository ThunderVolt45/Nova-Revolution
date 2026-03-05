// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovaActionWidget.generated.h"

/**
 * 12개의 액션 슬롯을 관리하는 UI 프레임워크 클래스
 */
UCLASS()
class NOVAREVOLUTION_API UNovaActionWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// --- WBP 아이템을 담아둘 배열 ---
	// WBP에서 버튼은 Slot_0 ~ Slot_11, 아이콘은 Icon_0 ~ Icon_11로 이름을 지어야 합니다.
	UPROPERTY()
	TArray<class UButton*> SlotButtons;

	// UPROPERTY()
	// TArray<class UImage*> SlotIcons;

public:
	/** 모든 슬롯을 비어있는 회색 상태로 초기화 */
	UFUNCTION(BlueprintCallable, Category = "Nova|UI")
	void ClearAllSlots();

	/** 특정 슬롯에 아이콘을 배치하고 활성화 */
	UFUNCTION(BlueprintCallable, Category = "Nova|UI")
	void SetSlotAction(int32 Index);

	/** 특정 슬롯을 다시 비우기 */
	UFUNCTION(BlueprintCallable, Category = "Nova|UI")
	void SetSlotEmpty(int32 Index);

private:
	// 비활성/활성 시 공통으로 사용할 색상 정의
	const FLinearColor EmptyColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.8f); // 어두운 회색
	const FLinearColor ActiveColor = FLinearColor::White;
};
