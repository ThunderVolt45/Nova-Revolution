// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/NovaPlayerState.h"
#include "AbilitySystemComponent.h"
#include "GAS/NovaResourceAttributeSet.h"

ANovaPlayerState::ANovaPlayerState()
{
	// AbilitySystemComponent 생성
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// 복제 모드 설정 (RTS는 보통 혼합 혹은 전체 복제 사용)
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// 자원 속성 세트 생성
	ResourceAttributeSet = CreateDefaultSubobject<UNovaResourceAttributeSet>(TEXT("ResourceAttributeSet"));

	// Net Update Frequency 설정 (네트워크 반응성 향상)
	SetNetUpdateFrequency(100.0f);
}

void ANovaPlayerState::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent && ResourceAttributeSet)
	{
		// Watt 변경 감지
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(ResourceAttributeSet->GetCurrentWattAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				// 최적화: 정수 값이 변했을 때만 UI 알림 (빈번한 틱 충전 대응)
				if (FMath::FloorToInt(Data.OldValue) != FMath::FloorToInt(Data.NewValue))
				{
					OnWattChanged.Broadcast(Data.NewValue, GetMaxWatt());
				}
			}
		);

		// SP 변경 감지
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(ResourceAttributeSet->GetCurrentSPAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnSPChanged.Broadcast(Data.NewValue, GetMaxSP());
			}
		);

		// Population 변경 감지
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(ResourceAttributeSet->GetCurrentPopulationAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnPopulationChanged.Broadcast(Data.NewValue, GetMaxPopulation());
			}
		);
	}
}

class UAbilitySystemComponent* ANovaPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

float ANovaPlayerState::GetCurrentWatt() const
{
	return ResourceAttributeSet ? ResourceAttributeSet->GetCurrentWatt() : 0.0f;
}

float ANovaPlayerState::GetMaxWatt() const
{
	return ResourceAttributeSet ? ResourceAttributeSet->GetMaxWatt() : 0.0f;
}

float ANovaPlayerState::GetCurrentSP() const
{
	return ResourceAttributeSet ? ResourceAttributeSet->GetCurrentSP() : 0.0f;
}

float ANovaPlayerState::GetMaxSP() const
{
	return ResourceAttributeSet ? ResourceAttributeSet->GetMaxSP() : 0.0f;
}

float ANovaPlayerState::GetCurrentPopulation() const
{
	return ResourceAttributeSet ? ResourceAttributeSet->GetCurrentPopulation() : 0.0f;
}

float ANovaPlayerState::GetMaxPopulation() const
{
	return ResourceAttributeSet ? ResourceAttributeSet->GetMaxPopulation() : 0.0f;
}

float ANovaPlayerState::GetTotalUnitWatt() const
{
	return ResourceAttributeSet ? ResourceAttributeSet->GetTotalUnitWatt() : 0.0f;
}

float ANovaPlayerState::GetMaxUnitWatt() const
{
	return ResourceAttributeSet ? ResourceAttributeSet->GetMaxUnitWatt() : 0.0f;
}
