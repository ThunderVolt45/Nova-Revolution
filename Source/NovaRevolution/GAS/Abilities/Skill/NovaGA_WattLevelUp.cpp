// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/Skill/NovaGA_WattLevelUp.h"
#include "AbilitySystemComponent.h"
#include "NovaRevolution.h"
#include "Core/NovaPlayerState.h"
#include "GAS/NovaGameplayTags.h"
#include "Core/NovaLog.h"

UNovaGA_WattLevelUp::UNovaGA_WattLevelUp()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UNovaGA_WattLevelUp::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    ANovaPlayerState* PS = Cast<ANovaPlayerState>(ActorInfo->OwnerActor.Get());

    if (!ASC || !PS)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 1. 현재 Watt 레벨 확인 (6단계 제한)
    float CurrentWattLevel = PS->GetWattLevel();
    if (CurrentWattLevel >= 6.0f)
    {
        NOVA_SCREEN(Warning, "Watt Level is already at MAX!");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 2. 자원 체크 및 소모 (Watt 500, SP 20 - 수치는 기획에 따라 조정)
    if (!CheckResources(500.0f, 20.0f))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 3. 비용 지불
    FGameplayEffectSpecHandle CostSpec = ASC->MakeOutgoingSpec(ModifyResourceGEClass, 1.0f, ASC->MakeEffectContext());
    if (CostSpec.IsValid())
    {
        CostSpec.Data->SetSetByCallerMagnitude(NovaGameplayTags::Data_Resource_Watt, -500.0f);
        CostSpec.Data->SetSetByCallerMagnitude(NovaGameplayTags::Data_Resource_SP, -20.0f);
        ASC->ApplyGameplayEffectSpecToSelf(*CostSpec.Data.Get());
    }

    // 4. Watt 레벨 속성 증가
    if (WattLevelUpIncrementGEClass)
    {
        ASC->ApplyGameplayEffectSpecToSelf(*ASC->MakeOutgoingSpec(WattLevelUpIncrementGEClass, 1.0f, ASC->MakeEffectContext()).Data.Get());
    }

    // 5. Watt 재생 GE만 교체 (중요: Watt 전용 태그 사용)
    ASC->RemoveActiveEffectsWithTags(FGameplayTagContainer(NovaGameplayTags::Effect_Resource_Regen_Watt));

    if (WattRegenGEClass)
    {
        // 새로운 Watt 레벨(Current + 1) 적용
        FGameplayEffectSpecHandle RegenSpec = ASC->MakeOutgoingSpec(WattRegenGEClass, CurrentWattLevel + 1.0f, ASC->MakeEffectContext());
        if (RegenSpec.IsValid())
        {
            ASC->ApplyGameplayEffectSpecToSelf(*RegenSpec.Data.Get());
        }
    }

    NOVA_SCREEN(Log, "WATT LEVEL UP: %.0f -> %.0f", CurrentWattLevel, CurrentWattLevel + 1.0f);
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}