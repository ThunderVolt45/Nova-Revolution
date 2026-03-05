// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "NovaBTTask_MoveToLocation.generated.h"

/**
 * 블랙보드의 목표 지점으로 유닛을 이동시키는 태스크
 * ANovaUnit의 직접 이동 로직을 BT 태스크로 이관하여 관리합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaBTTask_MoveToLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UNovaBTTask_MoveToLocation();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	/** 비헤이비어 트리의 틱 동안 이동 상태 감시 */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 목표 지점이 저장된 블랙보드 키 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FBlackboardKeySelector TargetLocationKey;

	/** 완료 후 상태를 초기화할 블랙보드 키 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FBlackboardKeySelector CommandTypeKey;

	/** 도달 인정 거리 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	float AcceptanceRadius = 10.0f;
};
