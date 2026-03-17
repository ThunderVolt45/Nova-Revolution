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
	
	
	// --- 1. GAS 초기화 (가장 먼저 수행) ---
	if (AbilitySystemComponent)
	{
		// Owner: 자신(PlayerState), Avatar: 자신(PlayerState)
		// 이제 PlayerState는 '사령관'으로서 어빌리티를 실행할 권한을 얻습니다.
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
	
	if (GetNetMode() != NM_Client && AbilitySystemComponent)
	{
		for (const auto& AbilityClass : SkillAbilities)
		{
			if (AbilityClass)
			{
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
			}
		}
	}
	
	

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

void ANovaPlayerState::ActivateSkillAbility_Implementation(FGameplayTag AbilityTag)
{
	if (AbilitySystemComponent)
	{
		// 입력받은 태그를 가진 어빌리티(사령관 스킬)를 찾아 실행을 시도합니다.
		// 이 한 줄로 앞으로 추가될 모든 사령관 스킬이 공통으로 처리됩니다.
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTag));
	}
}

void ANovaPlayerState::ActivateSkillSlot_Implementation(int32 SlotIndex)
{
	// 1. 슬롯 인덱스 유효성 검사 (0~9 범위 내에 있는지) 및 할당된 태그 존재 확인
	if (SkillSlotTags.IsValidIndex(SlotIndex) && SkillSlotTags[SlotIndex].IsValid())
	{
		// 2. 이미 구현된 태그 기반 스킬 실행 함수(ActivateSkillAbility)를 호출합니다.
		// 인터페이스 함수이므로 Execute_ 접두사를 사용하여 안전하게 호출합니다.
		Execute_ActivateSkillAbility(this, SkillSlotTags[SlotIndex]);
	}
}
