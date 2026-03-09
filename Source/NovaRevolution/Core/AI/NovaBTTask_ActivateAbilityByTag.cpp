// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaBTTask_ActivateAbilityByTag.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemInterface.h"
#include "NovaRevolution.h"
#include "Core/NovaLog.h"

UNovaBTTask_ActivateAbilityByTag::UNovaBTTask_ActivateAbilityByTag()
{
	NodeName = TEXT("Activate Ability By Tag");

	// 블랙보드 키 필터링: Actor 타입만 허용
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTTask_ActivateAbilityByTag, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UNovaBTTask_ActivateAbilityByTag::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (!ControllingPawn) return EBTNodeResult::Failed;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(ControllingPawn);
	if (!ASI) return EBTNodeResult::Failed;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return EBTNodeResult::Failed;

	// 1. 블랙보드에서 타겟 액터 가져오기
	AActor* TargetActor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TargetActorKey.SelectedKeyName));

	// 2. 어빌리티 발동을 위한 이벤트 데이터 구성
	FGameplayEventData EventData;
	EventData.Instigator = ControllingPawn;
	EventData.Target = TargetActor; // (참고: GA에서 TargetData를 우선시하므로 아래 TargetData 로직 추가)

	// GAS 표준 방식인 TargetDataHandle에 타겟 추가
	if (TargetActor)
	{
		FGameplayAbilityTargetDataHandle TargetDataHandle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(TargetActor);
		EventData.TargetData = TargetDataHandle;
	}

	// 3. Gameplay Event를 통해 어빌리티 트리거
	// 어빌리티의 "Ability Triggers" 설정에 해당 태그가 등록되어 있어야 합니다.
	int32 TriggerCount = ASC->HandleGameplayEvent(AbilityTag, &EventData);

	if (TriggerCount > 0)
	{
		NOVA_LOG(Log, "AI Task: Triggered ability with tag [%s] for %s", *AbilityTag.ToString(), *ControllingPawn->GetName());
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}
