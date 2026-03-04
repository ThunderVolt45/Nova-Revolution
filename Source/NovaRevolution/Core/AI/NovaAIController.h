// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Core/NovaInterfaces.h"
#include "NovaAIController.generated.h"

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

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	/** 명령 처리 유틸리티 함수들 */
	void HandleMoveCommand(const FVector& TargetLocation);
	void HandleAttackCommand(AActor* TargetActor);
	void HandleStopCommand();
};
