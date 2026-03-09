// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "NovaBTTask_Patrol.generated.h"

/**
 * 지정된 목표 지점과 현재 위치(순찰 시작 지점)를 왕복하는 순찰 태스크
 */
UCLASS()
class NOVAREVOLUTION_API UNovaBTTask_Patrol : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UNovaBTTask_Patrol();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 순찰 목표 지점이 저장된 블랙보드 키 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FBlackboardKeySelector TargetLocationKey;

	/** 순찰 시작 지점을 저장할 블랙보드 키 (Vector) */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FBlackboardKeySelector PatrolOriginKey;

	/** 적 발견 시 중단을 위한 타겟 블랙보드 키 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FBlackboardKeySelector TargetActorKey;

	/** 도달 인정 거리 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	float AcceptanceRadius = 10.0f;

	/** 순찰 지점으로부터의 최대 추격 거리 (Leash) */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	float ChaseDistanceLimit = 1000.0f;

	/** 공격 간격 (초) */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	float AttackInterval = 1.0f;

	/** 공격 시 발동할 어빌리티 태그 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FGameplayTag AbilityTag;

private:
	/** 현재 목표가 Origin인지 TargetLocation인지 여부 */
	bool bMovingToOrigin = false;

	/** 마지막 공격 시간 */
	float LastAttackTime = 0.0f;

	/** 교전이 시작된 지점 (Leash 기준점) */
	FVector CombatOrigin = FVector::ZeroVector;

	/** 공격 사거리 가져오기 */
	float GetAttackRange(class ANovaUnit* Unit) const;

	/** 공격 실행 */
	void PerformAttack(class ANovaUnit* Unit, AActor* Target);
};
