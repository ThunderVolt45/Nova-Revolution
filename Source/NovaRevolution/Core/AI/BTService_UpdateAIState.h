// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateAIState.generated.h"

/**
 * AI 플레이어의 자원 및 부대 상태를 주기적으로 블랙보드에 업데이트하는 서비스
 */
UCLASS()
class NOVAREVOLUTION_API UBTService_UpdateAIState : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateAIState();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 업데이트할 블랙보드 키들 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector WattKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector SPKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PopulationKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector MaxPopulationKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TotalUnitWattKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector MaxUnitWattKey;
};
