// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/Skill/NovaSkillAbility.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "NovaGA_Recall.generated.h"

/**
 * UNovaGA_Recall
 * 지정된 영역 내의 아군 유닛들을 아군 기지의 랠리 포인트로 텔레포트시키는 스킬입니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaGA_Recall : public UNovaSkillAbility
{
	GENERATED_BODY()
	
public:
	UNovaGA_Recall();

	/** 어빌리티가 활성화될 때 호출되는 메인 함수 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** 사용자가 마우스 클릭으로 타겟팅을 확정했을 때 호출되는 콜백 */
	UFUNCTION()
	void OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& DataHandle);

	/** 사용자가 타겟팅을 취소(ESC 등)했을 때 호출되는 콜백 */
	UFUNCTION()
	void OnTargetDataCancelledCallback(const FGameplayAbilityTargetDataHandle& DataHandle);
	
	/** 어빌리티가 종료될 때 자동으로 호출되는 GAS 표준 함수 */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	  const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 사용할 타겟 액터 클래스 (ANovaTargetActor_GroundRadius 또는 그 자식 블루프린트) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Recall")
	TSubclassOf<class ANovaTargetActor_GroundRadius> TargetActorClass;

	/** 텔레포트 시 발생시킬 비주얼 이펙트 태그 (GameplayCue) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Recall")
	FGameplayTag RecallCueTag;

	/** 타겟팅 반지름 (기본값 700 = 노바 기준 7) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Recall")
	float RecallRadius = 500.0f;
};
