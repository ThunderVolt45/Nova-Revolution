// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovaResourceWidget.generated.h"

/**
 * RTS 자원(Watt, SP, Population, UnitWatt) 정보를 실시간으로 표시하는 위젯 클래스
 */
UCLASS()
class NOVAREVOLUTION_API UNovaResourceWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	// 위젯 생성 시 호출 (블루프린트의 Construct)
	virtual void NativeConstruct() override;

	// --- 자원별 UI 요소 바인딩 (이름이 블루프린트 위젯 이름과 일치해야 함) ---

	// 1. Watt (와트)
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* ProgressBar_Watt;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text_Watt;

	// 2. SP (스킬 포인트)
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* ProgressBar_SP;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text_SP;

	// 3. Population (개체 수)
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* ProgressBar_Population;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text_Population;

	// 4. UnitWatt (유닛 와트 총합)
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* ProgressBar_UnitWatt;
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text_UnitWatt;

	// --- 델리게이트 응답 함수 ---
	UFUNCTION()
	void UpdateWattUI(float Current, float Max);

	UFUNCTION()
	void UpdateSPUI(float Current, float Max);

	UFUNCTION()
	void UpdatePopulationUI(float Current, float Max);
	
	UFUNCTION()
	void UpdateUnitWattUI(float Current, float Max);

	// 공통 텍스트 포맷팅 함수 (예: "50 / 100")
	void SetResourceText(class UTextBlock* TargetText, float Current, float Max);
	
};
