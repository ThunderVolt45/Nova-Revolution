// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
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

	/** 도달 인정 거리 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	float AcceptanceRadius = 10.0f;

private:
	/** 현재 목표가 Origin인지 TargetLocation인지 여부 */
	bool bMovingToOrigin = false;
};
