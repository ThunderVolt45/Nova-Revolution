// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Core/NovaInterfaces.h"
#include "NovaPlayerState.generated.h"

/**
 * Nova Revolution의 플레이어 상태 관리 클래스
 * GAS의 AbilitySystemComponent를 소유합니다.
 */
UCLASS()
class NOVAREVOLUTION_API ANovaPlayerState : public APlayerState, public IAbilitySystemInterface, public INovaResourceInterface
{
	GENERATED_BODY()

public:
	ANovaPlayerState();

protected:
	virtual void BeginPlay() override;

public:
	// IAbilitySystemInterface 구현
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

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

	// INovaResourceInterface 구현 (델리게이트)
	virtual FNovaOnResourceChangedSignature& GetOnWattChangedDelegate() override { return OnWattChanged; }
	virtual FNovaOnResourceChangedSignature& GetOnSPChangedDelegate() override { return OnSPChanged; }
	virtual FNovaOnResourceChangedSignature& GetOnPopulationChangedDelegate() override { return OnPopulationChanged; }

	/** 블루프린트에서 직접 접근 가능한 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Nova|Resource")
	FNovaOnResourceChangedSignature OnWattChanged;

	UPROPERTY(BlueprintAssignable, Category = "Nova|Resource")
	FNovaOnResourceChangedSignature OnSPChanged;

	UPROPERTY(BlueprintAssignable, Category = "Nova|Resource")
	FNovaOnResourceChangedSignature OnPopulationChanged;

	// AttributeSet 접근자
	class UNovaResourceAttributeSet* GetResourceAttributeSet() const { return ResourceAttributeSet; }

protected:
	// GAS 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	class UAbilitySystemComponent* AbilitySystemComponent;

	// 자원 속성 세트
	UPROPERTY()
	class UNovaResourceAttributeSet* ResourceAttributeSet;
};
