// Fill out your copyright notice in the Description page of Project Settings.
// Source/NovaRevolution/Lobby/UI/NovaLobbyStartGameWidget.cpp

#include "Lobby/UI/NovaLobbyStartGameWidget.h"
#include "Components/Button.h"
#include "Lobby/NovaLobbyPlayerController.h"

void UNovaLobbyStartGameWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 블루프린트에서 바인딩된 버튼에 클릭 이벤트 등록
	if (Btn_Start)
	{
		Btn_Start->OnClicked.AddDynamic(this, &UNovaLobbyStartGameWidget::OnStartGameClicked);
	}
}

void UNovaLobbyStartGameWidget::OnStartGameClicked()
{
	// 위젯을 소유한 플레이어 컨트롤러를 로비 전용 컨트롤러로 캐스팅
	if (ANovaLobbyPlayerController* PC = Cast<ANovaLobbyPlayerController>(GetOwningPlayer()))
	{
		// 설정된 TargetLevelName을 인자로 전달하여 레벨 이동 프로세스 시작
		PC->StartGame(TargetLevelName);
	}
}

