// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovaLobbyStartGameWidget.generated.h"

class UButton;

/**
 * UNovaLobbyStartGameWidget
 * 인게임(전투 레벨) 진입을 담당하는 전용 위젯입니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaLobbyStartGameWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	/** 위젯 생성 시 버튼 이벤트를 바인딩합니다. */
	virtual void NativeConstruct() override;

	/** 버튼 클릭 시 호출되어 레벨 이동을 수행합니다. */
	UFUNCTION()
	void OnStartGameClicked();

	/** 이동할 인게임 레벨 이름 (에디터의 디테일 패널에서 수정 가능) */
	UPROPERTY(EditAnywhere, Category = "Nova|Lobby")
	FName TargetLevelName = TEXT("LVL_Nova_Station");

protected:
	/** * 위젯 블루프린트(WBP)에서 'Btn_Start'라는 이름으로 버튼을 배치해야 합니다. 
	 * meta = (BindWidget)을 통해 코드와 에셋을 자동으로 연결합니다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Start;
	
};
