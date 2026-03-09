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
	// 1. Target 필드에 직접 액터가 있는 경우 (BTTask 등에서 직접 할당 시)
	if (TriggerEventData.Target)
	{
		return const_cast<AActor*>(TriggerEventData.Target.Get());
	}

	// 2. TargetDataHandle에 데이터가 있는 경우 (표준 GAS 타겟 데이터)
	if (TriggerEventData.TargetData.Num() > 0)
	{
		TArray<AActor*> Actors = UAbilitySystemBlueprintLibrary::GetActorsFromTargetData(TriggerEventData.TargetData, 0);
		if (Actors.Num() > 0 && Actors[0])
		{
			return Actors[0];
		}
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
