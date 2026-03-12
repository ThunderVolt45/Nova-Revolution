// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "NovaResourceComponent.generated.h"

/**
 * 플레이어의 자원(Watt, SP, Population) 상태를 관리하고 조작하는 컴포넌트
 */
UCLASS(ClassGroup=(Nova), meta=(BlueprintSpawnableComponent))
class NOVAREVOLUTION_API UNovaResourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UNovaResourceComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	/** 자원 구매 가능 여부 확인 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Resource")
	bool CanAfford(float WattCost, float SPCost) const;

	/** 유닛 생산 가능 여부 (인구수 및 와트 총합 제한) 확인 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Resource")
	bool CanSpawnUnit(float UnitWatt) const;

	/** 자원 소모 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Resource")
	void ConsumeResources(float WattCost, float SPCost);

	/** 인구수 및 와트 합계 업데이트 (유닛 생산/파괴 시 호출) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Resource")
	void UpdatePopulation(float DeltaPopulation, float DeltaWatt);

	/** 자원 회복 GE를 중단합니다. (기지 파괴 시 등) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Resource")
	void StopResourceRegen();

	/** 자원 회복용 GameplayEffect 클래스 */
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Resource")
	// TSubclassOf<class UGameplayEffect> RegenGameplayEffectClass;
	
	/** 자원 회복용 GameplayEffect 클래스 (Watt) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Resource")
	TSubclassOf<class UGameplayEffect> WattRegenGEClass;

	/** 자원 회복용 GameplayEffect 클래스 (SP) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Resource")
	TSubclassOf<class UGameplayEffect> SPRegenGEClass;

	/** 자원 획득/소모용 GameplayEffect 클래스 (Instant) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Resource")
	TSubclassOf<class UGameplayEffect> ModifyResourceEffectClass;

private:
	/** 자원 회복용 GE 핸들 */
	//FActiveGameplayEffectHandle RegenEffectHandle;
	
	/** 자원 회복용 GE 핸들 (Watt) */
	FActiveGameplayEffectHandle WattRegenHandle;

	/** 자원 회복용 GE 핸들 (SP) */
	FActiveGameplayEffectHandle SPRegenHandle;

private:
	/** 소유한 PlayerState 및 AttributeSet 캐싱 */
	UPROPERTY()
	class ANovaPlayerState* OwnerPlayerState;

	UPROPERTY()
	class UNovaResourceAttributeSet* ResourceAttributeSet;
};
