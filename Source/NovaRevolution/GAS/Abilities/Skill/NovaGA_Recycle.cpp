#include "GAS/Abilities/Skill/NovaGA_Recycle.h"
#include "Player/NovaPlayerController.h"
#include "Core/NovaUnit.h"
#include "AbilitySystemComponent.h"
#include "NovaRevolution.h"
#include "GAS/NovaAttributeSet.h"
#include "GAS/NovaGameplayTags.h"
#include "Core/NovaLog.h"
#include "GameFramework/PlayerState.h"
#include "Core/NovaPlayerState.h"
#include "Core/NovaInterfaces.h" // INovaTeamInterface 정의가 있는 곳

UNovaGA_Recycle::UNovaGA_Recycle()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    // 기본 비용 설정 (BP에서 수정 가능)
    WattCost = 0.0f;
    SPCost = 20.0f;
}

void UNovaGA_Recycle::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    // 0. 부모 클래스의 기본 활성화 로직 실행
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // 1. 자원(SP) 체크 (부족하면 즉시 종료 및 알림)
    if (!CheckSkillCost())
    {
        return;
    }

    // 2. 타겟 유닛 결정 (TriggerEventData 우선, 없으면 PC 선택 유닛)
    ANovaUnit* TargetUnit = nullptr;

    if (TriggerEventData && TriggerEventData->Target)
    {
        TargetUnit = const_cast<ANovaUnit*>(Cast<ANovaUnit>(TriggerEventData->Target));
    }

    if (!TargetUnit)
    {
        ANovaPlayerController* PC = Cast<ANovaPlayerController>(ActorInfo->PlayerController.Get());
        if (PC)
        {
            const TArray<AActor*>& SelectedUnits = PC->GetSelectedUnits();
            int32 TeamID = NovaTeam::None;
            if (INovaTeamInterface* TI = Cast<INovaTeamInterface>(PC->PlayerState)) TeamID = TI->GetTeamID();

            for (AActor* Actor : SelectedUnits)
            {
                if (ANovaUnit* Unit = Cast<ANovaUnit>(Actor))
                {
                    if (Unit->GetTeamID() == TeamID && !Unit->IsDead())
                    {
                        TargetUnit = Unit;
                        break;
                    }
                }
            }
        }
    }

    // 대상이 없으면 스킬 취소
    if (!TargetUnit)
    {
        NOVA_LOG(Warning, "Recycle Ability: No target unit found (EventData or Selected).");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 4. 반환받을 와트량 계산 (유닛의 Watt 스탯 * RefundRate)
    UAbilitySystemComponent* UnitASC = TargetUnit->GetAbilitySystemComponent();
    if (!UnitASC)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    float OriginalWatt = UnitASC->GetNumericAttribute(UNovaAttributeSet::GetWattAttribute());
    float RefundAmount = OriginalWatt * RefundRate;

    // 5. 자원 소모 및 와트 반환 적용 (GameplayEffect 사용)
    UAbilitySystemComponent* PlayerASC = GetAbilitySystemComponentFromActorInfo();
    if (PlayerASC && ModifyResourceGEClass)
    {
        FGameplayEffectSpecHandle SpecHandle = PlayerASC->MakeOutgoingSpec(ModifyResourceGEClass, 1.0f, PlayerASC->MakeEffectContext());
        if (SpecHandle.IsValid())
        {
            //반환 와트량 디버그메세지
            //NOVA_SCREEN(Warning, "반환 와트량 : %f", RefundAmount);
            
            // SP 소모 (음수) 및 Watt 반환 (양수)
            SpecHandle.Data->SetSetByCallerMagnitude(NovaGameplayTags::Data_Resource_SP, -SPCost);
            SpecHandle.Data->SetSetByCallerMagnitude(NovaGameplayTags::Data_Resource_Watt, RefundAmount);

            PlayerASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
        }
    }

    // 6. 시각 효과 실행
    if (RecycleCueTag.IsValid())
    {
        FGameplayCueParameters Params;
        Params.Location = TargetUnit->GetActorLocation();
        PlayerASC->ExecuteGameplayCue(RecycleCueTag, Params);
    }

    // 7. 유닛 분해 (Die 호출로 인구수/총 와트 반납 및 오브젝트 풀 복귀 처리)
    TargetUnit->Die();

    // 8. 어빌리티 종료
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

