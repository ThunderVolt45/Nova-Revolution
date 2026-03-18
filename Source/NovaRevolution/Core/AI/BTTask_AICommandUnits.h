// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Core/NovaTypes.h"
#include "BTTask_AICommandUnits.generated.h"

/**
 * BTTask_AICommandUnits
 * AI 플레이어가 소유한 모든 유닛에게 특정 명령(공격, 이동 등)을 일괄 하달하는 태스크입니다.
 */
UCLASS()
class NOVAREVOLUTION_API UBTTask_AICommandUnits : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_AICommandUnits();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	/** 하달할 명령 종류 (에디터 상에서 고정하거나 블랙보드 참조 가능) */
	UPROPERTY(EditAnywhere, Category = "AI")
	ECommandType CommandType = ECommandType::Attack;

	/** 블랙보드에서 목표 위치를 가져올 경우의 키 (필요 시) */
	UPROPERTY(EditAnywhere, Category = "AI")
	FBlackboardKeySelector TargetLocationKey;
};
