// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/UI/NovaPreviewUnitProfileTableWidget.h"

#include "Components/TextBlock.h"
#include "Lobby/UI/NovaUnitPartSpecEntryWidget.h"
#include "Lobby/NovaLobbyManager.h"
#include "Kismet/GameplayStatics.h"

void UNovaPreviewUnitProfileTableWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 월드에서 LobbyManager를 찾아 캐싱
	LobbyManager = Cast<ANovaLobbyManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ANovaLobbyManager::StaticClass()));

	if (LobbyManager)
	{
		// 2. 조립 변경 델리게이트에 나의 함수 바인딩
		LobbyManager->OnAssemblyDataChanged.AddDynamic(this, &UNovaPreviewUnitProfileTableWidget::HandleAssemblyChanged);

		// 3. 진입 시점에 즉시 한 번 갱신 (현재 선택된 유닛 정보 표시)
		RefreshProfileTable();
	}
}

void UNovaPreviewUnitProfileTableWidget::HandleAssemblyChanged(int32 SlotIndex, const FString& UnitName, const FNovaUnitAssemblyData& AssemblyData)
{
	// 신호를 받으면 테이블 갱신 호출
	RefreshProfileTable();
}

void UNovaPreviewUnitProfileTableWidget::RefreshProfileTable()
{
	if (!LobbyManager) return;

	// 4. [데이터 획득] 매니저에게 합산된 최종 스펙을 요청
	FNovaPartSpecRow TotalSpec = LobbyManager->GetTotalPendingSpec();
	const FNovaUnitAssemblyData& PendingData = LobbyManager->GetPendingData();
	
	// 2. [변경] 유닛 이름 설정 (일반 TextBlock 사용)
	if (Txt_UnitName)
	{
		// FString을 FText로 변환하여 UI에 적용합니다.
		Txt_UnitName->SetText(FText::FromString(PendingData.UnitName));
	}

	// 1. 숫자형 데이터 주입 (정수 표시)
	if (Entry_TotalWatt)
		Entry_TotalWatt->SetStatData(TEXT("TOTAL WATT"), TotalSpec.Watt, true);

	if (Entry_TotalHealth)
		Entry_TotalHealth->SetStatData(TEXT("체력(HP)"), TotalSpec.Health, true);

	if (Entry_TotalAttack)
		Entry_TotalAttack->SetStatData(TEXT("공격력(ATK)"), TotalSpec.Attack, true);

	if (Entry_TotalDefense)
		Entry_TotalDefense->SetStatData(TEXT("방어력(DEF)"), TotalSpec.Defense, true);

	if (Entry_TotalSpeed)
		Entry_TotalSpeed->SetStatData(TEXT("속도(SPD)"), TotalSpec.Speed, true);

	if (Entry_TotalFireRate)
		Entry_TotalFireRate->SetStatData(TEXT("연사(FR)"), TotalSpec.FireRate, true);

	if (Entry_TotalRange)
		Entry_TotalRange->SetStatData(TEXT("사거리(RG)"), TotalSpec.Range, true);

	if (Entry_TotalSight)
		Entry_TotalSight->SetStatData(TEXT("시야(ST)"), TotalSpec.Sight, true);

	// 2. 텍스트형 데이터 (이동 방식) 주입
	if (Entry_MovementType)
	{
		FString MoveTypeText = TEXT("알 수 없음");
    
		if (TotalSpec.MovementType == ENovaMovementType::Ground) 
			MoveTypeText = TEXT("지상");
		else if (TotalSpec.MovementType == ENovaMovementType::Air) 
			MoveTypeText = TEXT("공중");

		Entry_MovementType->SetStatText(TEXT("이동 방식"), MoveTypeText);
	}
}