// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "NovaBTTask_Attack.generated.h"

/**
 * 블랙보드에 지정된 타겟 액터를 공격하는 태스크
 * 사거리 밖이라면 사거리 안으로 이동하며, 사거리 내라면 공격 어빌리티를 실행합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaBTTask_Attack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UNovaBTTask_Attack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 공격할 대상이 저장된 블랙보드 키 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FBlackboardKeySelector TargetActorKey;

	/** 공격 시도 간격 (초) */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	float AttackInterval = 1.0f;

private:
	/** 마지막 공격 시간 */
	float LastAttackTime = 0.0f;

	/** 유닛의 공격 사거리를 가져옵니다. */
	float GetAttackRange(class ANovaUnit* Unit) const;

	/** 공격을 실행합니다. (GAS 어빌리티 호출 등) */
	void PerformAttack(class ANovaUnit* Unit, AActor* Target);
};
