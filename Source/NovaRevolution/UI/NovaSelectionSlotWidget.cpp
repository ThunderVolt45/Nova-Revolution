// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/NovaSelectionSlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Core/NovaUnit.h"
#include "GAS/NovaAttributeSet.h"

void UNovaSelectionSlotWidget::UpdateSlot(ANovaUnit* Unit, int32 SlotIndex)
{
	// 1. 기존 유닛의 델리게이트 구독 해제
	ClearSlot();

	if (!Unit) return;

	ObservedUnit = Unit;

	// 2. 새로운 유닛의 델리게이트 구독 (실시간 체력바 업데이트용)
	ObservedUnit->OnUnitAttributeChanged.AddUniqueDynamic(this, &ThisClass::OnAttributeChanged);

	// 3. 슬롯 이름 설정 (생산된 덱 슬롯 번호 사용)
	if (SlotNameText)
	{
		// 유닛이 기억하고 있는 생산 슬롯 번호 (0~9)를 가져와 +1 하여 표시
		int32 ProdIndex = Unit->GetProductionSlotIndex();
		
		// -1인 경우(미배치 유닛 등) 0으로 보정하여 BU0으로 표시
		int32 DisplayIndex = (ProdIndex != -1) ? (ProdIndex + 1) : 0;
		
		SlotNameText->SetText(FText::Format(FText::FromString("BU{0}"), FText::AsNumber(DisplayIndex)));
	}

	// 4. 초기 UI 갱신 호출
	OnAttributeChanged(ObservedUnit);
}

void UNovaSelectionSlotWidget::ClearSlot()
{
	// 기존 유닛의 델리게이트 구독 해제
	if (ObservedUnit)
	{
		ObservedUnit->OnUnitAttributeChanged.RemoveDynamic(this, &ThisClass::OnAttributeChanged);
		ObservedUnit = nullptr;
	}
}

void UNovaSelectionSlotWidget::OnAttributeChanged(ANovaUnit* Unit)
{
	// 현재 감시 중인 유닛과 일치하는 경우에만 UI를 갱신합니다.
	if (!Unit || Unit != ObservedUnit) return;

	// 체력바 및 색상 설정
	if (const UNovaAttributeSet* AS = Unit->GetAttributeSet())
	{
		const float CurHP = AS->GetHealth();
		const float MaxHP = AS->GetMaxHealth();
		const float HPPercent = (MaxHP > 0.f) ? (CurHP / MaxHP) : 0.f;

		if (HealthBar)
		{
			HealthBar->SetPercent(HPPercent);

			// 비율에 따른 색상 결정 (100% 초록, 50%이하 노랑, 20%이하 빨강)
			FLinearColor StatusColor = FLinearColor::Green;
			if (HPPercent <= 0.2f)
			{
				StatusColor = FLinearColor::Red;
			}
			else if (HPPercent <= 0.5f)
			{
				StatusColor = FLinearColor::Yellow;
			}

			HealthBar->SetFillColorAndOpacity(StatusColor);
		}
	}
}
