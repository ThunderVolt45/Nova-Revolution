// Fill out your copyright notice in the Description page of Project Settings.

#include "NovaGameMode.h"
#include "NovaRevolution.h"
#include "Core/NovaBase.h"
#include "Core/NovaPlayerState.h"
#include "Core/NovaSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

ANovaGameMode::ANovaGameMode()
{
	// 기본값 초기화
	BaseClass = nullptr;
}

void ANovaGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 1. 플레이어 덱 정보 로드
	LoadPlayerDecks();

	// 2. 게임 시작 시 기지 배치
	InitializePlayerBase();
}

void ANovaGameMode::LoadPlayerDecks()
{
	// 플레이어(Team1)의 덱 로드 시도
	if (UGameplayStatics::DoesSaveGameExist(TEXT("NovaPlayerSaveSlot"), 0))
	{
		if (UNovaSaveGame* LoadGameInstance = Cast<UNovaSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("NovaPlayerSaveSlot"), 0)))
		{
			TeamDecks.Add(1, LoadGameInstance->SavedDeck);
			NOVA_LOG(Log, "Loaded Player Deck with %d units.", LoadGameInstance->SavedDeck.Units.Num());
		}
	}
	else
	{
		NOVA_LOG(Warning, "No SaveGame found. Using empty deck.");
		TeamDecks.Add(1, FNovaDeckInfo());
	}
}

FNovaDeckInfo ANovaGameMode::GetPlayerDeck(int32 PlayerTeamID) const
{
	if (TeamDecks.Contains(PlayerTeamID))
	{
		return TeamDecks[PlayerTeamID];
	}
	
	return FNovaDeckInfo();
}

void ANovaGameMode::InitializePlayerBase()
{
	if (!BaseClass)
	{
		NOVA_LOG(Error, "BaseClass is not set in NovaGameMode!");
		return;
	}

	// 모든 PlayerStart를 찾아서 각 팀의 기지를 배치 (임시로 첫 번째는 Player, 두 번째는 Enemy)
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);

	// 로컬 플레이어의 PlayerState를 가져와서 팀 설정 (RTS 싱글플레이어 기준)
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	ANovaPlayerState* PS = PC ? PC->GetPlayerState<ANovaPlayerState>() : nullptr;
	if (PS)
	{
		PS->SetTeamID(1); // 플레이어는 무조건 Team 1
	}

	for (int32 i = 0; i < PlayerStarts.Num(); ++i)
	{
		// 첫 번째는 Team1(1), 두 번째는 Team2(2)...
		int32 AssignedTeamID = i + 1;
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ANovaBase* NewBase = GetWorld()->SpawnActor<ANovaBase>(BaseClass, PlayerStarts[i]->GetActorTransform(), SpawnParams);
		
		if (NewBase)
		{
			// 기지에 팀 ID 설정
			NewBase->SetTeamID(AssignedTeamID);

			// 팀 관리 맵에 저장
			TeamBases.Add(AssignedTeamID, NewBase);
			
			NOVA_LOG(Warning, "Spawned Base for TeamID: %d at %s", AssignedTeamID, *PlayerStarts[i]->GetName());
		}
	}
}

void ANovaGameMode::OnBaseDestroyed(ANovaBase* DestroyedBase)
{
	if (!DestroyedBase) return;

	int32 DestroyedTeamID = DestroyedBase->GetTeamID();
	NOVA_SCREEN(Error, "Match Over! Team %d's base has been destroyed.", DestroyedTeamID);

	// 상대 팀 승리 처리
	int32 WinningTeamID = NovaTeam::None;
	for (auto& Elem : TeamBases)
	{
		if (Elem.Key != DestroyedTeamID)
		{
			WinningTeamID = Elem.Key;
			break;
		}
	}
	
	EndMatch(WinningTeamID);
}

void ANovaGameMode::EndMatch(int32 WinningTeamID)
{
	NOVA_SCREEN(Warning, "WINNER: Team %d", WinningTeamID);
}
