// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/NovaGameplayAbility.h"
#include "NovaGA_Attack.generated.h"

/**
 * UNovaGA_Attack
 * 유닛의 기본 공격을 담당하는 어빌리티 클래스입니다.
 * 히트스캔 또는 발사체 생성을 처리하며, 연사 속도에 따른 쿨다운을 적용합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaGA_Attack : public UNovaGameplayAbility
{
	GENERATED_BODY()

public:
	UNovaGA_Attack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** 어빌리티 종료 처리 */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 실제 공격 로직 (히트스캔/발사체) 실행 */
	void ExecuteAttack(AActor* Target);

protected:
	/** 공격 시 재생할 몽타주 (필요 시) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Combat")
	TObjectPtr<UAnimMontage> AttackMontage;

	/** 발사체 클래스 (발사체형 무기일 경우) */
	// UPROPERTY(EditDefaultsOnly, Category = "Nova|Combat")
	// TSubclassOf<class ANovaProjectile> ProjectileClass;
};
