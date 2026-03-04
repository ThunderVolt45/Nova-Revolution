// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaResourceComponent.h"
#include "Core/NovaPlayerState.h"
#include "GAS/NovaResourceAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"

UNovaResourceComponent::UNovaResourceComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // GE를 사용하므로 틱이 필요 없음
}

void UNovaResourceComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPlayerState = Cast<ANovaPlayerState>(GetOwner());
	if (OwnerPlayerState)
	{
		ResourceAttributeSet = OwnerPlayerState->GetResourceAttributeSet();
		
		// 자원 회복 GE 적용 (RTS 방식의 지속 회복)
		UAbilitySystemComponent* ASC = OwnerPlayerState->GetAbilitySystemComponent();
		if (ASC && RegenGameplayEffectClass)
		{
			FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
			EffectContext.AddSourceObject(this);

			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(RegenGameplayEffectClass, 1.0f, EffectContext);
			if (SpecHandle.IsValid())
			{
				RegenEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}

void UNovaResourceComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UNovaResourceComponent::CanAfford(float WattCost, float SPCost) const
{
	if (!ResourceAttributeSet) return false;

	return (ResourceAttributeSet->GetCurrentWatt() >= WattCost) && 
           (ResourceAttributeSet->GetCurrentSP() >= SPCost);
}

bool UNovaResourceComponent::CanSpawnUnit(float UnitWatt) const
{
	if (!ResourceAttributeSet) return false;

	// 인구수 제한 체크
	bool bPopOk = (ResourceAttributeSet->GetCurrentPopulation() + 1.0f <= ResourceAttributeSet->GetMaxPopulation());
	
	// 와트 총합 제한 체크
	bool bWattLimitOk = (ResourceAttributeSet->GetTotalUnitWatt() + UnitWatt <= ResourceAttributeSet->GetMaxUnitWatt());

	return bPopOk && bWattLimitOk;
}

void UNovaResourceComponent::ConsumeResources(float WattCost, float SPCost)
{
	if (!OwnerPlayerState || !ModifyResourceEffectClass) return;

	UAbilitySystemComponent* ASC = OwnerPlayerState->GetAbilitySystemComponent();
	if (!ASC) return;

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ModifyResourceEffectClass, 1.0f, EffectContext);
	if (SpecHandle.IsValid())
	{
		// SetByCaller 등을 활용해 동적으로 비용 전달 (태그가 정의되어 있다고 가정)
		// 만약 태그가 정의되어 있지 않다면 임시로 직접 수정 로직으로 대체 가능하지만, 
		// 여기서는 GE 기반 설계를 위해 태그 기반 수정을 권장함.
		
		// WattCost와 SPCost를 음수로 전달하여 차감
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Resource.Watt")), -WattCost);
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Resource.SP")), -SPCost);

		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void UNovaResourceComponent::UpdatePopulation(float DeltaPopulation, float DeltaWatt)
{
    // 인구수 업데이트도 GE로 처리할 수 있으나, 보통 인구수 관리는 인스턴트 속성 변경이므로 직접 처리하거나 별도 GE 사용
    if (!ResourceAttributeSet) return;
    
    // 이 부분도 GE를 활용하고자 한다면 ConsumeResources와 유사하게 구현 가능
    ResourceAttributeSet->SetCurrentPopulation(ResourceAttributeSet->GetCurrentPopulation() + DeltaPopulation);
    ResourceAttributeSet->SetTotalUnitWatt(ResourceAttributeSet->GetTotalUnitWatt() + DeltaWatt);
}
