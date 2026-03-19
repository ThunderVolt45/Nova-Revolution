// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovaUnitPartSpecEntryWidget.generated.h"

/**
 * UNovaUnitPartSpecRow
 * 유닛 조립 로비의 스탯 표에서 '한 줄(행)'을 담당하는 최소 단위 위젯입니다.
 * 항목 이름(예: 체력)과 해당 수치를 세트로 표시합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaUnitPartSpecEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 외부(부모 위젯)에서 호출하여 이 줄의 데이터를 채웁니다.
	 * @param InLabel: 표시할 항목 이름 (예: "체력")
	 * @param InValue: 표시할 실제 수치
	 * @param bIsInteger: 정수형으로 표시할지 여부 (기본값 true)
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby|UI")
	void SetStatData(const FString& InLabel, float InValue, bool bIsInteger = true);
	
	/** 문자열 데이터(FString)를 받아 출력합니다. (예: 부품명, 이동 방식, 무기 타입 등) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby|UI")
	void SetStatText(const FString& InLabel, const FString& InValueText);

protected:
	/** 항목 이름 텍스트 (에디터의 WBP 내부 TextBlock 이름을 'Txt_Label'로 지어야 함) */
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PartSpec_Label;

	/** 수치 데이터 텍스트 (에디터의 WBP 내부 TextBlock 이름을 'Txt_Value'로 지어야 함) */
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PartSpec_Value;
};
