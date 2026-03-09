// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "NovaDamageCalculation.generated.h"

/**
 * UNovaDamageCalculation
 * 유닛의 공격력(Source)과 대상의 방어력(Target)을 참조하여 최종 데미지를 계산합니다.
 * 공식: Max(0, Attack - Defense)
 */
UCLASS()
class NOVAREVOLUTION_API UNovaDamageCalculation : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UNovaDamageCalculation();

	/** 실제 데미지 계산 로직 */
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	/** 공격자의 공격력(Attack) 캡처 정의 */
	FGameplayEffectAttributeCaptureDefinition AttackDef;
	
	/** 대상의 방어력(Defense) 캡처 정의 */
	FGameplayEffectAttributeCaptureDefinition DefenseDef;
};
