// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/Skill/NovaSkillAbility.h"
#include "NovaGA_WattLevelUp.generated.h"

/**
 * 
 */
UCLASS()
class NOVAREVOLUTION_API UNovaGA_WattLevelUp : public UNovaSkillAbility
{
	GENERATED_BODY()
	
public:
	UNovaGA_WattLevelUp();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** Watt 레벨 속성을 1 올리는 GE (GE_WattLevelUp_Increment) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|LevelUp")
	TSubclassOf<class UGameplayEffect> WattLevelUpIncrementGEClass;

	/** Watt 재생 Infinite GE (GE_WattRegen) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|LevelUp")
	TSubclassOf<class UGameplayEffect> WattRegenGEClass;

	/** 비용 소모 GE (SetByCaller) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|LevelUp")
	TSubclassOf<class UGameplayEffect> ModifyResourceGEClass;
	
};
