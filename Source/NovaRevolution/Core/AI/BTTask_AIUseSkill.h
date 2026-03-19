// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_AIUseSkill.generated.h"

/**
 * BTTask_AIUseSkill
 * 블랙보드에서 추천 스킬 정보를 읽어 사령관 스킬을 발동하는 태스크입니다.
 * GAS의 GameplayEvent를 통해 타겟 정보를 어빌리티에 전달합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UBTTask_AIUseSkill : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_AIUseSkill();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	/** 분석 서비스에서 설정한 추천 스킬 슬롯 키 */
	UPROPERTY(EditAnywhere, Category = "AI")
	FBlackboardKeySelector RecommendedSkillSlotKey;

	/** 스킬 대상 액터 키 */
	UPROPERTY(EditAnywhere, Category = "AI")
	FBlackboardKeySelector SkillTargetActorKey;

	/** 스킬 대상 위치 키 */
	UPROPERTY(EditAnywhere, Category = "AI")
	FBlackboardKeySelector SkillTargetLocationKey;
};
