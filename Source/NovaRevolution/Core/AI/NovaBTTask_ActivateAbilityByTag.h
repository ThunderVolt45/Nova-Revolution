// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "NovaBTTask_ActivateAbilityByTag.generated.h"

/**
 * UNovaBTTask_ActivateAbilityByTag
 * 유닛의 ASC에서 특정 GameplayTag를 가진 어빌리티를 트리거하는 태스크입니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaBTTask_ActivateAbilityByTag : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UNovaBTTask_ActivateAbilityByTag();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	/** 실행할 어빌리티를 식별할 태그 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FGameplayTag AbilityTag;

	/** 블랙보드에서 타겟 액터를 가져올 키 (선택 사항, TargetData로 넘길 때 사용) */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FBlackboardKeySelector TargetActorKey;
};
