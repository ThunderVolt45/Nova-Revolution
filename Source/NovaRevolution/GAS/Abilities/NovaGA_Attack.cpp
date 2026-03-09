// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/NovaGA_Attack.h"
#include "AbilitySystemComponent.h"
#include "NovaRevolution.h"
#include "GAS/NovaGameplayTags.h"

UNovaGA_Attack::UNovaGA_Attack()
{
	// 어빌리티 태그 설정 (SetAssetTags 권장 API 사용)
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(NovaGameplayTags::Ability_Attack);
	SetAssetTags(AssetTags);
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

	// 1. 공격자(Source)와 대상(Target)의 ASC를 가져옵니다.
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = Target->FindComponentByClass<UAbilitySystemComponent>();

	if (SourceASC && TargetASC && DamageEffectClass)
	{
		// 2. GameplayEffectSpec 생성 (레벨은 1로 고정)
		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			// 3. 대상에게 이펙트 적용 (히트스캔 방식: 즉시 적용)
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			
			NOVA_LOG(Log, "GA_Attack: Damage GE applied to %s via Hitscan", *Target->GetName());
		}
	}

	// TODO: GameplayCue 호출 (사격 이펙트/사운드)
	// ExecuteGameplayCueWithParams(NovaGameplayTags::GameplayCue_Unit_Fire, FGameplayCueParameters());
}
