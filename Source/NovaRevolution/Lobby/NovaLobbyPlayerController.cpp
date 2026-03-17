// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/NovaLobbyPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "NovaRevolution.h" // 프로젝트 전용 로그 매크로 사용을 위함
#include "Core/NovaSaveGame.h"
#include "Core/NovaUnit.h"
#include "Kismet/GameplayStatics.h"


ANovaLobbyPlayerController::ANovaLobbyPlayerController()
{
	// 마우스 커서 항상 표시 및 클릭/오버 이벤트 활성화
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ANovaLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// 1. 입력 모드를 Game And UI로 설정하여 월드와 UI 동시 상호작용 가능케 함
	// (로비에서 유닛을 클릭하여 회전시키거나 UI 버튼을 누르는 작업에 최적화)
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	// 2. 로비 메인 UI 위젯 생성 및 출력
	if (LobbyMainWidgetClass)
	{
		LobbyMainWidget = CreateWidget<UUserWidget>(this, LobbyMainWidgetClass);
		if (LobbyMainWidget)
		{
			LobbyMainWidget->AddToViewport();
			NOVA_LOG(Log, "Lobby Main Widget Created and Added to Viewport.");
		}
	}
	else
	{
		NOVA_SCREEN(Warning, "LobbyMainWidgetClass is not assigned in PlayerController!");
	}
	
	// --- 데이터 로드 및 프리뷰 액터 초기화 ---

	// 3-1. 세이브 파일에서 덱 정보를 불러와 메모리(CurrentDeck)에 올립니다.
	LoadDeckFromSave();

	// 3-2. 월드에 배치된 유닛 프리뷰 액터를 찾아 연결합니다.
	// (에디터에서 미리 변수에 할당해 두지 않았을 경우를 대비한 안전 코드입니다.)
	if (!PreviewActor)
	{
		// 월드에서 ANovaUnit 클래스의 액터를 하나 찾아옵니다.
		PreviewActor = Cast<ANovaUnit>(UGameplayStatics::GetActorOfClass(GetWorld(), ANovaUnit::StaticClass()));

		if (PreviewActor)
		{
			NOVA_LOG(Log, "Found PreviewActor in World: %s", *PreviewActor->GetName());
		}
		else
		{
			NOVA_LOG(Warning, "No ANovaUnit found in the Lobby level for Preview!");
		}
	}

	// 3-3. 현재 선택된 슬롯(기본 0번)의 유닛 정보를 프리뷰 액터에 적용하여 실시간 외형을 동기화합니다.
	UpdatePreviewActor();
}

void ANovaLobbyPlayerController::LoadDeckFromSave()
{
	if (UGameplayStatics::DoesSaveGameExist(TEXT("NovaPlayerSaveSlot"), 0))
	{
		if (UNovaSaveGame* LoadedGame = Cast<UNovaSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("NovaPlayerSaveSlot"), 0)))
		{
			CurrentDeck = LoadedGame->SavedDeck;
			return;
		}
	}

	// 세이브가 없거나 불러오기 실패 시 기본 10개의 빈 슬롯으로 초기화
	CurrentDeck.Units.SetNum(10);
}

void ANovaLobbyPlayerController::UpdatePreviewActor()
{
	if (PreviewActor && CurrentDeck.Units.IsValidIndex(SelectedSlotIndex))
	{
		// ANovaUnit에 구현된 부품 조립 로직을 호출하여 실시간으로 외형을 갱신합니다.
		PreviewActor->SetAssemblyData(CurrentDeck.Units[SelectedSlotIndex]);
	}
}

void ANovaLobbyPlayerController::SelectPart(ENovaPartType PartType, FName PartID)
{
	if (!CurrentDeck.Units.IsValidIndex(SelectedSlotIndex)) return;

	// TODO: DT_PartSpecData에서 해당 PartID의 Class를 찾아와야 함
	// 데이터 테이블 조회를 통해 실제 액터 클래스나 정보를 가져오는 로직이 추가될 위치입니다.
	// 예: CurrentDeck.Units[SelectedSlotIndex].LegsClass = FoundClass;

	UpdatePreviewActor();
}

void ANovaLobbyPlayerController::SelectDeckSlot(int32 SlotIndex)
{
	if (CurrentDeck.Units.IsValidIndex(SlotIndex))
	{
		SelectedSlotIndex = SlotIndex;
		UpdatePreviewActor(); // 슬롯 변경 시 외형 업데이트
	}
}

void ANovaLobbyPlayerController::SaveCurrentDeck()
{
	UNovaSaveGame* SaveGameInstance = Cast<UNovaSaveGame>(UGameplayStatics::CreateSaveGameObject(UNovaSaveGame::StaticClass()));
	if (SaveGameInstance)
	{
		SaveGameInstance->SavedDeck = CurrentDeck;
		UGameplayStatics::SaveGameToSlot(SaveGameInstance, TEXT("NovaPlayerSaveSlot"), 0);
		NOVA_SCREEN(Log, "Deck Saved Successfully!");
	}
}

void ANovaLobbyPlayerController::StartGame(FName InGameLevelName)
{
	SaveCurrentDeck(); // 저장 먼저 수행하여 데이터 정합성 유지
	UGameplayStatics::OpenLevel(GetWorld(), InGameLevelName);
}
