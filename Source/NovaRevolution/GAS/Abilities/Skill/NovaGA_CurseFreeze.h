// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/Skill/NovaSkillAbility.h"
#include "NovaGA_CurseFreeze.generated.h"

/**
 * UNovaGA_CurseFreeze
 * 범위 내 적 유닛의 이동 속도를 0으로 만듭니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaGA_CurseFreeze : public UNovaSkillAbility
{
	GENERATED_BODY()
	
public:
	UNovaGA_CurseFreeze();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** 타겟팅 확정 시 호출될 콜백 */
	UFUNCTION()
	void OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& DataHandle);

	/** 타겟팅 취소 시 호출될 콜백 */
	UFUNCTION()
	void OnTargetDataCancelledCallback(const FGameplayAbilityTargetDataHandle& DataHandle);

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 사용할 타겟 액터 클래스 (ANovaTargetActor_GroundRadius) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Skill")
	TSubclassOf<class ANovaTargetActor_GroundRadius> TargetActorClass;

	/** 적에게 적용할 게임플레이 이펙트 클래스 (7초 지속, 속도 0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Skill")
	TSubclassOf<class UGameplayEffect> CurseFreezeGEClass;

	/** 프리즈 범위 (기본값: 700.0f = Nova 기준 7) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Skill")
	float FreezeRadius = 700.0f;

	/** 타겟팅 중 영역 표시 색상 (얼음 느낌의 파란색/하늘색) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Skill")
	FLinearColor SkillHighlightColor = FLinearColor(0.0f, 0.5f, 1.0f);
	
};
