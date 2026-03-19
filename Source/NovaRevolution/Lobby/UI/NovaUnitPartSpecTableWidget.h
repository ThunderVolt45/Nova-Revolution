// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/NovaPartData.h" 
#include "NovaUnitPartSpecTableWidget.generated.h"

/**
 * UNovaUnitPartSpecTableWidget
 * 유닛 조립 로비의 개별 부품 스펙을 표(Table) 형식으로 보여주는 위젯입니다.
 * 여러 개의 UNovaUnitPartSpecEntryWidget을 자식으로 포함하여 데이터를 일괄 관리합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaUnitPartSpecTableWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** FNovaPartSpecRow 데이터를 받아 표의 모든 항목(Entry)을 한 번에 갱신합니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby|UI")
	void UpdateTable(const FNovaPartSpecRow& Spec);

protected:
	// --- 에디터 WBP 내부 위젯 인스턴스들과 자동 연결 (BindWidget) ---
	// 에디터에서 배치한 위젯 인스턴스의 이름을 아래 변수명과 일치시켜야 합니다.

	// UPROPERTY(meta = (BindWidget))
	// class UNovaUnitPartSpecEntryWidget* Entry_PartName;
	
	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_MovementType;
	
	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_TargetType;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_Watt;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_Weight;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_Health;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_Attack;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_Defense;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_Speed;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_FireRate;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_Sight;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_Range;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_MinRange;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_SplashRange;
	
	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_CollisionRadius;

	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecEntryWidget* Entry_Homing;
};