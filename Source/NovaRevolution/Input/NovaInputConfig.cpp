// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/NovaInputConfig.h"

const UInputAction* UNovaInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag,
                                                                   bool bLogNotFound) const
{
	for (const FNovaInputAction& Action : AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag == InputTag)
		{
			return Action.InputAction;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Can't find AbilityInputAction for InputTag [%s] on InputConfig [%s]."),
		       *InputTag.ToString(), *GetNameSafe(this));
	}

	return nullptr;
}
