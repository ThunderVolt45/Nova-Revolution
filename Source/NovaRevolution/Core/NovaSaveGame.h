// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/NovaTypes.h"
#include "Core/NovaAssemblyTypes.h"
#include "NovaSaveGame.generated.h"

/**
 * 플레이어의 덱 정보 및 커스텀 설정값을 저장하는 클래스
 */
UCLASS()
class NOVAREVOLUTION_API UNovaSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UNovaSaveGame();

	// 플레이어가 구성한 덱 정보
	UPROPERTY(VisibleAnywhere, Category = "Nova|SaveData")
	FNovaDeckInfo SavedDeck;

	// 세이브 슬롯 이름 및 사용자 인덱스
	UPROPERTY(VisibleAnywhere, Category = "Nova|SaveData")
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = "Nova|SaveData")
	int32 UserIndex;
};
