// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/Commander/NovaSkillAbility.h"
#include "NovaGA_ResourceLevelUp.generated.h"

/**
 * UNovaGA_ResourceLevelUp
 * 사령관의 자원(Watt/SP) 생산 레벨을 올리고 재생 GE를 갱신하는 스킬 어빌리티입니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaGA_ResourceLevelUp : public UNovaSkillAbility
{
	GENERATED_BODY()

public:
	UNovaGA_ResourceLevelUp();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** 자원 레벨(Attribute)을 1 올리는 Instant GE (GE_ResourceLevelUp_Increment) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|LevelUp")
	TSubclassOf<class UGameplayEffect> LevelUpIncrementGEClass;

	/** 자원 재생을 담당하는 Infinite GE (GE_ResourceRegen) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|LevelUp")
	TSubclassOf<class UGameplayEffect> ResourceRegenGEClass;

	/** 자원 소모(비용)를 담당하는 GE (SetByCaller 방식) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|LevelUp")
	TSubclassOf<class UGameplayEffect> ModifyResourceGEClass;
};
