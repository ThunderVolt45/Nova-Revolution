// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/NovaResourceWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Core/NovaPlayerState.h"

void UNovaResourceWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // PlayerState를 가져와 델리게이트 바인딩
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (ANovaPlayerState* PS = PC->GetPlayerState<ANovaPlayerState>())
        {
            // 1. 델리게이트 연결 (AddDynamic)
            PS->OnWattChanged.AddDynamic(this, &UNovaResourceWidget::UpdateWattUI);
            PS->OnSPChanged.AddDynamic(this, &UNovaResourceWidget::UpdateSPUI);
            PS->OnPopulationChanged.AddDynamic(this, &UNovaResourceWidget::UpdatePopulationUI);

            // 2. 초기값 동기화 (게임 시작 시 현재 자원 상태 반영)
            UpdateWattUI(PS->GetCurrentWatt(), PS->GetMaxWatt());
            UpdateSPUI(PS->GetCurrentSP(), PS->GetMaxSP());
            UpdatePopulationUI(PS->GetCurrentPopulation(), PS->GetMaxPopulation());

            // UnitWatt는 별도 델리게이트가 없다면 초기화 시 직접 설정
            SetResourceText(Text_UnitWatt, PS->GetTotalUnitWatt(), PS->GetMaxUnitWatt());
            if (ProgressBar_UnitWatt && PS->GetMaxUnitWatt() > 0)
                ProgressBar_UnitWatt->SetPercent(PS->GetTotalUnitWatt() / PS->GetMaxUnitWatt());
        }
    }
}

void UNovaResourceWidget::UpdateWattUI(float Current, float Max)
{
    if (ProgressBar_Watt && Max > 0) ProgressBar_Watt->SetPercent(Current / Max);
    SetResourceText(Text_Watt, Current, Max);
}

void UNovaResourceWidget::UpdateSPUI(float Current, float Max)
{
    if (ProgressBar_SP && Max > 0) ProgressBar_SP->SetPercent(Current / Max);
    SetResourceText(Text_SP, Current, Max);
}

void UNovaResourceWidget::UpdatePopulationUI(float Current, float Max)
{
    if (ProgressBar_Population && Max > 0) ProgressBar_Population->SetPercent(Current / Max);
    SetResourceText(Text_Population, Current, Max);

    // 인구수가 변할 때 유닛 와트 총합도 같이 갱신 (보통 같이 변하므로)
    if (ANovaPlayerState* PS = GetOwningPlayerState<ANovaPlayerState>())
    {
        SetResourceText(Text_UnitWatt, PS->GetTotalUnitWatt(), PS->GetMaxUnitWatt());
        if (ProgressBar_UnitWatt && PS->GetMaxUnitWatt() > 0)
            ProgressBar_UnitWatt->SetPercent(PS->GetTotalUnitWatt() / PS->GetMaxUnitWatt());
    }
}

void UNovaResourceWidget::SetResourceText(UTextBlock* TargetText, float Current, float Max)
{
    if (TargetText)
    {
        // 소수점 버림 처리하여 "125 / 500" 형태로 출력
        FString ResourceString = FString::Printf(TEXT("%d / %d"), FMath::FloorToInt(Current), FMath::FloorToInt(Max));
        TargetText->SetText(FText::FromString(ResourceString));
    }
}