// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/NovaMultiSelectionWidget.h"
#include "Components/UniformGridPanel.h"
#include "UI/NovaSelectionSlotWidget.h"
#include "Core/NovaUnit.h"

void UNovaMultiSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 위젯 생성 시 20개의 슬롯을 미리 만들어둡니다.
	InitializeFixedSlots();
}

void UNovaMultiSelectionWidget::InitializeFixedSlots()
{
	// 이미 생성되어 있거나 필요한 컴포넌트가 없다면 리턴
	if (!UnitGrid || !SlotWidgetClass || FixedSlotPool.Num() > 0) return;

	// 기존 내용 청소
	UnitGrid->ClearChildren();
	FixedSlotPool.Empty();

	// 4행 5열(총 20개)의 슬롯을 미리 생성하여 그리드에 고정 배치합니다.
	for (int32 i = 0; i < 20; ++i)
	{
		UNovaSelectionSlotWidget* NewSlot = CreateWidget<UNovaSelectionSlotWidget>(this, SlotWidgetClass);
		if (NewSlot)
		{
			// 그리드 좌표 계산 (5개씩 끊어서 다음 행으로)
			int32 Row = i / 5;
			int32 Column = i % 5;
			
			UnitGrid->AddChildToUniformGrid(NewSlot, Row, Column);
			
			// Hidden은 위젯을 숨기지만 레이아웃 공간은 그대로 차지합니다.
			NewSlot->SetVisibility(ESlateVisibility::Hidden);
			
			FixedSlotPool.Add(NewSlot);
		}
	}
}

void UNovaMultiSelectionWidget::UpdateSelection(const TArray<AActor*>& SelectedUnits)
{
	// 혹시나 풀이 비어있다면 다시 생성 시도
	if (FixedSlotPool.Num() == 0) InitializeFixedSlots();

	// 20개의 고정 슬롯을 순회하며 데이터만 갈아 끼웁니다.
	for (int32 i = 0; i < 20; ++i)
	{
		// 선택된 유닛이 있는 경우
		if (i < SelectedUnits.Num())
		{
			if (ANovaUnit* Unit = Cast<ANovaUnit>(SelectedUnits[i]))
			{
				// 데이터 업데이트 후 보이게 설정 (델리게이트 구독 포함)
				FixedSlotPool[i]->UpdateSlot(Unit, i);
				FixedSlotPool[i]->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				// 유효하지 않으면 투명하게 숨기고 델리게이트 해제
				FixedSlotPool[i]->ClearSlot();
				FixedSlotPool[i]->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		// 선택된 유닛 범위를 벗어난 경우 (빈 슬롯)
		else
		{
			// 해당 칸은 투명하게 숨기고 기존 감시 중인 델리게이트 해제
			FixedSlotPool[i]->ClearSlot();
			FixedSlotPool[i]->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}
