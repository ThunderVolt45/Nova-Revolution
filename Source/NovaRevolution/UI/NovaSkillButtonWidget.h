// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "NovaSkillButtonWidget.generated.h"

/**
 * UNovaCommanderSkillButtonWidget
 * 사령관 스킬 버튼의 공통 로직을 담당하는 베이스 클래스입니다.
 * 이 클래스를 상속받아 WBP_CommanderSkillButton을 제작합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaSkillButtonWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/**
	 * 이 버튼이 눌렸을 때 실행할 사령관 스킬의 Gameplay Tag입니다.
	 * 에디터에서 각 버튼 인스턴스마다 다르게 설정하여 스킬을 할당합니다.
	 * (예: Ability.Commander.ResourceLevelUp)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Ability")
	FGameplayTag ActionAbilityTag;
	
	/** 버튼에 표시될 스킬 아이콘 이미지 (에디터에서 버튼마다 다르게 설정 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	TObjectPtr<UTexture2D> SkillIcon;
	
protected:
	/**
	* 블루프린트 위젯(WBP)에 이름이 정확히 'SkillImage'인 Image 컴포넌트가 있어야 합니다.
	*/
	UPROPERTY(meta = (BindWidget))
	class UImage* SkillButtonImage;
	
	/**
	 * 블루프린트 위젯(WBP)에 이름이 정확히 'SkillButton'인 Button 컴포넌트가 있어야 합니다.
	 */
	UPROPERTY(meta = (BindWidget))
	class UButton* CommanderSkillButton;

protected:
	/**
	 * 버튼 클릭 시 호출될 로직입니다. Button 컴포넌트의 OnClicked 이벤트에서 이 함수를 호출하도록 연결합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Ability")
	void OnButtonClicked();
	
	/**
	* NativePreConstruct는 블루프린트의 PreConstruct 이벤트와 동일하게 작동합니다. 에디터 디자인 시와 게임 실행 시 모두 호출됩니다.
	*/
	virtual void NativePreConstruct() override;
	
	/**
	 * NativeConstruct는 블루프린트의 Construct 이벤트와 동일하게 작동합니다. 위젯이 생성될 때 딱 한 번 실행됩니다.
	 */
	virtual void NativeConstruct() override;
};
