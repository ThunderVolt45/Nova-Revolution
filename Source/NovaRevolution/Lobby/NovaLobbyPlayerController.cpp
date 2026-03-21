// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/NovaLobbyPlayerController.h"

#include "NovaLobbyManager.h"
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
	
	// 3. 월드 내 배치된 로비 매니저를 찾아 참조를 획득합니다.
	// 에디터에서 수동으로 할당하지 않아도 런타임에 자동으로 서비스 객체를 찾아 바인딩합니다.
	LobbyManager = Cast<ANovaLobbyManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ANovaLobbyManager::StaticClass()));

	if (LobbyManager)
	{
		// 성공 시 로그를 남겨 초기화 순서가 정상임을 확인합니다.
		NOVA_LOG(Log, "LobbyManager successfully bound to Controller.");
	}
	else
	{
		// 실패 시 화면에 경고를 띄워 개발자가 레벨 배치를 누락했음을 즉시 알립니다. (방어적 프로그래밍)
		NOVA_SCREEN(Warning, "LobbyManager not found in World! Please place ANovaLobbyManager in the level.");
	}
	
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
}

void ANovaLobbyPlayerController::StartGame(FName InGameLevelName)
{
	// 현재 편집 중인 덱 상태를 세이브 파일에 기록하도록 매니저에게 명령합니다.
	// 레벨이 전환되기 전 최신 데이터를 영구 저장하여 데이터 유실을 방지합니다.
	if (LobbyManager)
	{
		LobbyManager->SaveCurrentDeck();
	}

	// 이후 전투 레벨이나 지정된 다음 레벨로 이동하는 로직이 이어집니다.
	// UGameplayStatics::OpenLevel(GetWorld(), InGameLevelName);
}
