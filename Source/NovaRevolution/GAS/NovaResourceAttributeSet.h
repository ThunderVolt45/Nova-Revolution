// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "NovaResourceAttributeSet.generated.h"

// 속성 접근을 위한 매크로 정의
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 플레이어의 자원(Watt, SP, Population)을 관리하는 Attribute Set
 */
UCLASS()
class NOVAREVOLUTION_API UNovaResourceAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UNovaResourceAttributeSet();

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	// 현재 와트 (Current Watt)
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Resources")
	FGameplayAttributeData CurrentWatt;
	ATTRIBUTE_ACCESSORS(UNovaResourceAttributeSet, CurrentWatt)

	// 최대 와트 제한 (Max Watt Capacity)
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Resources")
	FGameplayAttributeData MaxWatt;
	ATTRIBUTE_ACCESSORS(UNovaResourceAttributeSet, MaxWatt)

	// 현재 스킬 포인트 (Current Skill Points)
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Resources")
	FGameplayAttributeData CurrentSP;
	ATTRIBUTE_ACCESSORS(UNovaResourceAttributeSet, CurrentSP)

	// 최대 스킬 포인트 제한 (Max Skill Points Capacity)
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Resources")
	FGameplayAttributeData MaxSP;
	ATTRIBUTE_ACCESSORS(UNovaResourceAttributeSet, MaxSP)

	// 현재 개체 수 (Current Unit Count)
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Resources")
	FGameplayAttributeData CurrentPopulation;
	ATTRIBUTE_ACCESSORS(UNovaResourceAttributeSet, CurrentPopulation)

	// 최대 개체 수 제한 (Max Unit Count Capacity)
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Resources")
	FGameplayAttributeData MaxPopulation;
	ATTRIBUTE_ACCESSORS(UNovaResourceAttributeSet, MaxPopulation)

	// 현재 필드 위 유닛들의 와트 총합 (Total Watt of Units on Field)
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Resources")
	FGameplayAttributeData TotalUnitWatt;
	ATTRIBUTE_ACCESSORS(UNovaResourceAttributeSet, TotalUnitWatt)

	// 필드 위 유닛들의 와트 총합 제한 (Max Total Watt Capacity)
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Resources")
	FGameplayAttributeData MaxUnitWatt;
	ATTRIBUTE_ACCESSORS(UNovaResourceAttributeSet, MaxUnitWatt)

	// 플레이어 와트 생산 레벨 (Watt Production Level)
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Resources")
	FGameplayAttributeData WattLevel;
	ATTRIBUTE_ACCESSORS(UNovaResourceAttributeSet, WattLevel)

	// 플레이어 SP 생산 레벨 (SP Production Level)
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Resources")
	FGameplayAttributeData SPLevel;
	ATTRIBUTE_ACCESSORS(UNovaResourceAttributeSet, SPLevel)
};
