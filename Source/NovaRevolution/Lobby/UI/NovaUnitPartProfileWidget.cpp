// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby/UI/NovaUnitPartProfileWidget.h"

#include "NovaRevolution.h"
#include "Lobby/UI/NovaUnitPartSpecTableWidget.h"
#include "Components/TextBlock.h"
#include "Core/NovaLog.h"

void UNovaUnitPartProfileWidget::InitCategory(ENovaPartType Category)
{
    if (!PartSpecTable) return;

    CategoryPartIDs.Empty();

    // 1. 데이터 테이블의 모든 행(RowName)을 가져와 현재 카테고리에 맞는 부품만 필터링
    TArray<FName> AllRowNames = PartSpecTable->GetRowNames();
    for (const FName& RowName : AllRowNames)
    {
        FNovaPartSpecRow* Spec = PartSpecTable->FindRow<FNovaPartSpecRow>(RowName, TEXT(""));
        if (Spec && Spec->PartType == Category)
        {
            CategoryPartIDs.Add(RowName);
        }
    }

    // 2. 리스트의 첫 번째 항목으로 초기 인덱스 설정 및 화면 갱신
    CurrentIndex = 0;
    UpdateDisplay();
}

void UNovaUnitPartProfileWidget::ShowNextPart()
{
    if (CategoryPartIDs.Num() == 0) return;
    
    // 리스트 끝에 도달하면 다시 처음(0)으로 돌아가는 순환 구조
    CurrentIndex = (CurrentIndex + 1) % CategoryPartIDs.Num();
    UpdateDisplay();
}

void UNovaUnitPartProfileWidget::ShowPrevPart()
{
    if (CategoryPartIDs.Num() == 0) return;
    
    // 리스트 처음에 도달하면 다시 마지막으로 돌아가는 순환 구조
    CurrentIndex = (CurrentIndex - 1 + CategoryPartIDs.Num()) % CategoryPartIDs.Num();
    UpdateDisplay();
}

void UNovaUnitPartProfileWidget::UpdateDisplay()
{
    if (!CategoryPartIDs.IsValidIndex(CurrentIndex)) return;

    // 1. 현재 인덱스에 해당하는 부품 ID(RowName) 추출
    FName TargetID = CategoryPartIDs[CurrentIndex];

    // 2. 데이터 테이블에서 해당 행(Row)의 상세 스펙 데이터 조회
    static const FString ContextString(TEXT("Part Profile Update"));
    FNovaPartSpecRow* SpecData = PartSpecTable->FindRow<FNovaPartSpecRow>(TargetID, ContextString);

    if (SpecData)
    {
        // A. 부품 명칭 텍스트 갱신 (예: 로드런너, 건봇 등)
        if (Txt_PartName)
        {
            Txt_PartName->SetText(FText::FromString(SpecData->PartName));
        }

        // B. 이미 구현된 '상세 스펙 표 위젯(WBP_PartSpecTable)'에게 데이터를 통째로 전달
        // 이 호출 한 번으로 표 안의 공격력, 체력, 연사력 등 수십 개 항목이 일괄 갱신됩니다.
        if (WBP_PartSpecTable)
        {
            WBP_PartSpecTable->UpdateTable(*SpecData);
        }

        NOVA_LOG(Log, "Profile UI Updated: %s (Index: %d)", *TargetID.ToString(), CurrentIndex);
    }
}

void UNovaUnitPartProfileWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    // 에디터 디자인 화면에서도 특정 카테고리(예: Legs)를 미리 불러와서
    // 데이터 테이블의 수치가 UI 표에 정상적으로 들어오는지 즉시 확인할 수 있습니다.
    if (!PartSpecTable) return;
    
    InitCategory(ENovaPartType::Legs);
}

