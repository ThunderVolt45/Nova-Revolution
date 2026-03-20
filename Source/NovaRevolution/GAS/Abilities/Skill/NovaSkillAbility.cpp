// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Skill/NovaSkillAbility.h"
#include "Core/NovaPlayerState.h"
#include "Core/NovaBase.h"
#include "AbilitySystemComponent.h"
#include "NovaRevolution.h"
#include "Core/NovaLog.h"
#include "GAS/NovaGameplayTags.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemInterface.h"


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

bool UNovaSkillAbility::CheckResources(float CheckWattCost, float CheckSPCost) const
{
	if (ANovaPlayerState* PS = Cast<ANovaPlayerState>(GetAvatarActorFromActorInfo()))
	{
		// PlayerState가 구현한 자원 인터페이스를 통해 현재 수치 확인
		return (PS->GetCurrentWatt() >= CheckWattCost) && (PS->GetCurrentSP() >= CheckSPCost);
	}
	return false;
}

bool UNovaSkillAbility::K2_CheckAndNotifyResources(float CheckWattCost, float CheckSPCost)
{
	if (CheckResources(CheckWattCost, CheckSPCost))
	{
		return true;
	}

	// TODO: 자원이 부족할 경우 HUD나 UI에 경고 메시지를 띄우는 GameplayTag 이벤트를 여기서 호출할 수 있습니다.
	// 예: SendGameplayEvent(NovaGameplayTags::Event_UI_InsufficientResources);

	return false;
}

bool UNovaSkillAbility::CheckSkillCost()
{
	// 1. 설정된 멤버 변수(Required)를 사용하여 자원 확인
	if (!CheckResources(WattCost, SPCost))
	{
		// 2. 자원이 부족할 경우, 어빌리티를 강제로 종료시키고 false 반환
		NOVA_SCREEN(Warning, "Insufficient resources! Ability ended.");

		// Handle, ActorInfo, ActivationInfo는 상속받은 클래스에서 바로 접근 가능 (또는 멤버 변수로 저장된 값 사용)
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);

		return false;
	}

	return true;
}

void UNovaSkillAbility::ApplySkillCost()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ModifyResourceGEClass)
	{
		FGameplayEffectSpecHandle CostSpec = ASC->MakeOutgoingSpec(ModifyResourceGEClass, 1.0f, ASC->MakeEffectContext());
		if (CostSpec.IsValid())
		{
			// 멤버 변수로 설정된 WattCost와 SPCost를 음수로 변환하여 전달
			CostSpec.Data->SetSetByCallerMagnitude(NovaGameplayTags::Data_Resource_Watt, -WattCost);
			CostSpec.Data->SetSetByCallerMagnitude(NovaGameplayTags::Data_Resource_SP, -SPCost);
            
			ASC->ApplyGameplayEffectSpecToSelf(*CostSpec.Data.Get());
		}
	}
}

void UNovaSkillAbility::ExecuteSkillGCN(const FGameplayAbilityTargetDataHandle& TargetData)
{
	if (!SkillGCNTag.IsValid() || GCNTargetType == ENovaSkillGCNTargetType::None)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	FGameplayCueParameters Params;
	Params.AbilityLevel = GetAbilityLevel();
	Params.Instigator = GetAvatarActorFromActorInfo();
	Params.EffectContext = ASC->MakeEffectContext();

	switch (GCNTargetType)
	{
	case ENovaSkillGCNTargetType::Avatar:
		{
			ASC->ExecuteGameplayCue(SkillGCNTag, Params);
		}
		break;

	case ENovaSkillGCNTargetType::Base:
		{
			if (ANovaBase* Base = GetPlayerBase())
			{
				if (UAbilitySystemComponent* BaseASC = Base->GetAbilitySystemComponent())
				{
					BaseASC->ExecuteGameplayCue(SkillGCNTag, Params);
				}
				else
				{
					Params.Location = Base->GetActorLocation();
					ASC->ExecuteGameplayCue(SkillGCNTag, Params);
				}
			}
		}
		break;

	case ENovaSkillGCNTargetType::TargetActors:
		{
			for (int32 i = 0; i < TargetData.Num(); ++i)
			{
				TArray<AActor*> TargetActors = UAbilitySystemBlueprintLibrary::GetActorsFromTargetData(TargetData, i);
				for (AActor* TargetActor : TargetActors)
				{
					if (TargetActor)
					{
						if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor))
						{
							if (UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent())
							{
								TargetASC->ExecuteGameplayCue(SkillGCNTag, Params);
								continue;
							}
						}
						
						Params.Location = TargetActor->GetActorLocation();
						ASC->ExecuteGameplayCue(SkillGCNTag, Params);
					}
				}
			}
		}
		break;

	case ENovaSkillGCNTargetType::TargetLocation:
		{
			if (TargetData.Num() > 0)
			{
				const FGameplayAbilityTargetData* Data = TargetData.Get(0);
				if (Data && Data->HasHitResult())
				{
					Params.Location = Data->GetHitResult()->ImpactPoint;
				}
				else if (Data)
				{
					Params.Location = Data->GetEndPoint();
				}
				ASC->ExecuteGameplayCue(SkillGCNTag, Params);
			}
		}
		break;
	}
}