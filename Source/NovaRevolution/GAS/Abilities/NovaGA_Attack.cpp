// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/NovaGA_Attack.h"
#include "AbilitySystemComponent.h"
#include "Core/NovaUnit.h"
#include "Core/NovaPart.h"
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

	ANovaUnit* Unit = Cast<ANovaUnit>(GetAvatarActorFromActorInfo());
	if (!Unit) return;

	// 1. 모든 무기 부품의 발사 효과(애니메이션 + 발사 Cue) 실행
	FGameplayTag ImpactTag;
	const TArray<TObjectPtr<UChildActorComponent>>& WeaponComps = Unit->GetWeaponPartComponents();
	
	for (auto WeaponComp : WeaponComps)
	{
		if (ANovaPart* WeaponPart = Cast<ANovaPart>(WeaponComp->GetChildActor()))
		{
			WeaponPart->PlayFireEffects();
			
			// 적중 효과 태그 저장 (첫 번째 무기 기준 혹은 모든 무기 공통 사용)
			if (!ImpactTag.IsValid())
			{
				ImpactTag = WeaponPart->GetImpactCueTag();
			}
		}
	}

	// 2. 공격자(Source)와 대상(Target)의 ASC를 가져옵니다.
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = Target->FindComponentByClass<UAbilitySystemComponent>();

	if (SourceASC && TargetASC && DamageEffectClass)
	{
		// GameplayEffectSpec 생성 (레벨은 1로 고정)
		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		ContextHandle.AddSourceObject(Unit);

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			// 대상에게 이펙트 적용 (히트스캔 방식: 즉시 적용)
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			
			NOVA_LOG(Log, "GA_Attack: Damage GE applied to %s via Hitscan", *Target->GetName());
		}
	}

	// 3. 적중 효과 (Impact GameplayCue) 실행 (히트스캔인 경우 타겟 위치에서 발동)
	if (ImpactTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = Target->GetActorLocation();
		Params.Instigator = Unit;
		Params.EffectCauser = Unit;
		
		ExecuteGameplayCueWithParams(ImpactTag, Params);
	}
}
