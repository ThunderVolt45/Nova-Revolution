// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/Skill/NovaSkillAbility.h"
#include "NovaGA_Overwork.generated.h"

/**
 * 
 */
UCLASS()
class NOVAREVOLUTION_API UNovaGA_Overwork : public UNovaSkillAbility
{
	GENERATED_BODY()
	
public:
	UNovaGA_Overwork();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** 실제 Watt 재생을 담당하는 GE 클래스 (GE_WattRegen) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Data")
	TSubclassOf<class UGameplayEffect> WattRegenGEClass;
	
	/** 부족한 Watt를 즉시 채워주는 Instant GE */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Skill")
	TSubclassOf<class UGameplayEffect> WattRefillGEClass;

	/** Watt 생산을 중단시키는 Duration GE (Effect.Resource.Block.Watt 태그 부여) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Skill")
	TSubclassOf<class UGameplayEffect> WattStopGEClass;

	/** 현재 와트 생산 속도를 가져오기 위한 함수 (데이터 에셋이나 계산 공식에 따라 조정 필요) */
	float GetCurrentWattRegenRate(float WattLevel) const;
	
};
