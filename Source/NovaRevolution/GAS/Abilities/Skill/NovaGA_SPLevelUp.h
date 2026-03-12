// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/Skill/NovaSkillAbility.h"
#include "NovaGA_SPLevelUp.generated.h"

/**
 * 
 */
UCLASS()
class NOVAREVOLUTION_API UNovaGA_SPLevelUp : public UNovaSkillAbility
{
	GENERATED_BODY()

public:
	UNovaGA_SPLevelUp();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** SP 레벨 속성을 1 올리는 GE (GE_SPLevelUp_Increment) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|LevelUp")
	TSubclassOf<class UGameplayEffect> SPLevelUpIncrementGEClass;

	/** SP 재생 Infinite GE (GE_SPRegen) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|LevelUp")
	TSubclassOf<class UGameplayEffect> SPRegenGEClass;

	/** 비용 소모 GE */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|LevelUp")
	TSubclassOf<class UGameplayEffect> ModifyResourceGEClass;
};
