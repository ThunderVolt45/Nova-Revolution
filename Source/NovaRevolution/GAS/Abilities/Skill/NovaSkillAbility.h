// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/NovaGameplayAbility.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "NovaSkillAbility.generated.h"

/**
 * GameplayCue가 발동되는 대상을 정의합니다.
 */
UENUM(BlueprintType)
enum class ENovaSkillGCNTargetType : uint8
{
	None,				// GCN 미사용
	Avatar,				// 어빌리티 실행자(AvatarActor, 보통 PS)
	Base,				// 플레이어의 메인 기지
	TargetActors,		// 타겟 데이터에 포함된 모든 액터
	TargetLocation		// 타겟 데이터의 첫 번째 위치 (부착 없이 실행)
};

/**
 * UNovaSkillAbility
 * 모든 사령관 전용 스킬(기지 소환, 프리즈, 레벨업 등)의 기반 클래스입니다.
 * 플레이어의 기지 참조 및 자원(Watt/SP) 확인 공통 로직을 포함합니다.
 */

UCLASS()
class NOVAREVOLUTION_API UNovaSkillAbility : public UNovaGameplayAbility
{
	GENERATED_BODY()
	
public:
	UNovaSkillAbility();

	/** 현재 플레이어의 자원(Watt, SP)이 충분한지 확인합니다. */
	UFUNCTION(BlueprintPure, Category = "Nova|Ability|Skill")
	bool CheckResources(float CheckWattCost, float CheckSPCost) const;

	/** [추가] 블루프린트에서 입력받을 자원 비용 */
	float GetWattCost() const { return WattCost; }
	float GetSPCost() const { return SPCost; }

protected:
	/** 어빌리티를 실행하는 플레이어의 메인 기지(ANovaBase)를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Nova|Ability|Skill")
	class ANovaBase* GetPlayerBase() const;

	/** 자원 소모가 가능한지 확인하고, 부족할 경우 경고 UI용 이벤트를 발생시킬 수 있습니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Ability|Skill")
	bool K2_CheckAndNotifyResources(float CheckWattCost, float CheckSPCost);
	
	/** [추가] 블루프린트에서 입력받을 자원 비용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nova|Skill|Cost")
	float WattCost = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nova|Skill|Cost")
	float SPCost = 0.0f;

	/** [추가] 자원 소모용 공통 GE */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Skill|Cost")
	TSubclassOf<class UGameplayEffect> ModifyResourceGEClass;

	/** [추가] 설정된 변수(WattCost, SPCost)를 사용하여 자동으로 체크 */
	bool CheckSkillCost();

	/** [추가] 설정된 변수와 GE를 사용하여 자동으로 소모 */
	void ApplySkillCost();

	/** GameplayCue 실행 방식 설정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nova|Skill|GameplayCue")
	ENovaSkillGCNTargetType GCNTargetType = ENovaSkillGCNTargetType::None;

	/** 실행할 GameplayCue 태그 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nova|Skill|GameplayCue")
	FGameplayTag SkillGCNTag;

	/** 설정된 타겟 타입에 맞춰 GCN을 실행합니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Ability|Skill")
	void ExecuteSkillGCN(const FGameplayAbilityTargetDataHandle& TargetData);
	
};
