// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_AIProduceUnit.generated.h"

/**
 * BTTask_AIProduceUnit
 * 블랙보드에서 추천 생산 슬롯을 읽어 AI 기지에 유닛 생산 명령을 내리는 태스크입니다.
 */
UCLASS()
class NOVAREVOLUTION_API UBTTask_AIProduceUnit : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_AIProduceUnit();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	/** 분석 서비스에서 설정한 추천 생산 슬롯 키 */
	UPROPERTY(EditAnywhere, Category = "AI")
	FBlackboardKeySelector RecommendedUnitSlotKey;
};
