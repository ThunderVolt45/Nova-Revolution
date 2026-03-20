// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/Skill/NovaGA_SPLevelUp.h"
#include "AbilitySystemComponent.h"
#include "NovaRevolution.h"
#include "Core/NovaPlayerState.h"
#include "GAS/NovaGameplayTags.h"
#include "Core/NovaLog.h"

UNovaGA_SPLevelUp::UNovaGA_SPLevelUp()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    // 통합 GCN 타겟 설정
    GCNTargetType = ENovaSkillGCNTargetType::Base;
}

void UNovaGA_SPLevelUp::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        NOVA_SCREEN(Error, "SPLevelUp: Ability Commit Failed!");
        
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    ANovaPlayerState* PS = Cast<ANovaPlayerState>(ActorInfo->OwnerActor.Get());

    if (!ASC || !PS)
    {
        NOVA_SCREEN(Error, "SPLevelUp: Internal Error - ASC or PS is NULL!");
        
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 1. 현재 SP 레벨 확인 (6단계 제한)
    float CurrentSPLevel = PS->GetSPLevel();
    if (CurrentSPLevel >= 6.0f)
    {
        NOVA_SCREEN(Warning, "SP Level is already at MAX!");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // 2. 자원 체크 및 3. 비용 지불
    if (!CheckSkillCost()) return; // 블루프린트 설정값으로 자원 체크
    
    ApplySkillCost();              // 한 줄로 공통 소모 처리

    // 4. SP 레벨 속성 증가
    if (SPLevelUpIncrementGEClass)
    {
        ASC->ApplyGameplayEffectSpecToSelf(*ASC->MakeOutgoingSpec(SPLevelUpIncrementGEClass, 1.0f, ASC->MakeEffectContext()).Data.Get());
    }

    // 5. SP 재생 GE만 교체 (중요: SP 전용 태그 사용)
    ASC->RemoveActiveEffectsWithTags(FGameplayTagContainer(NovaGameplayTags::Effect_Resource_Regen_SP));

    if (SPRegenGEClass)
    {
        // 새로운 SP 레벨(Current + 1) 적용
        FGameplayEffectSpecHandle RegenSpec = ASC->MakeOutgoingSpec(SPRegenGEClass, CurrentSPLevel + 1.0f, ASC->MakeEffectContext());
        if (RegenSpec.IsValid())
        {
            ASC->ApplyGameplayEffectSpecToSelf(*RegenSpec.Data.Get());
        }
    }

    // 6. 시각 효과 실행 (통합 GCN 시스템 사용)
    ExecuteSkillGCN(FGameplayAbilityTargetDataHandle());
    
    // NOVA_SCREEN(Log, "SP LEVEL UP: %.0f -> %.0f", CurrentSPLevel, CurrentSPLevel + 1.0f);
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

