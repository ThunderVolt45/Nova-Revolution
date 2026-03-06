// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/NovaResourceAttributeSet.h"

UNovaResourceAttributeSet::UNovaResourceAttributeSet()
{
	InitCurrentWatt(600.f);
	InitMaxWatt(2000.f);
	InitCurrentSP(40.f);
	InitMaxSP(100.f);
	InitMaxPopulation(20.f);
	InitMaxUnitWatt(6000.f);
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
	else if (Attribute == GetWattLevelAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f); // 최소 레벨 1 유지
	}
	else if (Attribute == GetSPLevelAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f); // 최소 레벨 1 유지
	}
}
