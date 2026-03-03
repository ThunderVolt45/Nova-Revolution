// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaGameMode.h"
#include "Core/NovaBase.h"
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
			UE_LOG(LogTemp, Log, TEXT("Loaded Player Deck with %d units."), LoadGameInstance->SavedDeck.Units.Num());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No SaveGame found. Using empty deck."));
		TeamDecks.Add(1, FNovaDeckInfo());
	}

	// TODO: AI(Team2)의 덱 정보 설정 (데이터 테이블 등에서 로드 가능)
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
		UE_LOG(LogTemp, Error, TEXT("BaseClass is not set in NovaGameMode!"));
		return;
	}

	// 모든 PlayerStart를 찾아서 각 팀의 기지를 배치 (임시로 첫 번째는 Player, 두 번째는 Enemy)
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);

	for (int32 i = 0; i < PlayerStarts.Num(); ++i)
	{
		// 첫 번째는 Team1(1), 두 번째는 Team2(2)...
		int32 AssignedTeamID = i + 1;
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ANovaBase* NewBase = GetWorld()->SpawnActor<ANovaBase>(BaseClass, PlayerStarts[i]->GetActorTransform(), SpawnParams);
		
		if (NewBase)
		{
			// 팀 관리 맵에 저장
			TeamBases.Add(AssignedTeamID, NewBase);
			
			UE_LOG(LogTemp, Warning, TEXT("Spawned Base for TeamID: %d at %s"), AssignedTeamID, *PlayerStarts[i]->GetName());
		}
	}
}

void ANovaGameMode::OnBaseDestroyed(ANovaBase* DestroyedBase)
{
	if (!DestroyedBase) return;

	int32 DestroyedTeamID = DestroyedBase->GetTeamID();
	UE_LOG(LogTemp, Error, TEXT("Match Over! Team %d's base has been destroyed."), DestroyedTeamID);

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
	UE_LOG(LogTemp, Warning, TEXT("WINNER: Team %d"), WinningTeamID);
}
