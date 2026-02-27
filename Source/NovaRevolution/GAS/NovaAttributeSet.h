// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "NovaAttributeSet.generated.h"

// 속성 접근을 위한 매크로 정의
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Nova Revolution 유닛들의 공통 속성을 관리하는 클래스
 */
UCLASS()
class NOVAREVOLUTION_API UNovaAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UNovaAttributeSet();

	// 체력 (Health Points)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UNovaAttributeSet, Health)

	// 와트 (생산 비용 및 현재 가치)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Watt;
	ATTRIBUTE_ACCESSORS(UNovaAttributeSet, Watt)

	// 공격력 (Attack Power)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Attack;
	ATTRIBUTE_ACCESSORS(UNovaAttributeSet, Attack)

	// 방어력 (Defense)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS(UNovaAttributeSet, Defense)

	// 이동 속도 (Movement Speed)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Speed;
	ATTRIBUTE_ACCESSORS(UNovaAttributeSet, Speed)

	// 연사 속도 (Fire Rate - 초당 공격 횟수 등)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData FireRate;
	ATTRIBUTE_ACCESSORS(UNovaAttributeSet, FireRate)

	// 시야 (Sight Range)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Sight;
	ATTRIBUTE_ACCESSORS(UNovaAttributeSet, Sight)

	// 사거리 (Attack Range)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Range;
	ATTRIBUTE_ACCESSORS(UNovaAttributeSet, Range)

	// 최소 사거리 (Minimum Attack Range)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MinRange;
	ATTRIBUTE_ACCESSORS(UNovaAttributeSet, MinRange)

	// 스플래시 범위 (Splash/Area of Effect Range)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData SplashRange;
	ATTRIBUTE_ACCESSORS(UNovaAttributeSet, SplashRange)
};
