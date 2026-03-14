// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/Abilities/Skill/NovaGA_Recall.h"
#include "GAS/Targeting/NovaTargetActor_GroundRadius.h"
#include "Core/NovaBase.h"
#include "Core/NovaUnit.h"
#include "AbilitySystemComponent.h"
#include "NovaRevolution.h"
#include "Core/NovaLog.h"

UNovaGA_Recall::UNovaGA_Recall()
{
    // 요구사항: Watt 200, SP 15 -> 블루프린트에서 값 설정
    //WattCost = 200.0f;
    //SPCost = 15.0f;

    // 인스턴싱 정책 설정 (상태 저장을 위해 인스턴스화 필요)
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UNovaGA_Recall::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    // 0. 부모 클래스의 ActivateAbility 호출 (필수)
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // 1. 자원 체크 (부족하면 즉시 종료)
    if (!CheckSkillCost())
    {
        return;
    }

    // 2. 타겟 액터 클래스 유효성 검사
    if (!TargetActorClass)
    {
        NOVA_SCREEN(Error, "TargetActorClass is not assigned in Recall Ability!");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 3. [해결] 타겟 액터를 우리가 직접 먼저 스폰합니다.
    // (태스크의 protected 변수인 TargetActor에 접근하는 문제를 회피하기 위함)
    ANovaTargetActor_GroundRadius* SpawnedTargetActor = GetWorld()->SpawnActor<ANovaTargetActor_GroundRadius>(TargetActorClass);

    if (SpawnedTargetActor)
    {
        // 4. [해결] 제공해주신 헤더 확인 결과, StartLocation은 Public이므로 직접 대입 가능합니다.
        SpawnedTargetActor->StartLocation = MakeTargetLocationInfoFromOwnerActor();

        // 5. 반지름 및 필터 설정 (GA의 데이터를 액터에 주입)
        SpawnedTargetActor->Radius = RecallRadius;
        SpawnedTargetActor->TargetingFilter = ENovaTargetingFilter::Ally;
    }

    // 6. [해결] WaitTargetDataUsingActor를 사용하여 우리가 만든 액터를 태스크에 전달합니다.
    UAbilityTask_WaitTargetData* WaitTargetTask = UAbilityTask_WaitTargetData::WaitTargetDataUsingActor(
        this,
        TEXT("WaitTargetData_Recall"),
        EGameplayTargetingConfirmation::UserConfirmed,
        SpawnedTargetActor
    );

    if (WaitTargetTask)
    {
        // 7. 델리게이트 연결 (성공/취소)
        WaitTargetTask->ValidData.AddDynamic(this, &UNovaGA_Recall::OnTargetDataReadyCallback);
        WaitTargetTask->Cancelled.AddDynamic(this, &UNovaGA_Recall::OnTargetDataCancelledCallback);

        // 8. 태스크 활성화 (이 시점부터 타겟 액터가 월드에 나타나고 마우스를 추적합니다)
        WaitTargetTask->ReadyForActivation();
    }
    else
    {
        NOVA_SCREEN(Error, "Failed to create WaitTargetData Task!");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    }
}

void UNovaGA_Recall::OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& DataHandle)
{
    // 1. 자원 실제 소모
    ApplySkillCost();

    // 2. 기지 및 이동 목표지점(랠리 포인트) 확인
    ANovaBase* PlayerBase = GetPlayerBase();
    if (!PlayerBase)
    {
        NOVA_SCREEN(Error, "Player Base not found!");
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    FVector Destination = PlayerBase->GetRallyPoint();

    // 3. 타겟팅된 유닛들 처리
    //TArray<AActor*> TargetActors = DataHandle.GetActors();
    
    // 라이브러리 함수를 사용하여 핸들로부터 액터 배열을 한 번에 추출합니다.
    TArray<AActor*> TargetActors = UAbilitySystemBlueprintLibrary::GetAllActorsFromTargetData(DataHandle);
    
    for (AActor* Actor : TargetActors)
    {
        if (ANovaUnit* Unit = Cast<ANovaUnit>(Actor))
        {
            // (A) 비주얼 이펙트 실행 (GameplayCue)
            if (RecallCueTag.IsValid())
            {
                Unit->GetAbilitySystemComponent()->ExecuteGameplayCue(RecallCueTag);
            }

            // (B) 실제 위치 이동
            // 여러 유닛이 동시에 이동할 때 겹침 방지를 위해 약간의 오프셋을 줄 수도 있습니다.
            Unit->SetActorLocation(Destination);

            // (C) 이동 후 유닛의 기존 명령 취소 (선택 사항)
            // Unit->Stop(); // 이동 후 멍하게 서 있지 않도록 명령 초기화 가능
        }
    }

    // 4. 어빌리티 종료
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UNovaGA_Recall::OnTargetDataCancelledCallback(const FGameplayAbilityTargetDataHandle& DataHandle)
{
    // 사용자가 취소했을 경우 자원을 소모하지 않고 어빌리티만 종료합니다.
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}


