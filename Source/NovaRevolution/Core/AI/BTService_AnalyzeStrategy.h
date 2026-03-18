// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_AnalyzeStrategy.generated.h"

/**
 * 전황을 분석하여 생산할 유닛 슬롯과 사용할 스킬 슬롯을 결정하는 핵심 AI 서비스
 */
UCLASS()
class NOVAREVOLUTION_API UBTService_AnalyzeStrategy : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_AnalyzeStrategy();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 분석 결과가 기록될 블랙보드 키들 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector RecommendedUnitSlotKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector RecommendedSkillSlotKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector SkillTargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector SkillTargetLocationKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector bShouldAttackKey;

	/** 행동 트리에서 설정 가능한 고가치 유닛 기준 (와트) */
	UPROPERTY(EditAnywhere, Category = "AI|Threshold")
	float HighValueWattThreshold = 500.0f;

	/** 스킬 슬롯 설정 (0~9) */
	UPROPERTY(EditAnywhere, Category = "AI|SkillSlot")
	int32 RecycleSkillSlot = 0;

	UPROPERTY(EditAnywhere, Category = "AI|SkillSlot")
	int32 FreezeSkillSlot = 1;

	UPROPERTY(EditAnywhere, Category = "AI|SkillSlot")
	int32 BaseSummonSkillSlot = 2;

	UPROPERTY(EditAnywhere, Category = "AI|SkillSlot")
	int32 ResourceBoostSkillSlot = 3;

	UPROPERTY(EditAnywhere, Category = "AI|SkillSlot")
	int32 OverworkSkillSlot = 4;

	/** 부품 정보 데이터 테이블 (유닛 성능 분석용) */
	UPROPERTY(EditAnywhere, Category = "AI|Data")
	TObjectPtr<UDataTable> PartSpecDataTable;

private:
	/** 유닛 생산 판단 로직 */
	int32 AnalyzeProduction(class ANovaAIPlayerController* AIC, class ANovaPlayerState* PS);

	/** 슬롯 유닛의 예상 성능 데이터를 산출합니다. */
	void CalculateUnitPerformance(const struct FNovaUnitAssemblyData& AssemblyData, struct FNovaPartSpecRow& OutStats);

	/** 스킬 사용 판단 로직 */
	int32 AnalyzeSkills(class ANovaAIPlayerController* AIC, class ANovaPlayerState* PS, AActor*& OutTargetActor, FVector& OutTargetLocation, bool bAttackingJustStarted);

	/** 공격 개시 여부 판단 로직 */
	bool ShouldStartAttack(class ANovaAIPlayerController* AIC, class ANovaPlayerState* PS);

	/** 초기화 시 공격 부대 규모를 기억하기 위한 변수 (기지 소환 판단용) */
	float LastAttackStartedUnitCount = 0.0f;
	bool bIsAttacking = false;
};
