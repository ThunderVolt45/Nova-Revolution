// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Calculations/NovaDamageCalculation.h"
#include "GAS/NovaAttributeSet.h"
#include "GAS/NovaGameplayTags.h"

UNovaDamageCalculation::UNovaDamageCalculation()
{
	// 1. 공격자(Source)의 공격력(Attack) 속성 캡처 정의
	// bSnapshot=true: 발사체 발사 시점의 공격력을 캡처하여 저장합니다.
	AttackDef.AttributeToCapture = UNovaAttributeSet::GetAttackAttribute();
	AttackDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;
	AttackDef.bSnapshot = true;

	// 2. 대상(Target)의 방어력(Defense) 속성 캡처 정의
	// bSnapshot=false: 적중하는 순간의 실시간 방어력을 참조합니다.
	DefenseDef.AttributeToCapture = UNovaAttributeSet::GetDefenseAttribute();
	DefenseDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	DefenseDef.bSnapshot = false;

	// 계산 시스템에 캡처 정의 추가
	RelevantAttributesToCapture.Add(AttackDef);
	RelevantAttributesToCapture.Add(DefenseDef);
}

float UNovaDamageCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// 캡처한 속성값들을 가져오기 위한 태그 컨테이너
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// 공격력과 방어력 값을 실제 속성에서 가져옴
	float Attack = 0.0f;
	GetCapturedAttributeMagnitude(AttackDef, Spec, EvaluationParameters, Attack);

	float Defense = 0.0f;
	// 방어력 무시 태그(Effect.Damage.IgnoreDefense)가 있는지 확인
	if (Spec.CapturedSourceTags.GetAggregatedTags()->HasTag(NovaGameplayTags::Effect_Damage_IgnoreDefense))
	{
		Defense = 0.0f;
	}
	else
	{
		GetCapturedAttributeMagnitude(DefenseDef, Spec, EvaluationParameters, Defense);
	}

	// 공식: 최종 데미지 = Max(0, 공격력 - 방어력)
	const float FinalDamage = FMath::Max(0.0f, Attack - Defense);

	return FinalDamage;
}
