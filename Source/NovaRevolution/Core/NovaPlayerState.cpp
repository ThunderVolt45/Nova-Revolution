// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/NovaPlayerState.h"
#include "AbilitySystemComponent.h"
#include "GAS/NovaAttributeSet.h"

ANovaPlayerState::ANovaPlayerState()
{
	// AbilitySystemComponent 생성
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// 복제 모드 설정 (RTS는 보통 혼합 혹은 전체 복제 사용)
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// AttributeSet 생성
	AttributeSet = CreateDefaultSubobject<UNovaAttributeSet>(TEXT("AttributeSet"));

	// Net Update Frequency 설정 (네트워크 반응성 향상)
	SetNetUpdateFrequency(100.0f);
}

class UAbilitySystemComponent* ANovaPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
