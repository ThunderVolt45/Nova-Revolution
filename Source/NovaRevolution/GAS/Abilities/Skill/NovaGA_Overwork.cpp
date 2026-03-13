// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Skill/NovaGA_Overwork.h"
#include "AbilitySystemComponent.h"
#include "NovaRevolution.h"
#include "Core/NovaPlayerState.h"
#include "GAS/NovaGameplayTags.h"
#include "Core/NovaLog.h"
#include "GAS/NovaResourceAttributeSet.h"

UNovaGA_Overwork::UNovaGA_Overwork()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UNovaGA_Overwork::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    ANovaPlayerState* PS = Cast<ANovaPlayerState>(ActorInfo->OwnerActor.Get());
    if (!ASC || !PS) return;
    
    // 1. 자원 체크 (Watt 0, SP 20)
    // 부모 클래스(UNovaSkillAbility)의 기능을 활용합니다.
    if (!CheckResources(0.0f, 20.0f))
    {
        NOVA_SCREEN(Warning, "Insufficient Resources! Need 500 Watt, 20 SP.");
        
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 2. 필요한 수치 계산
    float CurrentWatt = PS->GetCurrentWatt();
    float MaxWatt = PS->GetMaxWatt();
    float RefillAmount = MaxWatt - CurrentWatt;

    // 3. 생산 중지 시간 계산 (부족분 / 생산속도 * 1.3)
    float WattLevel = PS->GetWattLevel();
    float RegenRate = GetCurrentWattRegenRate(WattLevel);
    float StopDuration = (RefillAmount / RegenRate) * 1.3f;

    // 4. 비용 지불 (기존 SetByCaller GE 적용)
    FGameplayEffectSpecHandle CostSpec = ASC->MakeOutgoingSpec(ModifyResourceGEClass, 1.0f, ASC->MakeEffectContext());
    if (CostSpec.IsValid())
    {
        CostSpec.Data->SetSetByCallerMagnitude(NovaGameplayTags::Data_Resource_Watt, -0.0f);
        CostSpec.Data->SetSetByCallerMagnitude(NovaGameplayTags::Data_Resource_SP, -20.0f);
        ASC->ApplyGameplayEffectSpecToSelf(*CostSpec.Data.Get());
    }

    // 5. Watt 즉시 회복 (SetByCaller 활용)
    if (WattRefillGEClass)
    {
        FGameplayEffectSpecHandle RefillSpec = ASC->MakeOutgoingSpec(WattRefillGEClass, 1.0f, ASC->MakeEffectContext());
        if (RefillSpec.IsValid())
        {
            RefillSpec.Data->SetSetByCallerMagnitude(NovaGameplayTags::Data_Resource_Watt, RefillAmount);
            ASC->ApplyGameplayEffectSpecToSelf(*RefillSpec.Data.Get());
        }
    }

    // 6. 생산 중지 효과 적용 (동적 시간 설정)
    if (WattStopGEClass && StopDuration > 0.0f)
    {
        FGameplayEffectSpecHandle StopSpec = ASC->MakeOutgoingSpec(WattStopGEClass, 1.0f, ASC->MakeEffectContext());
        if (StopSpec.IsValid())
        {
            // 새로 만드신 태그를 사용하여 계산된 시간을 주입합니다.
            StopSpec.Data->SetSetByCallerMagnitude(NovaGameplayTags::Data_Resource_Duration_WattStop, StopDuration);

            // 주의: 이제 SetDuration(StopDuration, false) 코드는 생략해도 됩니다.
            // GE 내부에서 Set by Caller가 이미 이 태그를 지속 시간으로 쓰겠다고 설정되어 있지 때문입니다.

            ASC->ApplyGameplayEffectSpecToSelf(*StopSpec.Data.Get());
        }
    }

    NOVA_SCREEN(Log, "Base Overwork Activated: Refilled %.0f, Stopped for %.2f sec", RefillAmount, StopDuration);
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

float UNovaGA_Overwork::GetCurrentWattRegenRate(float WattLevel) const
{
    if (!WattRegenGEClass)
    {
        NOVA_SCREEN(Error, "BaseOverwork: WattRegenGEClass is NOT assigned!");
        return 1.0f;
    }

    // 1. GE 클래스의 기본 객체(CDO)를 가져옵니다.
    const UGameplayEffect* GE = WattRegenGEClass->GetDefaultObject<UGameplayEffect>();

    // 2. Period(주기)를 동적으로 가져옵니다. (ScalableFloat 타입이므로 GetValueAtLevel 사용)
    float ActualPeriod = GE->Period.GetValueAtLevel(WattLevel);

    // 안전장치: 주기가 0이면 무한루프 방지를 위해 기본값 설정
    if (ActualPeriod <= 0.f) ActualPeriod = 0.05f;

    // 3. Modifier 목록에서 CurrentWatt 속성을 수정하는 항목을 찾아 값을 가져옵니다.
    float ValueAtLevel = 0.0f;
    bool bFoundModifier = false;

    for (const FGameplayModifierInfo& Mod : GE->Modifiers)
    {
        // 이 Modifier가 CurrentWatt 속성을 대상으로 하는지 확인
        if (Mod.Attribute == UNovaResourceAttributeSet::GetCurrentWattAttribute())
        {
            // Modifier의 Magnitude(수치)를 현재 레벨(WattLevel) 기준으로 추출
            // ScalableFloat(CurveTable 포함)인 경우 GetStaticMagnitudeIfPossible로 쉽게 가져올 수 있습니다.
            Mod.ModifierMagnitude.GetStaticMagnitudeIfPossible(WattLevel, ValueAtLevel);
            bFoundModifier = true;
            break;
        }
    }

    if (!bFoundModifier || ValueAtLevel <= 0.f)
    {
        NOVA_SCREEN(Warning, "BaseOverwork: Could not find valid Modifier in GE_WattRegen!");
        return 1.0f;
    }

    // [최종 계산] 초당 재생량 = (주기마다 차오르는 양 / 주기 시간)
    // 예: 0.05초당 1.0 차오르면 -> 1.0 / 0.05 = 초당 20.0
    return ValueAtLevel / ActualPeriod;
}