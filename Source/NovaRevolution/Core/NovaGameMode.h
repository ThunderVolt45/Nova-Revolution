// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/NovaTypes.h"
#include "Core/NovaAssemblyTypes.h"
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

	// 플레이어의 덱 정보를 반환 (유닛 생산 팩토리 등에서 참조)
	UFUNCTION(BlueprintPure, Category = "Nova|GameMode")
	FNovaDeckInfo GetPlayerDeck(int32 PlayerTeamID) const;

protected:
	virtual void BeginPlay() override;

	// 플레이어별 기지 초기화 로직
	virtual void InitializePlayerBase();

	// 세이브 데이터로부터 덱 정보 로드
	virtual void LoadPlayerDecks();

	/** 데이터가 없는 경우 기본 프리셋 덱을 생성합니다. */
	virtual void InitializeDefaultDecks();

	// 게임 종료 처리
	void EndMatch(int32 WinningTeamID);

	// --- 데이터 ---
	// 스폰할 기지 클래스 (BP_NovaBase 등으로 할당)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|GameMode")
	TSubclassOf<ANovaBase> BaseClass;

	// 자동 스폰할 AI 컨트롤러 클래스 (BP_NovaAIPlayerController 할당)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|GameMode")
	TSubclassOf<class ANovaAIPlayerController> AIControllerClass;

	// 게임 시작 시 AI 컨트롤러를 자동 스폰할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|GameMode")
	bool bAutoSpawnAI = true;

	/** 데이터가 없는 경우 사용할 기본 프리셋 덱 에셋 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|GameMode")
	TObjectPtr<class UNovaDeckPreset> DefaultDeckPreset;

	/** 부품 정보 조회를 위한 데이터 테이블 (오브젝트 풀 Prewarm 시 필요) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|GameMode")
	TObjectPtr<class UDataTable> PartDataTable;

	// 현재 게임 내에 살아있는 각 팀의 기지 관리
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|GameMode")
	TMap<int32, ANovaBase*> TeamBases;

	// 각 팀별 덱 정보 관리
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|GameMode")
	TMap<int32, FNovaDeckInfo> TeamDecks;
};
