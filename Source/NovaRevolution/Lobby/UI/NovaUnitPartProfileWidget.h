// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/NovaPartData.h"
#include "NovaUnitPartProfileWidget.generated.h"

/**
 * UNovaUnitPartProfileWidget
 * 부품의 공식 명칭과 상세 스펙 표를 데이터 테이블과 연결하여 동적으로 보여주는 위젯입니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaUnitPartProfileWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 특정 카테고리(다리/몸통/무기)의 부품들만 필터링하여 리스트업합니다.
	 * (WBP의 Construct나 버튼 이벤트에서 호출)
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby|UI")
	void InitCategory(ENovaPartType Category);

	/** 화살표 버튼 이벤트 (UI 버튼에 바인딩) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby|UI")
	void ShowNextPart();

	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby|UI")
	void ShowPrevPart();
	
	//test
	virtual void NativePreConstruct() override;

protected:
	/** 현재 인덱스의 데이터를 읽어와 이름과 표 위젯을 실시간으로 업데이트합니다. */
	void UpdateDisplay();

	// --- 데이터 테이블 (에디터 디테일 패널에서 할당) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Data")
	TObjectPtr<UDataTable> PartSpecTable;

	// --- 에디터 WBP 위젯 바인딩 (이름 일치 필수) ---

	/** 부품 공식 명칭 표시 (로드런너, 건봇 등) */
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_PartName;

	/** 상세 능력치 표 위젯 (이미 구현된 TableWidget 인스턴스) */
	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecTableWidget* WBP_PartSpecTable;

	// --- 내부 상태 관리 ---
	/** 현재 카테고리에 속한 부품 ID 리스트 (순회용) */
	TArray<FName> CategoryPartIDs; 
    
	int32 CurrentIndex = 0;
};