// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/NovaGameplayAbility.h"
#include "NovaCommanderAbility.generated.h"

/**
 * UNovaCommanderAbility
 * 모든 사령관 전용 스킬(기지 소환, 프리즈, 레벨업 등)의 기반 클래스입니다.
 * 플레이어의 기지 참조 및 자원(Watt/SP) 확인 공통 로직을 포함합니다.
 */

UCLASS()
class NOVAREVOLUTION_API UNovaCommanderAbility : public UNovaGameplayAbility
{
	GENERATED_BODY()
	
public:
	UNovaCommanderAbility();

protected:
	/** 어빌리티를 실행하는 플레이어의 메인 기지(ANovaBase)를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Nova|Ability|Commander")
	class ANovaBase* GetPlayerBase() const;

	/** 현재 플레이어의 자원(Watt, SP)이 충분한지 확인합니다. */
	UFUNCTION(BlueprintPure, Category = "Nova|Ability|Commander")
	bool CheckCommanderResources(float WattCost, float SPCost) const;

	/** 자원 소모가 가능한지 확인하고, 부족할 경우 경고 UI용 이벤트를 발생시킬 수 있습니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Ability|Commander")
	bool K2_CheckAndNotifyResources(float WattCost, float SPCost);
	
};
