// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/UI/NovaWarningPopupWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UNovaWarningPopupWidget::SetMessage(FText InMessage)
{
	// 전달받은 경고 메시지를 UI 텍스트 블록에 적용합니다.
	if (MessageText)
	{
		MessageText->SetText(InMessage);
	}
}

void UNovaWarningPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 블루프린트에서 바인딩된 확인 버튼에 클릭 이벤트 등록
	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.AddDynamic(this, &UNovaWarningPopupWidget::OnConfirmClicked);
	}
}

void UNovaWarningPopupWidget::OnConfirmClicked()
{
	// 확인 버튼 클릭 시 위젯을 부모(Viewport 등)로부터 제거하여 팝업을 닫습니다.
	// 만약 닫기 애니메이션이 필요하다면 BlueprintImplementableEvent를 선언하여 
	// BP에서 애니메이션 재생 후 RemoveFromParent를 호출하도록 확장할 수 있습니다.
	RemoveFromParent();
}
