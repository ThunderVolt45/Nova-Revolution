// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/NovaAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Core/NovaUnit.h"
#include "NovaRevolution.h"

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
	InitFireRate(100.0f);
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
			 Attribute == GetSpeedAttribute() || 
			 Attribute == GetSightAttribute() || Attribute == GetRangeAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	// 연사 속도는 최소 25 이상 유지 (연사력이 너무 빨라지지 않도록)
	else if (Attribute == GetFireRateAttribute())
	{
		NewValue = FMath::Max(NewValue, 25.0f);
	}
}

void UNovaAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 데미지 메타 속성 처리
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		// MMC 등을 통해 계산된 최종 데미지량 추출
		const float LocalDamageDone = Data.EvaluatedData.Magnitude;
		
		// 메타 속성 초기화 (이벤트 통로 역할이므로 즉시 리셋)
		SetDamage(0.0f);

		if (LocalDamageDone > 0.0f)
		{
			// 현재 체력 반영 (방어력 계산은 이미 MMC에서 완료됨)
			const float NewHealth = GetHealth() - LocalDamageDone;
			SetHealth(FMath::Clamp(NewHealth, 0.0f, GetMaxHealth()));

			// 데미지 로그 출력
			// NOVA_LOG(Log, "Unit Damaged: %f. Remaining HP: %f/%f", LocalDamageDone, GetHealth(), GetMaxHealth());

			// 사망 처리 호출
			if (GetHealth() <= 0.0f)
			{
				if (ANovaUnit* Unit = Cast<ANovaUnit>(GetOwningActor()))
				{
					// 이미 죽었는지 확인하여 무한 루프 방지
					if (!Unit->IsDead())
					{
						Unit->Die();
					}
				}
			}
		}
	}
	// MaxHealth 변경 시 현재 Health가 범위를 벗어나지 않도록 조정
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	// 주요 전투 스탯들의 최소치 제한 (0 미만으로 떨어지지 않게 보호)
	else if (Data.EvaluatedData.Attribute == GetDefenseAttribute())
	{
		SetDefense(FMath::Max(GetDefense(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetAttackAttribute())
	{
		SetAttack(FMath::Max(GetAttack(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetSpeedAttribute())
	{
		SetSpeed(FMath::Max(GetSpeed(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetFireRateAttribute())
	{
		SetFireRate(FMath::Max(GetFireRate(), 25.0f));
	}
}

void UNovaAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// 최종 계산된 속성값이 최소치 미만으로 내려가지 않도록 최후의 보호 장치
	if (Attribute == GetDefenseAttribute() && NewValue < 0.0f)
	{
		SetDefense(0.0f);
	}
	else if (Attribute == GetAttackAttribute() && NewValue < 0.0f)
	{
		SetAttack(0.0f);
	}
	else if (Attribute == GetSpeedAttribute() && NewValue < 0.0f)
	{
		SetSpeed(0.0f);
	}
	else if (Attribute == GetFireRateAttribute() && NewValue < 25.0f)
	{
		SetFireRate(25.0f);
	}
	else if (Attribute == GetSightAttribute() && NewValue < 0.0f)
	{
		SetSight(0.0f);
	}
}
