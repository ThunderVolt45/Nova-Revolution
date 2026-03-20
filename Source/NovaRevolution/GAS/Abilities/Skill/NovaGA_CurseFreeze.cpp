// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/Skill/NovaGA_CurseFreeze.h"
#include "GAS/Targeting/NovaTargetActor_GroundRadius.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NovaRevolution.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Core/NovaUnit.h"
#include "Core/NovaLog.h"
#include "Player/NovaPlayerController.h"

UNovaGA_CurseFreeze::UNovaGA_CurseFreeze()
{
    // 요구사항: Watt 200, SP 15 (부모 클래스인 NovaSkillAbility의 변수 사용)
    WattCost = 200.0f;
    SPCost = 15.0f;

    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    // 통합 GCN 타겟 설정
    GCNTargetType = ENovaSkillGCNTargetType::TargetActors;
}

void UNovaGA_CurseFreeze::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // 1. 자원 체크 (Watt/SP 부족 시 종료)
    if (!CheckSkillCost())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 2. 컨트롤러를 스킬 모드로 전환 (마우스 커서 변경 및 다른 클릭 무시용)
    if (ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetActorInfo().PlayerController))
    {
        PC->SetPendingCommandType(ECommandType::Skill);
    }

    // 3. 타겟 액터 스폰 및 설정
    if (!TargetActorClass)
    {
        NOVA_SCREEN(Error, "TargetActorClass가 설정되지 않았습니다!");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ANovaTargetActor_GroundRadius* SpawnedTargetActor = GetWorld()->SpawnActor<ANovaTargetActor_GroundRadius>(TargetActorClass);
    if (SpawnedTargetActor)
    {
        SpawnedTargetActor->StartLocation = MakeTargetLocationInfoFromOwnerActor();
        SpawnedTargetActor->Radius = FreezeRadius;

        // [중요] 필터를 적군(Enemy)으로 설정! 아군 하이라이트와 차별화
        SpawnedTargetActor->TargetingFilter = ENovaTargetingFilter::Enemy;
        SpawnedTargetActor->CurrentHighlightColor = SkillHighlightColor;
    }

    // 4. 타겟 데이터 대기 태스크 (UserConfirmed: 좌클릭 시 확정)
    UAbilityTask_WaitTargetData* WaitTargetTask = UAbilityTask_WaitTargetData::WaitTargetDataUsingActor(
        this,
        TEXT("WaitTargetData_CurseFreeze"),
        EGameplayTargetingConfirmation::UserConfirmed,
        SpawnedTargetActor
    );

    if (WaitTargetTask)
    {
        WaitTargetTask->ValidData.AddDynamic(this, &UNovaGA_CurseFreeze::OnTargetDataReadyCallback);
        WaitTargetTask->Cancelled.AddDynamic(this, &UNovaGA_CurseFreeze::OnTargetDataCancelledCallback);
        WaitTargetTask->ReadyForActivation();
    }
}

void UNovaGA_CurseFreeze::OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& DataHandle)
{
    // 1. 자원 소모
    ApplySkillCost();

    // 2. 타겟 유닛들에게 효과 적용
    TArray<AActor*> TargetActors = UAbilitySystemBlueprintLibrary::GetAllActorsFromTargetData(DataHandle);

    // 비주얼 이펙트 실행 (통합 GCN 시스템 사용)
    ExecuteSkillGCN(DataHandle);

    for (AActor* Actor : TargetActors)
    {
        if (ANovaUnit* Unit = Cast<ANovaUnit>(Actor))
        {
            // GE_CurseFreeze 적용 (속도 0, State_Frozen 태그 부여)
            if (CurseFreezeGEClass)
            {
                FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CurseFreezeGEClass);
                if (SpecHandle.IsValid())
                {
                    ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle, DataHandle);
                }
            }
        }
    }

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UNovaGA_CurseFreeze::OnTargetDataCancelledCallback(const FGameplayAbilityTargetDataHandle& DataHandle)
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UNovaGA_CurseFreeze::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    // 어빌리티 종료 시 컨트롤러 상태 복구
    if (ANovaPlayerController* PC = Cast<ANovaPlayerController>(ActorInfo->PlayerController))
    {
        if (PC->GetPendingCommandType() == ECommandType::Skill)
        {
            PC->SetPendingCommandType(ECommandType::None);
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}