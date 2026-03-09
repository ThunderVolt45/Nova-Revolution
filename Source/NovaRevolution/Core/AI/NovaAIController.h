// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Core/NovaInterfaces.h"
#include "GameplayTagContainer.h"
#include "NovaAIController.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;
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

	// --- INovaCommandInterface 구현 ---
	virtual void IssueCommand(const FCommandData& CommandData) override;

	/** 태그를 통해 빙의된 유닛의 어빌리티를 발동합니다. (중복 로직 통합) */
	UFUNCTION(BlueprintCallable, Category = "Nova|AI")
	void ActivateAbilityByTag(const FGameplayTag& AbilityTag, AActor* Target);

	/** 빙의된 유닛이 사망했을 때 호출되어 AI 동작을 정지합니다. */
	void OnPawnDeath();

protected:
	virtual void OnPossess(APawn* InPawn) override;

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

private:
	/** 블랙보드 키 이름 정의 */
	static const FName TargetLocationKey;
	static const FName TargetActorKey;
	static const FName CommandTypeKey;
};
