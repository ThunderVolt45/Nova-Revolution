// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "NovaPlayerState.generated.h"

/**
 * Nova Revolution의 플레이어 상태 관리 클래스
 * GAS의 AbilitySystemComponent를 소유합니다.
 */
UCLASS()
class NOVAREVOLUTION_API ANovaPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ANovaPlayerState();

	// IAbilitySystemInterface 구현
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// AttributeSet 접근자
	class UNovaAttributeSet* GetAttributeSet() const { return AttributeSet; }

protected:
	// GAS 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	class UAbilitySystemComponent* AbilitySystemComponent;

	// 속성 세트
	UPROPERTY()
	class UNovaAttributeSet* AttributeSet;
};
