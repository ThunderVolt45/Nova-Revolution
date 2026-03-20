// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/NovaAssemblyTypes.h"
#include "Core/NovaPartData.h"
#include "NovaLobbyPlayerController.generated.h"

/**
 * ANovaLobbyPlayerController
 * 로비 레벨에서의 입력을 관리하고, UI가 LobbyManager에 접근할 수 있도록 
 * 통로(Gatway) 역할을 수행하는 컨트롤러입니다.
 */
UCLASS()
class NOVAREVOLUTION_API ANovaLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ANovaLobbyPlayerController();

	/** 게임 시작 (저장 후 인게임 레벨로 이동) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby")
	void StartGame(FName InGameLevelName = TEXT("Lvl_TopDown"));
	
	/** 월드에 존재하는 로비 매니저 인스턴스를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Nova|Lobby")
	class ANovaLobbyManager* GetLobbyManager() const { return LobbyManager; }

protected:
	virtual void BeginPlay() override;
	
	/** 로비의 비즈니스 로직(조립, 저장)을 담당하는 매니저 액터 */
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Lobby")
	TObjectPtr<class ANovaLobbyManager> LobbyManager;
	
	/** 로비 메인 UI 위젯 클래스 (에디터/BP에서 지정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Lobby")
	TSubclassOf<class UUserWidget> LobbyMainWidgetClass;

	/** 생성된 로비 UI 인스턴스 */
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Lobby")
	class UUserWidget* LobbyMainWidget;
};
