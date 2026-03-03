// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/NovaAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UNovaAttributeSet::UNovaAttributeSet()
{
	// 기본값 초기화
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	InitDamage(0.0f);
	InitWatt(100.0f);
	InitAttack(10.0f);
	InitDefense(0.0f);
	InitSpeed(300.0f);
	InitFireRate(1.0f);
	InitSight(2400.0f);
	InitRange(1800.0f);
	InitMinRange(0.0f);
	InitSplashRange(0.0f);
}

void UNovaAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 체력이 MaxHealth를 초과하지 않도록, 그리고 0보다 작아지지 않도록 제한
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	// 최대 체력은 최소 1 이상 유지
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	// 주요 전투 스탯들의 최소치 제한 (0 미만으로 떨어지지 않게 보호)
	else if (Attribute == GetAttackAttribute() || Attribute == GetDefenseAttribute() || 
			 Attribute == GetSpeedAttribute() || Attribute == GetFireRateAttribute() ||
			 Attribute == GetSightAttribute() || Attribute == GetRangeAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UNovaAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 데미지 메타 속성 처리
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		// 적용된 데미지 추출 및 초기화
		const float LocalDamageDone = GetDamage();
		SetDamage(0.0f);

		if (LocalDamageDone > 0.0f)
		{
			// 노바 1492 데미지 공식: 실제 데미지 = 원본 데미지 - 방어력 (최소 0)
			const float ActualDamage = FMath::Max(LocalDamageDone - GetDefense(), 0.0f);
			
			// 현재 체력 반영 (PreAttributeChange에서 이미 클램핑 로직이 있지만 명시적으로 SetHealth 사용)
			const float NewHealth = GetHealth() - ActualDamage;
			SetHealth(FMath::Clamp(NewHealth, 0.0f, GetMaxHealth()));

			// 데미지 출력 (디버그 로그 - 추후 UI 및 이펙트 연동 포인트)
			UE_LOG(LogTemp, Log, TEXT("Unit Damage: Raw(%f) - Def(%f) = Actual(%f). HP: %f/%f"), 
				LocalDamageDone, GetDefense(), ActualDamage, GetHealth(), GetMaxHealth());

			// 사망 처리 체크
			if (GetHealth() <= 0.0f)
			{
				// TODO: 유닛 사망 처리 로직 호출 (예: Actor의 Die() 함수 등)
				UE_LOG(LogTemp, Error, TEXT("Unit has died!"));
			}
		}
	}
	// MaxHealth 변경 시 현재 Health가 범위를 벗어나지 않도록 조정
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
}
