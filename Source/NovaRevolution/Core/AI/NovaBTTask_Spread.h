// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "NovaBTTask_Spread.generated.h"

/**
 * 유닛을 현재 위치에서 무작위 방향으로 산개시키는 태스크
 */
UCLASS()
class NOVAREVOLUTION_API UNovaBTTask_Spread : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UNovaBTTask_Spread();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 명령 지점이 저장된 블랙보드 키 (산개의 중심점) */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FBlackboardKeySelector TargetLocationKey;

	/** 완료 후 상태를 초기화할 블랙보드 키 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FBlackboardKeySelector CommandTypeKey;

	/** 기본 최소 산개 거리 (유닛의 MinRange가 이보다 작을 경우 사용) */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	float DefaultMinSpreadDistance = 200.0f;

	/** 도달 인정 거리 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	float AcceptanceRadius = 10.0f;
};
