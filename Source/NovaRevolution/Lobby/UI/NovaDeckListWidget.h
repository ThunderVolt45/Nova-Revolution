// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovaDeckListWidget.generated.h"

class UUniformGridPanel;
class UNovaDeckSlotWidget;

/**
 * 10개의 덱 슬롯을 관리하고 화면에 나열하는 컨테이너 위젯입니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaDeckListWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/** 로비 매니저가 이 위젯을 생성한 직후 호출하여 10개의 슬롯을 초기화합니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|UI|Deck")
	void InitializeDeckList();

protected:
	virtual void NativeConstruct() override;

	// --- UMG 에디터에서 직접 배치할 10개의 슬롯 ---
	// 위젯 블루프린트에서 반드시 아래 이름과 동일하게 배치해야 합니다.
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UNovaDeckSlotWidget> DeckSlot_0;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UNovaDeckSlotWidget> DeckSlot_1;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UNovaDeckSlotWidget> DeckSlot_2;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UNovaDeckSlotWidget> DeckSlot_3;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UNovaDeckSlotWidget> DeckSlot_4;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UNovaDeckSlotWidget> DeckSlot_5;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UNovaDeckSlotWidget> DeckSlot_6;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UNovaDeckSlotWidget> DeckSlot_7;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UNovaDeckSlotWidget> DeckSlot_8;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UNovaDeckSlotWidget> DeckSlot_9;

private:
	// 코드로 순회하기 쉽도록 초기화 시 배열에 담아둡니다.
	UPROPERTY()
	TArray<TObjectPtr<UNovaDeckSlotWidget>> SlotWidgets;
	
};
