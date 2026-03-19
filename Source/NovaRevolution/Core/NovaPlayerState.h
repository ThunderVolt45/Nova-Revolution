// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Core/NovaInterfaces.h"
#include "NovaPlayerState.generated.h"

/** 기지 변경 알림을 위한 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNovaOnBaseChangedSignature, class ANovaBase*, NewBase);

/**
 * Nova Revolution의 플레이어 상태 관리 클래스
 * GAS의 AbilitySystemComponent를 소유합니다.
 */
UCLASS()
class NOVAREVOLUTION_API ANovaPlayerState : public APlayerState, public IAbilitySystemInterface, public INovaResourceInterface, public INovaTeamInterface, public INovaSkillInterface
{
	GENERATED_BODY()

public:
	ANovaPlayerState();

	// IAbilitySystemInterface 구현
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// INovaTeamInterface 구현
	virtual int32 GetTeamID() const override { return TeamID; }

	/** 팀 ID를 설정합니다. (GameMode에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Team")
	void SetTeamID(int32 NewTeamID) { TeamID = NewTeamID; }

	// INovaResourceInterface 구현 (속성 접근자)
	UFUNCTION()
	virtual float GetCurrentWatt() const override;
	
	UFUNCTION()
	virtual float GetMaxWatt() const override;
	
	UFUNCTION()
	virtual float GetCurrentSP() const override;
	
	UFUNCTION()
	virtual float GetMaxSP() const override;
	
	UFUNCTION()
	virtual float GetCurrentPopulation() const override;
	
	UFUNCTION()
	virtual float GetMaxPopulation() const override;
	
	UFUNCTION()
	virtual float GetTotalUnitWatt() const override;
	
	UFUNCTION()
	virtual float GetMaxUnitWatt() const override;
	
	UFUNCTION()
	virtual float GetWattLevel() const override;
	
	UFUNCTION()
	virtual float GetSPLevel() const override;

	// INovaResourceInterface 구현 (델리게이트)
	virtual FNovaOnResourceChangedSignature& GetOnWattChangedDelegate() override { return OnWattChanged; }
	virtual FNovaOnResourceChangedSignature& GetOnSPChangedDelegate() override { return OnSPChanged; }
	virtual FNovaOnResourceChangedSignature& GetOnPopulationChangedDelegate() override { return OnPopulationChanged; }
	virtual FNovaOnResourceChangedSignature& GetOnTotalUnitWattChangedDelegate() override { return OnTotalUnitWattChanged; }
	virtual FNovaOnResourceChangedSignature& GetOnWattLevelChangedDelegate() override { return OnWattLevelChanged; }
	virtual FNovaOnResourceChangedSignature& GetOnSPLevelChangedDelegate() override { return OnSPLevelChanged; }

	// INovaSkillInterface 구현: 특정 태그의 사령관 스킬 실행 
	virtual void ActivateSkillAbility_Implementation(FGameplayTag AbilityTag) override;
	
	//  INovaSkillInterface 구현: 슬롯 기반 사령관 스킬 실행
	virtual void ActivateSkillSlot_Implementation(int32 SlotIndex) override;
	
	/** 전체 스킬 슬롯 태그 정보를 반환합니다. (AI Task 등에서 사용) */
	const TArray<FGameplayTag>& GetSkillSlotTags() const { return SkillSlotTags; }

	/** 플레이어의 메인 기지(Base)를 등록합니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Base")
	void SetPlayerBase(class ANovaBase* InBase);

	/** 플레이어의 메인 기지(Base)를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Nova|Base")
	class ANovaBase* GetPlayerBase() const { return PlayerBase.Get(); }

	/** 블루프린트에서 직접 접근 가능한 자원 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Nova|Resource")
	FNovaOnResourceChangedSignature OnWattChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Nova|Resource")
	FNovaOnResourceChangedSignature OnSPChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Nova|Resource")
	FNovaOnResourceChangedSignature OnPopulationChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Nova|Resource")
	FNovaOnResourceChangedSignature OnTotalUnitWattChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Nova|Resource")
	FNovaOnResourceChangedSignature OnWattLevelChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Nova|Resource")
	FNovaOnResourceChangedSignature OnSPLevelChanged;

	/** 기지 변경/파괴 시 호출되는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Nova|Base")
	FNovaOnBaseChangedSignature OnBaseChanged;

	// AttributeSet 접근자
	class UNovaResourceAttributeSet* GetResourceAttributeSet() const { return ResourceAttributeSet; }

protected:
	virtual void BeginPlay() override;

	// 플레이어의 메인 기지 (약참조)
	UPROPERTY()
	TWeakObjectPtr<class ANovaBase> PlayerBase;

	// 현재 플레이어의 팀 ID
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Team")
	int32 TeamID = 0; // NovaTeam::None

	// GAS 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	class UAbilitySystemComponent* AbilitySystemComponent;

	// 자원 속성 세트
	UPROPERTY()
	class UNovaResourceAttributeSet* ResourceAttributeSet;
	
	// 테스트 및 초기 부여용 어빌리티 리스트
	UPROPERTY(EditDefaultsOnly, Category = "Nova|GAS")
	TArray<TSubclassOf<class UGameplayAbility>> SkillAbilities;
	
	// 각 슬롯(0~9)에 할당된 스킬 태그 배열 (에디터에서 설정 가능)
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Skill")
	TArray<FGameplayTag> SkillSlotTags;
};
