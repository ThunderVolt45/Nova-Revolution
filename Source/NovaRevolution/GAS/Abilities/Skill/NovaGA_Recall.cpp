// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/Skill/NovaGA_Recall.h"
#include "GAS/Targeting/NovaTargetActor_GroundRadius.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Core/NovaBase.h"
#include "Core/NovaUnit.h"
#include "AbilitySystemComponent.h"
#include "NovaRevolution.h"
#include "Core/NovaLog.h"
#include "Player/NovaPlayerController.h"
#include "NavigationSystem.h"

UNovaGA_Recall::UNovaGA_Recall()
{
    // 기본 비용 설정 (BP에서 수정 가능)
    WattCost = 200.0f;
    SPCost = 15.0f;

    // 인스턴싱 정책 설정 (상태 저장을 위해 인스턴스화 필요)
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    // 통합 GCN 타겟 설정
    GCNTargetType = ENovaSkillGCNTargetType::TargetActors;
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
    
    // 추가) PlayerController를 가져와 스킬 모드로 설정
    if (ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetActorInfo().PlayerController))
    {
        //PC->PendingCommandType = ECommandType::Skill;
        PC->SetPendingCommandType(ECommandType::Skill);
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
        
        // 하이라이트 색상값 전달
        SpawnedTargetActor->CurrentHighlightColor = SkillHighlightColor;
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

        // 8. 태스크 활성화 (이 시점부터 타겟 액터가 월드에 나타나고 마우스를 추적합니다) **타겟액터 활동시작!!**
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

    // 2. 기지 위치 확인 (랠리 포인트가 아닌 기지 본체로 소환)
    ANovaBase* PlayerBase = GetPlayerBase();
    if (!PlayerBase)
    {
        NOVA_SCREEN(Error, "Player Base not found!");
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    FVector Destination = PlayerBase->GetActorLocation();
    
    // 라이브러리 함수를 사용하여 핸들로부터 액터 배열을 한 번에 추출합니다.
    TArray<AActor*> TargetActors = UAbilitySystemBlueprintLibrary::GetAllActorsFromTargetData(DataHandle);
    
    // 비주얼 이펙트 실행 (통합 GCN 시스템 사용)
    ExecuteSkillGCN(DataHandle);

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

    for (AActor* Actor : TargetActors)
    {
        if (ANovaUnit* Unit = Cast<ANovaUnit>(Actor))
        {
            // 이동할 최종 위치 계산 (기지 위치를 기본으로 하되, 지상 유닛은 NavMesh 상의 최단 거리 지점 탐색)
            FVector FinalLocation = Destination;

            if (Unit->GetMovementType() == ENovaMovementType::Ground && NavSys)
            {
                // 모든 유닛이 한 점에 겹치는 것을 방지하기 위해 기지 중심에서 약간의 오프셋(jitter)을 줍니다.
                FVector2D Jitter = FMath::RandPointInCircle(200.0f);
                FVector TestPoint = Destination;
                TestPoint.X += Jitter.X;
                TestPoint.Y += Jitter.Y;

                // 유닛의 내비게이션 속성에 맞는 특정 NavData(지상용 NavMesh)를 가져옵니다.
                const ANavigationData* NavData = NavSys->GetNavDataForProps(Unit->GetNavAgentPropertiesRef());
                if (NavData)
                {
                    FNavLocation NavLoc;
                    // 오프셋이 적용된 지점에서 가장 가까운(Nearest) 유효 지면을 찾습니다.
                    if (NavSys->ProjectPointToNavigation(TestPoint, NavLoc, FVector(500.f, 500.f, 200.f), NavData))
                    {
                        FinalLocation = NavLoc.Location;
                    }
                }
            }

            // 공중 유닛인 경우, 유닛이 유지해야 할 기본 고도(DefaultAirZ)를 Z축에 더해줍니다.
            // 이를 통해 공중 유닛이 기지 바닥에 처박히는 현상을 방지합니다.
            if (Unit->GetMovementType() == ENovaMovementType::Air)
            {
                FinalLocation.Z += Unit->GetDefaultAirZ();
            }

            // [수정] 보정된 위치로 순간이동
            Unit->SetActorLocation(FinalLocation);

            // 이동 후 유닛의 기존 명령을 취소하고 정지 상태로 만듭니다. (선택 사항)
            if (INovaCommandInterface* CmdInterface = Cast<INovaCommandInterface>(Unit))
            {
                FCommandData StopCmd;
                StopCmd.CommandType = ECommandType::Stop;
                CmdInterface->IssueCommand(StopCmd);
            }
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

void UNovaGA_Recall::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    // [추가] 어빌리티가 끝날 때(완료 또는 취소) 무조건 컨트롤러 상태를 복구합니다.
    if (ANovaPlayerController* PC = Cast<ANovaPlayerController>(ActorInfo->PlayerController))
    {
        // 현재 상태가 Skill일 때만 일반 모드(None)로 복구하여 조작권을 플레이어에게 돌려줍니다.
        if (PC->GetPendingCommandType() == ECommandType::Skill)
        {
            PC->SetPendingCommandType(ECommandType::None);
        }
    }
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


