// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/NovaUnitInfoWidget.h"
#include "Player/NovaPlayerController.h"
#include "Core/NovaUnit.h"
#include "GAS/NovaAttributeSet.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/WidgetSwitcher.h"
#include "UI/NovaMultiSelectionWidget.h"

void UNovaUnitInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetOwningPlayer()))
	{
		// 플레이어 컨트롤러의 선택 변경 델리게이트 구독
		PC->OnSelectionChanged.RemoveDynamic(this, &ThisClass::HandleSelectionChanged);
		PC->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleSelectionChanged);

		// 초기 상태 동기화 (이미 선택된 유닛이 있을 경우 대비)
		HandleSelectionChanged(PC->GetSelectedUnits());
	}
}

void UNovaUnitInfoWidget::HandleSelectionChanged(const TArray<AActor*>& SelectedActors)
{
	// 기존에 감시하던 유닛의 델리게이트 해제
	if (CurrentObservedUnit)
	{
		CurrentObservedUnit->OnUnitAttributeChanged.RemoveDynamic(this, &ThisClass::OnAttributeChanged);
		CurrentObservedUnit = nullptr;
	}

	const int32 SelectionCount = SelectedActors.Num();

	// 선택이 없는 경우 위젯 숨김
	if (SelectionCount == 0)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::Visible);

	// 1. 단일 선택 모드 (SelectionCount == 1)
	if (SelectionCount == 1)
	{
		if (ANovaUnit* Unit = Cast<ANovaUnit>(SelectedActors[0]))
		{
			if (WidgetSwitcher) WidgetSwitcher->SetActiveWidgetIndex(0);

			// 새로운 유닛 감시 및 델리게이트 바인딩
			CurrentObservedUnit = Unit;
			CurrentObservedUnit->OnUnitAttributeChanged.AddDynamic(this, &ThisClass::OnAttributeChanged);

			UpdateUnitInfo(Unit);
		}
	}
	// 2. 다중 선택 모드 (SelectionCount > 1)
	else
	{
		if (WidgetSwitcher) WidgetSwitcher->SetActiveWidgetIndex(1);
		
		if (MultiSelectionWidget)
		{
			MultiSelectionWidget->UpdateSelection(SelectedActors);
		}
	}
}

void UNovaUnitInfoWidget::OnAttributeChanged(ANovaUnit* Unit)
{
	// 감시 중인 유닛과 일치하는지 확인 후 갱신
	if (Unit == CurrentObservedUnit)
	{
		UpdateUnitInfo(Unit);
	}
}

void UNovaUnitInfoWidget::UpdateUnitInfo(ANovaUnit* Unit)
{
	if (!Unit) return;

	// 유닛 이름 설정
	if (UnitNameText)
	{
		UnitNameText->SetText(FText::FromString(Unit->GetUnitName()));
	}

	// AttributeSet을 통해 상세 정보 갱신
	if (const UNovaAttributeSet* AS = Unit->GetAttributeSet())
	{
		const float CurHP = AS->GetHealth();
		const float MaxHP = AS->GetMaxHealth();
		const float HPPercent = (MaxHP > 0.f) ? (CurHP / MaxHP) : 0.f;

		// 1. 체력 수치 텍스트 (현재 / 최대)
		if (HealthText)
		{
			HealthText->SetText(FText::Format(FText::FromString("{0} / {1}"),
			                                  FText::AsNumber(FMath::RoundToInt(CurHP)),
			                                  FText::AsNumber(FMath::RoundToInt(MaxHP))));
		}

		// 2. 비율별 체력바 색상 설정 (100% 초록, 50%이하 노랑, 20%이하 빨강)
		FLinearColor StatusColor = FLinearColor::Green;
		if (HPPercent <= 0.2f)
		{
			StatusColor = FLinearColor::Red;
		}
		else if (HPPercent <= 0.5f)
		{
			StatusColor = FLinearColor::Yellow;
		}

		// 3. 나머지 능력치 텍스트 설정
		if (AttackText) AttackText->SetText(FText::AsNumber(FMath::RoundToInt(AS->GetAttack())));
		if (DefenseText) DefenseText->SetText(FText::AsNumber(FMath::RoundToInt(AS->GetDefense())));
		if (SpeedText) SpeedText->SetText(FText::AsNumber(FMath::RoundToInt(AS->GetSpeed())));
	}
}
