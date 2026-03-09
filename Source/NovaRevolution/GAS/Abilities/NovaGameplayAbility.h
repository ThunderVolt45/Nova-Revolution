// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NovaGameplayAbility.generated.h"

/**
 * UNovaGameplayAbility
 * Nova Revolution 프로젝트의 모든 Gameplay Ability(유닛 공격, 사령관 스킬 등)의 기반이 되는 클래스입니다.
 * 공통적인 타겟 데이터 추출 및 GameplayCue 호출 기능을 제공합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UNovaGameplayAbility();

protected:
	/** 특정 태그와 파라미터를 사용하여 GameplayCue를 즉시 실행하는 헬퍼 함수 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Ability")
	void ExecuteGameplayCueWithParams(const FGameplayTag& CueTag, const FGameplayCueParameters& Parameters);

	/** EventData(TriggerEventData)로부터 타겟 액터를 안전하게 추출합니다. */
	UFUNCTION(BlueprintPure, Category = "Nova|Ability")
	AActor* GetTargetActorFromEventData(const FGameplayEventData& TriggerEventData) const;

	/** EventData(TriggerEventData)로부터 타겟 위치를 안전하게 추출합니다. */
	UFUNCTION(BlueprintPure, Category = "Nova|Ability")
	FVector GetTargetLocationFromEventData(const FGameplayEventData& TriggerEventData) const;

protected:
	/** 이 어빌리티에 할당된 입력 태그 (UI 및 입력 컴포넌트 연동용) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nova|Ability")
	FGameplayTag AbilityInputTag;
};
