// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/Skill/NovaSkillAbility.h"
#include "NovaGA_Recycle.generated.h"

/**
 * UNovaGA_Recycle
 * 선택된 유닛을 분해하여 소모된 와트의 일부를 반환받는 스킬입니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaGA_Recycle : public UNovaSkillAbility
{
	GENERATED_BODY()

public:
	UNovaGA_Recycle();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 와트 반환 비율 (기본 0.6 = 60%) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nova|Skill|Recycle")
	float RefundRate = 0.6f;

	/** 분해 시 재생할 시각 효과 태그 (GameplayCue) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Skill|Recycle")
	FGameplayTag RecycleCueTag;
};
