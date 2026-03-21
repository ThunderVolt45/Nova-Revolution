// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/Production/NovaFactorySettings.h"
#include "Core/NovaUnit.h"

UNovaFactorySettings::UNovaFactorySettings()
{
	// 에셋의 기본 경로 지정 (기존 하드코딩된 경로를 기본값으로 사용, 언제든 에디터에서 변경 가능)
	PartSpecDataTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/_BP/Data/DT_PartSpecData.DT_PartSpecData")));
	DefaultUnitClass = TSoftClassPtr<ANovaUnit>(FSoftClassPath(TEXT("/Game/_BP/Units/BP_NovaUnitBase.BP_NovaUnitBase_C")));
}
