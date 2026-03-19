// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/UI/NovaUnitPartSpecEntryWidget.h"
#include "Components/TextBlock.h"

void UNovaUnitPartSpecEntryWidget::SetStatData(const FString& InLabel, float InValue, bool bIsInteger)
{
	// 1. 항목 이름 설정 (예: "체력", "이동 속도")
	if (PartSpec_Label)
	{
		PartSpec_Label->SetText(FText::FromString(InLabel));
	}

	// 2. 수치 데이터 설정 및 포맷팅
	if (PartSpec_Value)
	{
		FText FormattedValue;

		if (bIsInteger)
		{
			// 정수형 처리 (예: 500.0 -> 500)
			// 반올림을 통해 데이터의 정확성을 유지합니다.
			FormattedValue = FText::AsNumber(FMath::RoundToInt(InValue));
		}
		else
		{
			// 소수점 처리 (이동 속도, 공격 속도 등)
			// 기본적으로 소수점 1자리까지 고정해서 표시하여 UI 정렬을 깔끔하게 유지합니다.
			FNumberFormattingOptions Options;
			Options.MaximumFractionalDigits = 1;
			Options.MinimumFractionalDigits = 1;
			FormattedValue = FText::AsNumber(InValue, &Options);
		}

		PartSpec_Value->SetText(FormattedValue);
	}
}

void UNovaUnitPartSpecEntryWidget::SetStatText(const FString& InLabel, const FString& InValueText)
{
	if (PartSpec_Label) PartSpec_Label->SetText(FText::FromString(InLabel));
	if (PartSpec_Value) PartSpec_Value->SetText(FText::FromString(InValueText));
}
