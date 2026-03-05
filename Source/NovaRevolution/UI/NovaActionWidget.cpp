// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/NovaActionWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"

void UNovaActionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 버튼(Slot_0 ~ Slot_11)만 수집합니다.
	SlotButtons.Empty();
	for (int32 i = 0; i < 12; i++)
	{
		if (UButton* Btn = Cast<UButton>(GetWidgetFromName(*FString::Printf(TEXT("Slot_%d"), i))))
		{
			SlotButtons.Add(Btn);
		}
	}

	// 2. 초기 상태 설정
	ClearAllSlots();
	
}

void UNovaActionWidget::ClearAllSlots()
{
	for (int32 i = 0; i < 12; i++)
	{
		SetSlotEmpty(i);
	}
}

void UNovaActionWidget::SetSlotAction(int32 Index)
{
	if (SlotButtons.IsValidIndex(Index))
	{
		SlotButtons[Index]->SetIsEnabled(true);
		// 활성화 시 색상 (예: 밝은 회색 또는 기본색)
		SlotButtons[Index]->SetBackgroundColor(FLinearColor::White);
	}
}

void UNovaActionWidget::SetSlotEmpty(int32 Index)
{
	if (SlotButtons.IsValidIndex(Index))
	{
		// 버튼 비활성화 (클릭 불가) 및 회색 채우기
		SlotButtons[Index]->SetIsEnabled(false);
		SlotButtons[Index]->SetBackgroundColor(EmptyColor);
	}
}