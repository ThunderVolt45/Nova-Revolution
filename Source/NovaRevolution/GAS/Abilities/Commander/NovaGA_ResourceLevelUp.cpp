// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Commander/NovaGA_ResourceLevelUp.h"
#include "AbilitySystemComponent.h"
#include "Core/NovaPlayerState.h"
#include "GAS/NovaGameplayTags.h"

UNovaGA_ResourceLevelUp::UNovaGA_ResourceLevelUp()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UNovaGA_ResourceLevelUp::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    // 0. 기본 유효성 검사
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

    // 1. 현재 레벨 확인 (6단계 제한)
    float CurrentLevel = PS->GetWattLevel();
    if (CurrentLevel >= 6.0f)
    {
        // TODO: UI에 '최대 레벨' 메시지 출력 로직 추가 가능
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 2. 자원 체크 (Watt 500, SP 20)
    // 부모 클래스(UNovaCommanderAbility)의 기능을 활용합니다.
    if (!CheckCommanderResources(500.0f, 20.0f))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 3. 비용 지불 (기존 SetByCaller GE 적용)
    FGameplayEffectSpecHandle CostSpec = ASC->MakeOutgoingSpec(ModifyResourceGEClass, 1.0f, ASC->MakeEffectContext());
    if (CostSpec.IsValid())
    {
        CostSpec.Data->SetSetByCallerMagnitude(NovaGameplayTags::Data_Resource_Watt, -500.0f);
        CostSpec.Data->SetSetByCallerMagnitude(NovaGameplayTags::Data_Resource_SP, -20.0f);
        ASC->ApplyGameplayEffectSpecToSelf(*CostSpec.Data.Get());
    }

    // 4. 레벨 속성 수치 증가 (GE_ResourceLevelUp_Increment 적용)
    if (LevelUpIncrementGEClass)
    {
        FGameplayEffectSpecHandle LevelUpSpec = ASC->MakeOutgoingSpec(LevelUpIncrementGEClass, 1.0f, ASC->MakeEffectContext());
        if (LevelUpSpec.IsValid())
        {
            ASC->ApplyGameplayEffectSpecToSelf(*LevelUpSpec.Data.Get());
        }
    }

    // 5. 자원 재생 GE 갱신
    // 기존 재생 GE 제거 (Effect.Resource.Regen 태그를 가진 효과들)
    ASC->RemoveActiveEffectsWithTags(FGameplayTagContainer(NovaGameplayTags::Effect_Resource_Regen));

    // 새로운 레벨(CurrentLevel + 1)로 재생 GE 적용
    if (ResourceRegenGEClass)
    {
        // 1.0f 대신 '현재 레벨 + 1'을 인자로 넘겨 CurveTable의 다음 행을 읽게 함
        FGameplayEffectSpecHandle RegenSpec = ASC->MakeOutgoingSpec(ResourceRegenGEClass, CurrentLevel + 1.0f, ASC->MakeEffectContext());
        if (RegenSpec.IsValid())
        {
            ASC->ApplyGameplayEffectSpecToSelf(*RegenSpec.Data.Get());
        }
    }

    // 6. 어빌리티 종료
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}