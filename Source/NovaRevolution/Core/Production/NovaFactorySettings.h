// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/DataTable.h"
#include "NovaFactorySettings.generated.h"

class ANovaUnit;

/**
 * Nova Revolution 팩토리 및 유닛 생산 관련 전역 설정
 * 프로젝트 환경설정(Project Settings) -> 게임(Game) 메뉴에서 설정 가능합니다.
 */
UCLASS(Config=Game, defaultconfig, meta=(DisplayName="Nova Factory Settings"))
class NOVAREVOLUTION_API UNovaFactorySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UNovaFactorySettings();

	/** 부품 스펙을 담고 있는 기본 데이터 테이블 */
	UPROPERTY(Config, EditAnywhere, Category="Factory")
	TSoftObjectPtr<UDataTable> PartSpecDataTable;

	/** 생산에 사용할 기본 유닛 클래스 */
	UPROPERTY(Config, EditAnywhere, Category="Factory")
	TSoftClassPtr<class ANovaUnit> DefaultUnitClass;
};
