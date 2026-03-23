// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/UI/NovaDeckListWidget.h"

#include "NovaRevolution.h"
#include "Lobby/UI/NovaDeckSlotWidget.h"
#include "Components/UniformGridPanel.h"
#include "Lobby/NovaLobbyManager.h"
#include "Lobby/NovaLobbyPlayerController.h"
#include "Lobby/Deck/NovaDeckManager.h"
#include "Lobby/Deck/NovaDeckSlot.h"
#include "Core/NovaLog.h"

void UNovaDeckListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 10개의 고정된 위젯을 코드에서 반복문으로 돌리기 위해 배열에 캐싱해 둡니다.
	SlotWidgets.Empty();
	SlotWidgets.Add(DeckSlot_0);
	SlotWidgets.Add(DeckSlot_1);
	SlotWidgets.Add(DeckSlot_2);
	SlotWidgets.Add(DeckSlot_3);
	SlotWidgets.Add(DeckSlot_4);
	SlotWidgets.Add(DeckSlot_5);
	SlotWidgets.Add(DeckSlot_6);
	SlotWidgets.Add(DeckSlot_7);
	SlotWidgets.Add(DeckSlot_8);
	SlotWidgets.Add(DeckSlot_9);
}

void UNovaDeckListWidget::InitializeDeckList()
{
	// 플레이어 컨트롤러를 통해 로비 매니저 -> 덱 매니저(월드 액터)를 찾습니다.
	ANovaLobbyPlayerController* PC = Cast<ANovaLobbyPlayerController>(GetOwningPlayer());
	if (!PC) return;

	ANovaLobbyManager* LobbyManager = Cast<ANovaLobbyManager>(PC->GetLobbyManager());
	if (!LobbyManager) return;

	ANovaDeckManager* DeckManager = LobbyManager->GetDeckManager();
	if (!DeckManager) return;

	// 미리 배치된 10개의 슬롯 UI에 각각의 렌더 타겟을 연결합니다.
	for (int32 i = 0; i < SlotWidgets.Num(); ++i)
	{
		if (UNovaDeckSlotWidget* CurrentSlotWidget = SlotWidgets[i])
		{
			// 덱 매니저에서 i번째 슬롯 액터를 가져옵니다.
			ANovaDeckSlot* SlotActor = DeckManager->GetSlotActor(i);

			UTextureRenderTarget2D* RT = nullptr;
			if (SlotActor)
			{
				RT = SlotActor->GetRenderTarget();
			}

			// 슬롯 UI에 인덱스와 렌더 타겟을 넘겨주어 초기화합니다.
			CurrentSlotWidget->InitSlot(i, RT);
		}
	}
}