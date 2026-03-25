// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NovaLobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class NOVAREVOLUTION_API ANovaLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ANovaLobbyGameMode();

protected:
	virtual void BeginPlay() override;

public:
	/** 로비 진입 시 미리 메모리에 로드(Preload)할 부품 데이터 테이블 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Lobby")
	class UDataTable* PartAssetDataTable;
};
