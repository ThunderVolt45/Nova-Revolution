// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Core/NovaInterfaces.h"
#include "Core/NovaTypes.h"
#include "NovaUnit.generated.h"

class UAbilitySystemComponent;
class UNovaAttributeSet;
struct FOnAttributeChangeData;

/**
 * Nova Revolution의 모든 유닛(로봇)의 기본 클래스
 */
UCLASS()
class NOVAREVOLUTION_API ANovaUnit : public ACharacter, public IAbilitySystemInterface, public INovaSelectableInterface, public INovaCommandInterface, public INovaTeamInterface
{
	GENERATED_BODY()

public:
	ANovaUnit();

	// --- IAbilitySystemInterface ---
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// --- INovaSelectableInterface ---
	virtual void OnSelected() override;
	virtual void OnDeselected() override;
	virtual bool IsSelectable() const override;

	// --- INovaCommandInterface ---
	virtual void IssueCommand(const FCommandData& CommandData) override;

	// --- INovaTeamInterface ---
	virtual ENovaTeam GetTeam() const override { return Team; }

	// 사망 처리 함수
	virtual void Die();

protected:
	virtual void BeginPlay() override;

	// --- GAS 구성 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UNovaAttributeSet> AttributeSet;

	// --- 유닛 기본 정보 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	ENovaTeam Team = ENovaTeam::None;

	// 선택 여부 (팀원 B가 하이라이트 로직 구현 시 사용)
	UPROPERTY(BlueprintReadOnly, Category = "Unit")
	bool bIsSelected = false;

private:
	// 속성 변경 시 호출될 콜백 함수 (UI 업데이트 등)
	void OnHealthChanged(const FOnAttributeChangeData& Data);
};
