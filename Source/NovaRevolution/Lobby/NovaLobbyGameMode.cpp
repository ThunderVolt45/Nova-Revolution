// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/NovaLobbyGameMode.h"

#include "NovaRevolution.h"
#include "Lobby/NovaLobbyPlayerController.h"
#include "Core/NovaPartData.h"
#include "Core/NovaObjectPoolSubsystem.h"
#include "Core/NovaLog.h"
#include "Core/NovaPart.h"

ANovaLobbyGameMode::ANovaLobbyGameMode()
{
	PlayerControllerClass = ANovaLobbyPlayerController::StaticClass();
	
	// 로비용 카메라/폰이 필요하다면 여기에 추가 (현재는 기본 DefaultPawn 사용 가능)
	// DefaultPawnClass = ANovaLobbyPawn::StaticClass();
}

void ANovaLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 로비 진입 시 블루프린트에서 할당된 PartAssetDataTable을 읽어 
	// 모든 부품을 오브젝트 풀에 최소 1개씩 미리 생성(Preload)합니다.
	// 이를 통해 카메라 캡처 시 텍스처/머티리얼 스트리밍이 지연되어 흐릿하게 나오는 현상을 방지합니다.
	if (PartAssetDataTable)
	{
		UNovaObjectPoolSubsystem* Pool = GetWorld()->GetSubsystem<UNovaObjectPoolSubsystem>();
		if (Pool)
		{
			TArray<FNovaPartAssetRow*> AllRows;
			PartAssetDataTable->GetAllRows<FNovaPartAssetRow>(TEXT("LobbyPreload"), AllRows);

			int32 PreloadedCount = 0;
			for (FNovaPartAssetRow* Row : AllRows)
			{
				if (Row && Row->PartClass)
				{
					Pool->PreloadPool(Row->PartClass, 1);
					PreloadedCount++;
				}
			}

			NOVA_LOG(Log, "Lobby GameMode: Successfully preloaded %d part assets into the Object Pool.", PreloadedCount);
		}
	}
	else
	{
		NOVA_LOG(Warning, "Lobby GameMode: PartAssetDataTable is not assigned. Cannot preload parts.");
	}
}
