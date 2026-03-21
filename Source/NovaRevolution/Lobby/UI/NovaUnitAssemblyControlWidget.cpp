// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby/UI/NovaUnitAssemblyControlWidget.h"

#include "NovaRevolution.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/NovaLog.h"
#include "Lobby/NovaLobbyPlayerController.h"
#include "Lobby/NovaLobbyManager.h"

void UNovaUnitAssemblyControlWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 조립 확정 버튼에 클릭 이벤트 동적 바인딩
	if (Btn_Confirm)
	{
		Btn_Confirm->OnClicked.AddDynamic(this, &UNovaUnitAssemblyControlWidget::OnConfirmClicked);
	}

	// 조립 해제 버튼에 클릭 이벤트 동적 바인딩
	if (Btn_Release)
	{
		Btn_Release->OnClicked.AddDynamic(this, &UNovaUnitAssemblyControlWidget::OnReleaseClicked);
	}
	
	// 매니저를 찾아 델리게이트 신호를 구독합니다.
	if (ANovaLobbyPlayerController* PC = Cast<ANovaLobbyPlayerController>(GetOwningPlayer()))
	{
		if (ANovaLobbyManager* Manager = PC->GetLobbyManager())
		{
			// [중요] 매니저의 OnAssemblyDataChanged 신호가 올 때마다 
			// 내 UpdateStatus 함수가 자동으로 호출되도록 연결(Bind)합니다.
			Manager->OnAssemblyDataChanged.AddDynamic(this, &UNovaUnitAssemblyControlWidget::UpdateStatus);

			// 초기 동기화: 위젯이 켜진 순간의 현재 데이터를 한 번 반영합니다.
			UpdateStatus(Manager->GetSelectedSlotIndex(), Manager->GetPendingData().UnitName);
            
			NOVA_LOG(Log, "Widget successfully subscribed to LobbyManager's Assembly Event.");
		}
		else
		{
			// [중요] 매니저를 찾지 못한 경우: 매니저가 아직 스폰되지 않았거나 참조가 꼬였을 가능성이 큽니다.
			// 화면에 즉시 경고를 띄워 개발자가 문제를 인지하게 합니다.
			NOVA_SCREEN(Warning, "Widget: LobbyManager not found during Construct. Check Actor Spawn Timing!");
			NOVA_LOG(Error, "Widget: Failed to find LobbyManager from PlayerController.");
		}
	}
	
}

void UNovaUnitAssemblyControlWidget::UpdateStatus(int32 SlotIndex, const FString& UnitName)
{
	// 슬롯 번호 텍스트 갱신 (내부 인덱스 0~9를 유저 친화적인 1~10으로 변환)
	if (Txt_SlotNumber)
	{
		Txt_SlotNumber->SetText(FText::AsNumber(SlotIndex + 1));
	}

	// 현재 조립 중인 유닛의 이름을 텍스트로 표시
	if (Txt_UnitName)
	{
		Txt_UnitName->SetText(FText::FromString(UnitName));
	}
}

void UNovaUnitAssemblyControlWidget::OnConfirmClicked()
{
	// 플레이어 컨트롤러를 통해 중앙 로비 매니저에 접근하여 조립 데이터 확정 요청
	if (ANovaLobbyPlayerController* PC = Cast<ANovaLobbyPlayerController>(GetOwningPlayer()))
	{
		if (ANovaLobbyManager* Manager = PC->GetLobbyManager())
		{
			// 임시 데이터를 실제 덱 슬롯에 저장하고 전시장 갱신 트리거
			Manager->ConfirmAssembly();
		}
	}
}

void UNovaUnitAssemblyControlWidget::OnReleaseClicked()
{
	// 플레이어 컨트롤러를 통해 중앙 로비 매니저에 접근하여 조립 해제 요청
	if (ANovaLobbyPlayerController* PC = Cast<ANovaLobbyPlayerController>(GetOwningPlayer()))
	{
		if (ANovaLobbyManager* Manager = PC->GetLobbyManager())
		{
			// 현재 슬롯의 데이터를 초기화하고 전시장 유닛 제거 트리거
			Manager->ReleaseAssembly();
		}
	}
}

