// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/NovaGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTargetTypes.h" // TargetDataHandle 처리를 위해 추가

UNovaGameplayAbility::UNovaGameplayAbility()
{
	// 인스턴스화 정책: 액터당 하나씩 (RTS 환경에서 보편적)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UNovaGameplayAbility::ExecuteGameplayCueWithParams(const FGameplayTag& CueTag, const FGameplayCueParameters& Parameters)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->ExecuteGameplayCue(CueTag, Parameters);
	}
}

AActor* UNovaGameplayAbility::GetTargetActorFromEventData(const FGameplayEventData& TriggerEventData) const
{
	// GAS 표준 방식: TargetDataHandle에서 첫 번째 액터를 추출
	if (TriggerEventData.TargetData.Num() > 0)
	{
		return GetTargetActorFromEventData(TriggerEventData);
	}

	return nullptr;
}

FVector UNovaGameplayAbility::GetTargetLocationFromEventData(const FGameplayEventData& TriggerEventData) const
{
	// TargetData에 히트 결과가 있다면 해당 임팩트 지점을 반환
	if (TriggerEventData.TargetData.Num() > 0)
	{
		if (UAbilitySystemBlueprintLibrary::TargetDataHasHitResult(TriggerEventData.TargetData, 0))
		{
			FHitResult Hit = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TriggerEventData.TargetData, 0);
			return Hit.ImpactPoint;
		}
		
		// 히트 결과는 없지만 액터 정보가 있다면 액터 위치 반환
		if (AActor* TargetActor = GetTargetActorFromEventData(TriggerEventData))
		{
			return TargetActor->GetActorLocation();
		}
	}

	return FVector::ZeroVector;
}
