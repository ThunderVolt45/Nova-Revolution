// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/NovaLobbyGameMode.h"
#include "Lobby/NovaLobbyPlayerController.h"

ANovaLobbyGameMode::ANovaLobbyGameMode()
{
	PlayerControllerClass = ANovaLobbyPlayerController::StaticClass();
	
	// 로비용 카메라/폰이 필요하다면 여기에 추가 (현재는 기본 DefaultPawn 사용 가능)
	// DefaultPawnClass = ANovaLobbyPawn::StaticClass();
}
