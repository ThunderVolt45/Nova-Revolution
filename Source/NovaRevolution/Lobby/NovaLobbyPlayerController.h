// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/NovaAssemblyTypes.h"
#include "Core/NovaPartData.h"
#include "NovaLobbyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class NOVAREVOLUTION_API ANovaLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ANovaLobbyPlayerController();
	
	// --- 데이터 처리 함수 ---
	/** 부품 리스트에서 부품을 선택했을 때 호출 (UI에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby")
	void SelectPart(ENovaPartType PartType, FName PartID);

	/** 1~10번 덱 슬롯을 선택했을 때 호출 (UI에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby")
	void SelectDeckSlot(int32 SlotIndex);

	/** 현재 덱 정보를 세이브 파일("NovaPlayerSaveSlot")에 저장 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby")
	void SaveCurrentDeck();

	/** 게임 시작 (저장 후 인게임 레벨로 이동) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby")
	void StartGame(FName InGameLevelName = TEXT("Lvl_TopDown"));

protected:
	virtual void BeginPlay() override;
	
	/** 초기 진입 시 세이브 파일에서 기존 덱 정보를 불러옴 */
	void LoadDeckFromSave();

	/** 현재 조립 상태를 월드의 프리뷰 액터에 반영 */
	void UpdatePreviewActor();

	// --- 데이터 및 참조 ---
	/** 현재 편집 중인 10개 유닛의 전체 덱 정보 */
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Lobby")
	FNovaDeckInfo CurrentDeck;

	/** 현재 선택된 슬롯 인덱스 (0 ~ 9) */
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Lobby")
	int32 SelectedSlotIndex = 0;

	/** 월드에 배치된 유닛 프리뷰 액터 (BP에서 할당하거나 BeginPlay에서 찾음) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Lobby")
	TObjectPtr<class ANovaUnit> PreviewActor;

	/** 로비 메인 UI 위젯 클래스 (에디터/BP에서 지정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Lobby")
	TSubclassOf<class UUserWidget> LobbyMainWidgetClass;

	/** 생성된 로비 UI 인스턴스 */
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Lobby")
	class UUserWidget* LobbyMainWidget;
};
