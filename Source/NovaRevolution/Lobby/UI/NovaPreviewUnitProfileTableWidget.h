// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/NovaAssemblyTypes.h"
#include "Core/NovaPartData.h"
#include "NovaPreviewUnitProfileTableWidget.generated.h"

/**
 * UNovaPreviewUnitProfileTableWidget
 * 조립된 전체 유닛의 합산 스탯을 표(Table) 형식으로 보여주는 위젯입니다.
 * 스스로 LobbyManager의 이벤트를 구독하여 데이터를 갱신합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaPreviewUnitProfileTableWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	/** 위젯 생성 시 초기화 및 이벤트 바인딩 */
	virtual void NativeConstruct() override;

	/** 로비 매니저의 조립 데이터 변경 신호를 처리하는 콜백 함수 */
	UFUNCTION()
	void HandleAssemblyChanged(int32 SlotIndex, const FString& UnitName, const FNovaUnitAssemblyData& AssemblyData);

	/** 매니저로부터 합산 스탯을 가져와 UI를 실제로 갱신하는 함수 */
	void RefreshProfileTable();

protected:
	/** [추가] 유닛 이름을 크게 보여줄 일반 텍스트 위젯 */
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_UnitName;
	
	// --- WBP에서 배치할 Entry 위젯들 (이름을 아래 변수명과 일치시켜야 함) ---
	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_TotalWatt;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_TotalHealth;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_TotalAttack;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_TotalDefense;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_TotalSpeed;
	
	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_TotalFireRate;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_TotalRange;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_TotalSight;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_MovementType;

private:
	/** 캐싱된 로비 매니저 참조 */
	UPROPERTY()
	TObjectPtr<class ANovaLobbyManager> LobbyManager;
	
};
