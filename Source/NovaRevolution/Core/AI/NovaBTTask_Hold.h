// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "NovaBTTask_Hold.generated.h"

/**
 * 유닛을 현재 위치에서 정지시키고 대기 상태로 유지하는 태스크
 */
UCLASS()
class NOVAREVOLUTION_API UNovaBTTask_Hold : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UNovaBTTask_Hold();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 공격할 대상이 저장된 블랙보드 키 (FindTarget 서비스에 의해 업데이트됨) */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FBlackboardKeySelector TargetActorKey;

	/** 공격 시도 간격 (초) */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	float AttackInterval = 1.0f;

	/** 공격 시 발동할 어빌리티 태그 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FGameplayTag AbilityTag;

private:
	/** 마지막 공격 시간 */
	float LastAttackTime = 0.0f;

	/** 유닛의 공격 사거리를 가져옵니다. */
	float GetAttackRange(class ANovaUnit* Unit) const;
};
