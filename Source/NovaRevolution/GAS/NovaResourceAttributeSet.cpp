// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/NovaResourceAttributeSet.h"

UNovaResourceAttributeSet::UNovaResourceAttributeSet()
{
}

void UNovaResourceAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 최대값에 맞춰 클램핑 처리
	if (Attribute == GetCurrentWattAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxWatt());
	}
	else if (Attribute == GetCurrentSPAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxSP());
	}
	else if (Attribute == GetCurrentPopulationAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxPopulation());
	}
	else if (Attribute == GetTotalUnitWattAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxUnitWatt());
	}
}
