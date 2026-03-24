// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovaGameResultWidget.generated.h"

/**
 * 게임 종료 시 승리/패배 결과를 표시하는 위젯의 베이스 클래스
 */
UCLASS()
class NOVAREVOLUTION_API UNovaGameResultWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	
	/** 위젯이 생성될 때 승패 여부를 설정합니다. (블루프린트에서 호출 가능) */
	UFUNCTION(BlueprintCallable, Category = "Nova|UI")
	void SetGameResult(bool bInIsWinner);
	
protected:
	// --- [UI 요소 바인딩] ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> ResultText;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> LobbyBtn;
	
	/** 결과에 따른 연출 처리 내부 함수 */
	void OnResultApplied();

	/** 로비(조립 화면)로 돌아가기 버튼 클릭 시 호출 */
	UFUNCTION(BlueprintCallable, Category = "Nova|UI")
	void OnQuitToLobbyClicked();

	/** 현재 게임 다시 시작 버튼 클릭 시 호출 */
	// UFUNCTION(BlueprintCallable, Category = "Nova|UI")
	// void OnRestartClicked();

	/** 플레이어가 승리했는지 여부 (블루프린트에서 참조용) */
	UPROPERTY(BlueprintReadOnly, Category = "Nova|UI")
	bool bIsWinner = false;
	
	/** 로비 맵의 실제 이름 (프로젝트 설정에 맞게 "LVL_Unit_Assembly_Lobby" 등 지정) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|UI")
	FName LobbyLevelName = FName("LVL_Unit_Assembly_Lobby"); // FName LobbyLevelName = FName("Lvl_Entry");
	
	
};
