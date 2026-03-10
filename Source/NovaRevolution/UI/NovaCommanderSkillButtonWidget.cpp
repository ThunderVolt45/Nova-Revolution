// Fill out your copyright notice in the Description page of Project Settings.


// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/NovaCommanderSkillButtonWidget.h"
#include "Core/NovaInterfaces.h"
#include "GameFramework/PlayerState.h"

void UNovaCommanderSkillButtonWidget::OnButtonClicked()
{
	// 1. 설정된 태그가 유효하지 않으면 실행하지 않습니다.
	if (!ActionAbilityTag.IsValid())
	{
		return;
	}

	// 2. 이 위젯을 소유한 플레이어의 PlayerState를 가져옵니다.
	APlayerState* PS = GetOwningPlayerState();
	if (!PS)
	{
		return;
	}

	// 3. PlayerState가 사령관 인터페이스(INovaCommanderInterface)를 구현하고 있는지 확인합니다.
	if (PS->GetClass()->ImplementsInterface(UNovaCommanderInterface::StaticClass()))
	{
		/**
		 * 4. 인터페이스 함수를 호출하여 사령관에게 스킬 실행을 요청합니다.
		 * BlueprintNativeEvent로 선언된 인터페이스 함수를 C++에서 호출할 때는
		 * IInterfaceName::Execute_FunctionName(대상, 인자...) 형식을 사용합니다.
		 */
		INovaCommanderInterface::Execute_ActivateCommanderAbility(PS, ActionAbilityTag);
	}
}

