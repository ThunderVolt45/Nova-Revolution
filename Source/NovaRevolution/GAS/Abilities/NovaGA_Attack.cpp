// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/NovaGA_Attack.h"
#include "AbilitySystemComponent.h"
#include "NovaRevolution.h"
#include "GAS/NovaGameplayTags.h"

UNovaGA_Attack::UNovaGA_Attack()
{
	// 어빌리티 태그 설정
	AbilityTags.AddTag(NovaGameplayTags::Ability_Attack);
}

void UNovaGA_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 어빌리티 발동 가능 여부 확인 (비용, 쿨다운 등)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. 공격 대상 확인 (베이스 클래스의 헬퍼 함수 활용)
	if (!TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Target = GetTargetActorFromEventData(*TriggerEventData);
	if (!Target)
	{
		NOVA_LOG(Warning, "GA_Attack: Target is null. Attack Canceled.");
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 실제 공격 실행
	ExecuteAttack(Target);

	// 4. 어빌리티 종료 (단발성 공격인 경우 즉시 종료)
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UNovaGA_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UNovaGA_Attack::ExecuteAttack(AActor* Target)
{
	if (!Target) return;

	// TODO: GameplayCue 호출 (사격 이펙트/사운드)
	// ExecuteGameplayCueWithParams(NovaGameplayTags::GameplayCue_Unit_Fire, FGameplayCueParameters());

	// TODO: 유닛 타입에 따른 분기 (히트스캔 vs 발사체)
	// if (ProjectileClass) { // 발사체 생성 로직 }
	// else { // 히트스캔 로직 }

	NOVA_LOG(Log, "GA_Attack: Attack Executed on %s!", *Target->GetName());
}
