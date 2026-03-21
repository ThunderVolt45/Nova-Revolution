// Copyright Epic Games, Inc. All Rights Reserved.

#include "NovaRevolution.h"
#include "Modules/ModuleManager.h"
#include "AbilitySystemGlobals.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FNovaRevolutionModule, NovaRevolution, "NovaRevolution" );

void FNovaRevolutionModule::StartupModule()
{
	// GAS 글로벌 데이터 초기화 (빌드 버전에서 GCN 작동을 위해 필수)
	UAbilitySystemGlobals::Get().InitGlobalData();
}

void FNovaRevolutionModule::ShutdownModule()
{
}

DEFINE_LOG_CATEGORY(LogNovaRevolution)