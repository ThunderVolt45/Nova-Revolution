// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovaWarningPopupWidget.generated.h"

/**
 * UNovaWarningPopupWidget
 * 조립 오류나 시스템 경고 메시지를 표시하는 범용 팝업 위젯입니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaWarningPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** * 외부(LobbyManager 등)에서 호출하여 팝업에 표시할 텍스트를 설정합니다. 
	 * @param InMessage 표시할 경고 내용
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|UI")
	void SetMessage(FText InMessage);

protected:
	/** 위젯 생성 시 버튼 이벤트를 바인딩합니다. */
	virtual void NativeConstruct() override;

	/** '확인' 버튼 클릭 시 호출되어 팝업을 닫는 등의 처리를 수행합니다. */
	UFUNCTION()
	void OnConfirmClicked();

	/** * 경고 메시지가 출력될 텍스트 블록입니다. 
	 * WBP에서 'MessageText'라는 이름의 TextBlock이 반드시 존재해야 합니다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> MessageText;

	/** * 팝업을 닫기 위한 확인 버튼입니다. 
	 * WBP에서 'ConfirmButton'이라는 이름의 Button이 반드시 존재해야 합니다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> ConfirmButton;
};
