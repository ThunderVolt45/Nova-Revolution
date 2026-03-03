// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/NovaTypes.h"
#include "NovaGameMode.generated.h"

class ANovaBase;

/**
 * Nova Revolution의 핵심 게임 규칙 및 흐름을 관리하는 클래스
 */
UCLASS()
class NOVAREVOLUTION_API ANovaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANovaGameMode();

	// 기지 파괴 시 호출될 함수 (승패 판정의 트리거)
	virtual void OnBaseDestroyed(ANovaBase* DestroyedBase);

protected:
	virtual void BeginPlay() override;

	// 플레이어별 기지 초기화 로직
	virtual void InitializePlayerBase();

	// 게임 종료 처리
	void EndMatch(ENovaTeam WinningTeam);

	// --- 데이터 ---
	// 스폰할 기지 클래스 (BP_NovaBase 등으로 할당)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|GameMode")
	TSubclassOf<ANovaBase> BaseClass;

private:
	// 현재 게임 내에 살아있는 각 팀의 기지 관리
	TMap<ENovaTeam, ANovaBase*> TeamBases;
};
