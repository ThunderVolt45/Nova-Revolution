// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovaUnitAssemblyControlWidget.generated.h"

/**
 * UNovaUnitAssemblyControlWidget
 * 로비 조립 화면 하단 혹은 측면에 위치하여 조립 확정(Confirm), 해제(Release)를 담당하고
 * 현재 편집 중인 슬롯의 상태를 보여주는 컨트롤 패널 위젯입니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaUnitAssemblyControlWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 위젯이 생성되고 초기화될 때 버튼 클릭 이벤트를 바인딩합니다. */
	virtual void NativeConstruct() override;

	/** * 현재 편집 중인 슬롯 번호와 유닛 이름을 UI 텍스트에 동기화합니다. 
	 * LobbyManager의 SelectDeckSlot이나 ConfirmAssembly 호출 시 함께 갱신하기에 적합합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby|UI")
	void UpdateStatus(int32 SlotIndex, const FString& UnitName);

protected:
	/** 조립 확정 버튼 클릭 시 호출되어 LobbyManager의 ConfirmAssembly를 실행합니다. */
	UFUNCTION()
	void OnConfirmClicked();

	/** 조립 해제 버튼 클릭 시 호출되어 LobbyManager의 ReleaseAssembly를 실행합니다. */
	UFUNCTION()
	void OnReleaseClicked();

	// --- 위젯 바인딩 (WBP의 계층 구조 내 이름과 일치해야 함) ---

	/** 현재 편집 중인 슬롯의 인덱스를 표시하는 텍스트 블록 */
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_SlotNumber;

	/** 현재 유닛의 이름을 표시하는 텍스트 블록 */
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_UnitName;

	/** 조립 확정(저장) 버튼 */
	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_Confirm;

	/** 조립 해제(초기화) 버튼 */
	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_Release;
};