// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Core/NovaInterfaces.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NovaAIController.generated.h"

class UBehaviorTreeComponent;
class UBehaviorTree;

/**
 * Nova Revolution의 유닛 제어를 담당하는 전용 AI 컨트롤러
 */
UCLASS()
class NOVAREVOLUTION_API ANovaAIController : public AAIController, public INovaCommandInterface
{
	GENERATED_BODY()

public:
	ANovaAIController();

	virtual void Tick(float DeltaTime) override;

	// --- INovaCommandInterface 구현 ---
	virtual void IssueCommand(const FCommandData& CommandData) override;
	
	/** 현재 명령 상태 반환 */
	ECommandType GetCurrentCommand() const;

	/** 유닛 타입에 맞춰 최적화된 이동 명령 (위치 기반) */
	void MoveToLocationOptimized(const FVector& Dest, float AcceptanceRadius = 50.f);

	/** 유닛 타입에 맞춰 최적화된 이동 명령 (액터 추적 기반) */
	void MoveToActorOptimized(AActor* TargetActor, float AcceptanceRadius = 50.f);

	/** 현재 이동 명령이 진행 중인지 여부를 반환합니다. (수동 이동 포함) */
	bool IsMoveInProgress() const;

	/** 모든 방식의 이동을 즉시 중단합니다. */
	void StopMovementOptimized();

	/** 태그를 통해 빙의된 유닛의 어빌리티를 발동합니다. (중복 로직 통합) */
	UFUNCTION(BlueprintCallable, Category = "Nova|AI")
	void ActivateAbilityByTag(const FGameplayTag& AbilityTag, AActor* Target);

	/** 빙의된 유닛이 사망했을 때 호출되어 AI 동작을 정지합니다. */
	void OnPawnDeath();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** 블랙보드 값이 변경될 때 호출될 콜백 함수 */
	EBlackboardNotificationResult OnCommandTypeChanged(const UBlackboardComponent& InBlackboard, FBlackboard::FKey KeyID);

	/** 유닛이 장애물이나 다른 유닛에 막혀 멈춰있는지(Stuck) 감지하고 처리합니다. */
	void UpdateStuckDetection(float DeltaTime);

	/** Stuck 상태로 판정되었을 때 실행할 회피 기동 로직입니다. */
	void HandleStuckStatus();

	// --- AI 컴포넌트 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|AI")
	TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|AI")
	TObjectPtr<UBlackboardComponent> BlackboardComponent;

	// --- 설정 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	// 이동 명령 시 불필요한 이동을 방지하기 위한 최소 이동 사거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|AI")
	float MinMoveDistance = 10.f;

	/** Stuck 감지용 시간 임계값 (이 시간 동안 못 움직이면 Stuck으로 간주) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|AI")
	float StuckTimeThreshold = 1.0f;

	/** Stuck 판단 기준 거리 (이 시간 동안 이 거리보다 적게 움직이면 멈춘 것으로 간주) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|AI")
	float StuckDistanceThreshold = 50.0f;

	/** 조기 도착(Early Arrival) 판정을 위한 목표 지점 인접 거리 (이 거리 내에서 아군에 막히면 도착으로 간주) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|AI")
	float EarlyArrivalDistance = 600.0f;

	/** Stuck 상태에서 우회 기동 시 옆으로 이동할 목표 지점까지의 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|AI")
	float BypassDistance = 300.0f;

private:
	/** Stuck 감지용 누적 타이머 */
	float StuckTimer = 0.0f;

	/** 마지막으로 위치를 체크했던 좌표 */
	FVector LastStuckCheckLocation = FVector::ZeroVector;

	/** 공중 유닛 수동 이동 제어를 위한 변수 */
	bool bIsManualMoving = false;
	FVector ManualMoveGoal = FVector::ZeroVector;
	TWeakObjectPtr<AActor> ManualMoveTargetActor;
	float ManualAcceptanceRadius = 50.f;

	/** 공중 유닛 수동 이동 처리 로직 (Tick에서 호출) */
	void UpdateManualMovement(float DeltaTime);

	/** 블랙보드 키 이름 정의 */
	static const FName TargetLocationKey;
	static const FName TargetActorKey;
	static const FName CommandTypeKey;
};
