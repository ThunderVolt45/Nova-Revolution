// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovaDeckSlotWidget.generated.h"

class UImage;
class UButton;
class UTextureRenderTarget2D;

/**
 * 덱의 개별 슬롯 UI를 담당하는 위젯입니다.
 * RenderTarget을 받아와 슬롯 이미지를 표시하고, 클릭 이벤트를 처리합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaDeckSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/** 매니저가 이 UI 슬롯을 초기화할 때 호출합니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|UI|Deck")
	void InitSlot(int32 InSlotIndex, UTextureRenderTarget2D* RenderTarget);

protected:
	virtual void NativeConstruct() override;

	/** 버튼 클릭 시 로비 매니저에 슬롯 선택을 알립니다. */
	UFUNCTION()
	void OnSlotButtonClicked();
	
	// 배경 투명화 작업
	/** UMG 에디터에서 배경 투명화 등을 지원하는 M_DeckSlot_UI 머티리얼을 할당합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|UI|Deck")
	TObjectPtr<UMaterialInterface> SlotMaterialBase;

	/** 런타임에 렌더 타겟 파라미터를 조절하기 위한 동적 머티리얼 */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SlotDynamicMaterial;

protected:
	// UMG 에디터에서 'SlotImage'라는 이름의 Image 위젯과 짝지어집니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SlotImage;

	// UMG 에디터에서 'SlotButton'이라는 이름의 Button 위젯과 짝지어집니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SlotButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> SlotText;

	// 현재 UI 슬롯이 담당하는 인덱스 (0~9)
	int32 SlotIndex = 0;
	
};
