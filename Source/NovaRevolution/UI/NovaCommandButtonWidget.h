// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/NovaTypes.h"
#include "NovaCommandButtonWidget.generated.h"

/**
 * 유닛 명령(이동, 공격, 정지 등)을 수행하는 버튼 위젯
 */
UCLASS()
class NOVAREVOLUTION_API UNovaCommandButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 이 버튼이 수행할 명령 타입 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Command")
	ECommandType CommandType = ECommandType::None;

	/** 버튼에 표시될 명령 아이콘 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	TObjectPtr<UTexture2D> CommandIcon;

protected:
	/** 블루프린트 위젯의 Image 컴포넌트와 바인딩 (이름: CommandImage) */
	UPROPERTY(meta = (BindWidget))
	class UImage* CommandImage;

	/** 블루프린트 위젯의 Button 컴포넌트와 바인딩 (이름: CommandButton) */
	UPROPERTY(meta = (BindWidget))
	class UButton* CommandButton;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	/** 버튼 클릭 시 실제 명령을 컨트롤러에 전달 */
	UFUNCTION()
	void OnButtonClicked();

	/** 선택된 유닛 배열이 변경될 때 가시성을 업데이트하기 위한 핸들러 */
	UFUNCTION()
	void HandleSelectionChanged(const TArray<AActor*>& SelectedUnits);

	/** 현재 선택된 유닛들 중 내가 명령을 내릴 수 있는 유닛이 있는지 체크하는 헬퍼 함수 */
	bool HasControllableUnit(const TArray<AActor*>& SelectedUnits) const;
};
