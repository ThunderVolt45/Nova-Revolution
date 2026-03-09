#include "Core/NovaPlayerState.h"
#include "Core/NovaBase.h"
#include "AbilitySystemComponent.h"
#include "GAS/NovaResourceAttributeSet.h"
#include "NovaRevolution.h"

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
					// NOVA_SCREEN(Log, "Watt Changed: %.0f / %.0f", Data.NewValue, GetMaxWatt());
				}
			}
		);

		// SP 변경 감지
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(ResourceAttributeSet->GetCurrentSPAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				// 최적화: 정수 값이 변했을 때만 UI 알림 (빈번한 틱 충전 대응)
				if (FMath::FloorToInt(Data.OldValue) != FMath::FloorToInt(Data.NewValue))
				{
					OnSPChanged.Broadcast(Data.NewValue, GetMaxSP());
					// NOVA_SCREEN(Log, "SP Changed: %.1f / %.1f", Data.NewValue, GetMaxSP());
				}
			}
		);

		// Population 변경 감지
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(ResourceAttributeSet->GetCurrentPopulationAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnPopulationChanged.Broadcast(Data.NewValue, GetMaxPopulation());
				// NOVA_SCREEN(Log, "Population Changed: %.0f / %.0f", Data.NewValue, GetMaxPopulation());
			}
		);

		// TotalUnitWatt 변경 감지
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(ResourceAttributeSet->GetTotalUnitWattAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnTotalUnitWattChanged.Broadcast(Data.NewValue, GetMaxUnitWatt());
			}
		);

		// Watt Level 변경 감지
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(ResourceAttributeSet->GetWattLevelAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnWattLevelChanged.Broadcast(Data.NewValue, 0.0f);
				// NOVA_SCREEN(Warning, "Watt Production Level Up: %.0f", Data.NewValue);
			}
		);

		// SP Level 변경 감지
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(ResourceAttributeSet->GetSPLevelAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnSPLevelChanged.Broadcast(Data.NewValue, 0.0f);
				// NOVA_SCREEN(Warning, "SP Production Level Up: %.0f", Data.NewValue);
			}
		);
	}
}

class UAbilitySystemComponent* ANovaPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ANovaPlayerState::SetPlayerBase(ANovaBase* InBase)
{
	PlayerBase = InBase;
	if (OnBaseChanged.IsBound())
	{
		OnBaseChanged.Broadcast(InBase);
	}
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

float ANovaPlayerState::GetWattLevel() const
{
	return ResourceAttributeSet ? ResourceAttributeSet->GetWattLevel() : 1.0f;
}

float ANovaPlayerState::GetSPLevel() const
{
	return ResourceAttributeSet ? ResourceAttributeSet->GetSPLevel() : 1.0f;
}
