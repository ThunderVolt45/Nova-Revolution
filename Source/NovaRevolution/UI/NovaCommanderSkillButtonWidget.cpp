// Fill out your copyright notice in the Description page of Project Settings.


// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/NovaCommanderSkillButtonWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
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

void UNovaCommanderSkillButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	if (SkillButtonImage && SkillIcon)
	{
		// 텍스처를 이미지 컴포넌트에 할당합니다.
		// 이 코드는 에디터에서 아이콘을 바꿀 때마다 실시간으로 실행됩니다!
		SkillButtonImage->SetBrushFromTexture(SkillIcon);
	}
}

void UNovaCommanderSkillButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 1. SkillButton 컴포넌트가 유효한지 확인합니다 (BindWidget이 잘 되었는지).
	if (CommanderSkillButton)
	{
		// 2. 버튼의 OnClicked 델리게이트에 우리가 만든 함수를 '동적 바인딩'합니다.
		// 이 코드가 '전선'을 연결하는 역할을 합니다!
		CommanderSkillButton->OnClicked.AddDynamic(this, &UNovaCommanderSkillButtonWidget::OnButtonClicked);
	}
}

