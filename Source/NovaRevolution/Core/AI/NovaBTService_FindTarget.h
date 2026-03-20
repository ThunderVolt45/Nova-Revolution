// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "NovaBTService_FindTarget.generated.h"

/**
 * 유닛 주변의 적군을 탐색하여 블랙보드에 저장하는 서비스
 */
UCLASS()
class NOVAREVOLUTION_API UNovaBTService_FindTarget : public UBTService
{
	GENERATED_BODY()

public:
	UNovaBTService_FindTarget();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 타겟 정보를 저장할 블랙보드 키 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FBlackboardKeySelector TargetActorKey;

	/** 현재 명령 상태를 확인할 블랙보드 키 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FBlackboardKeySelector CommandTypeKey;

	/** 강제 공격 여부를 확인할 블랙보드 키 */
	UPROPERTY(EditAnywhere, Category = "Nova|AI")
	FBlackboardKeySelector IsFocusAttackKey;
};
