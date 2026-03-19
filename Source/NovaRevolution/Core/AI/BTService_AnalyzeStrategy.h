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

	/** 위협 감지 거리 (긴급 방어용) */
	UPROPERTY(EditAnywhere, Category = "AI|Threshold")
	float ThreatDetectionRadius = 1200.0f;
	
	/** 자원 우위 판단 기준 (Watt/SP업 시전) */
	UPROPERTY(EditAnywhere, Category = "AI|Threshold")
	float WattPredominance = 1000.f;
	

private:
	/** 특정 슬롯 번호의 아군 유닛 수를 셉니다. */
	int32 CountUnitsOfSlot(class ANovaPlayerState* PS, int32 SlotIndex);

	/** 긴급 상황 및 매크로 루프에서 적의 상성을 고려하여 가장 적합한 아군 슬롯을 산출합니다. */
	int32 AnalyzeDynamicCounter(class ANovaAIPlayerController* AIC, class ANovaPlayerState* PS, bool bIsEmergency, int32 DefaultSlot = -1);

	/** 슬롯 유닛의 예상 성능 데이터를 산출합니다. */
	void CalculateUnitPerformance(const struct FNovaUnitAssemblyData& AssemblyData, struct FNovaPartSpecRow& OutStats);

	/** 스킬 사용 판단 로직 (오프닝/매크로 외 자율 스킬) */
	void AnalyzeOccasionalSkills(class ANovaAIPlayerController* AIC, class ANovaPlayerState* PS, UBlackboardComponent* BB);

	/** 초기화 시 공격 부대 규모를 기억하기 위한 변수 (기지 소환 판단용) */
	float LastAttackStartedUnitCount = 0.0f;
	bool bIsAttacking = false;
};
