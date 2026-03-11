// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Commander/NovaSkillAbility.h"
#include "Core/NovaPlayerState.h"
#include "Core/NovaBase.h"
#include "AbilitySystemComponent.h"


UNovaSkillAbility::UNovaSkillAbility()
{
	// 사령관 스킬은 기본적으로 서버에서 실행되며 클라이언트에 통보되는 방식을 따릅니다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

ANovaBase* UNovaSkillAbility::GetPlayerBase() const
{
	// 어빌리티의 AvatarActor는 PlayerState입니다.
	if (ANovaPlayerState* PS = Cast<ANovaPlayerState>(GetAvatarActorFromActorInfo()))
	{
		return PS->GetPlayerBase();
	}
	return nullptr;
}

bool UNovaSkillAbility::CheckCommanderResources(float WattCost, float SPCost) const
{
	if (ANovaPlayerState* PS = Cast<ANovaPlayerState>(GetAvatarActorFromActorInfo()))
	{
		// PlayerState가 구현한 자원 인터페이스를 통해 현재 수치 확인
		return (PS->GetCurrentWatt() >= WattCost) && (PS->GetCurrentSP() >= SPCost);
	}
	return false;
}

bool UNovaSkillAbility::K2_CheckAndNotifyResources(float WattCost, float SPCost)
{
	if (CheckCommanderResources(WattCost, SPCost))
	{
		return true;
	}

	// TODO: 자원이 부족할 경우 HUD나 UI에 경고 메시지를 띄우는 GameplayTag 이벤트를 여기서 호출할 수 있습니다.
	// 예: SendGameplayEvent(NovaGameplayTags::Event_UI_InsufficientResources);

	return false;
}