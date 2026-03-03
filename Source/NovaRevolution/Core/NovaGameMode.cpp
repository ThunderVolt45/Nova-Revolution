// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaGameMode.h"
#include "Core/NovaBase.h"
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

	// 게임 시작 시 기지 배치
	InitializePlayerBase();
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
		ENovaTeam AssignedTeam = (i == 0) ? ENovaTeam::Player : ENovaTeam::Enemy;
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ANovaBase* NewBase = GetWorld()->SpawnActor<ANovaBase>(BaseClass, PlayerStarts[i]->GetActorTransform(), SpawnParams);
		
		if (NewBase)
		{
			// 팀 설정 (내부적으로 인터페이스를 통해 GetTeam 호출 가능)
			// 실제 구현 시 ANovaBase에 SetTeam(AssignedTeam) 함수를 추가하거나 초기화 로직 필요
			// 여기서는 로그로 대체하고 팀 관리 맵에 저장
			TeamBases.Add(AssignedTeam, NewBase);
			
			UE_LOG(LogTemp, Warning, TEXT("Spawned Base for Team: %d at %s"), (int32)AssignedTeam, *PlayerStarts[i]->GetName());
		}
	}
}

void ANovaGameMode::OnBaseDestroyed(ANovaBase* DestroyedBase)
{
	if (!DestroyedBase) return;

	ENovaTeam DestroyedTeam = DestroyedBase->GetTeam();
	UE_LOG(LogTemp, Error, TEXT("Match Over! Team %d's base has been destroyed."), (int32)DestroyedTeam);

	// 상대 팀 승리 처리
	ENovaTeam WinningTeam = (DestroyedTeam == ENovaTeam::Player) ? ENovaTeam::Enemy : ENovaTeam::Player;
	EndMatch(WinningTeam);
}

void ANovaGameMode::EndMatch(ENovaTeam WinningTeam)
{
	// 게임 종료 UI 호출 및 게임 멈춤 로직 등 추가 예정
	UE_LOG(LogTemp, Warning, TEXT("WINNER: Team %d"), (int32)WinningTeam);

	// TODO: GameState를 통해 모든 클라이언트에 승패 알림
}
