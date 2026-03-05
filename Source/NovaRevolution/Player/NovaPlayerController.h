// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "NovaPlayerController.generated.h"

struct FInputActionValue;
class UInputMappingContext;

/**
 * ANovaPlayerController
 * 플레이어의 입력을 받아 유닛을 선택하고 명령을 내리는 전술 지휘관 클래스
 */
UCLASS()
class NOVAREVOLUTION_API ANovaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANovaPlayerController();

protected:
	virtual void BeginPlay() override;
	
	// 커스텀 입력 컴포넌트를 사용하기 위해 오버라이드 합니다.
	virtual void SetupInputComponent() override;

	// 현재 드래그 상태인지 여부
	bool bIsDraggingBox = false;
	
	// 현재 Shift(다중 선택) 모드인지 여부
	bool bIsShiftDown = false;
	
	// 에디터에서 할당할 IMC
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Input")
	TObjectPtr<UInputMappingContext> IMC;

	// 에디터에서 할당할 입력 설정 데이터 에셋
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Input")
	TObjectPtr<class UNovaInputConfig> InputConfig;

	// 에디터에서 할당할 카메라 이동 입력 액션
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Input")
	TObjectPtr<class UInputAction> MoveCameraAction;
	
	// 현재 마우스로 선택한 액터들을 담아두는 배열
	UPROPERTY(BlueprintReadOnly, Category="Nova|Selection")
	TArray<TObjectPtr<AActor>> SelectedUnits;
	
	// 드래그 시작 시점의 화면 좌표 (픽셀 단위)
	FVector2D DragStartPos;
	
	// UNovaInputComponent에서 태그를 매개변수로 넘겨주게 됩니다.
	// 마우스 Pressed
	void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	
	// 마우스 Released
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);
	
	// 마우스 Held
	void Input_AbilityInputTagHeld(FGameplayTag InputTag);

	// 실제 카메라 이동 처리 함수
	void MoveCamera(const FInputActionValue& Value);

	// 드래그 선택을 수행하는 실제 판정 함수
	void PerformBoxSelection();
	
protected:
	/** 화면에 띄울 메인 HUD 위젯 클래스 (블루프린트에서 설정) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|UI")
	TSubclassOf<class UUserWidget> MainHUDClass;

	/** 생성된 HUD 인스턴스 저장용 */
	UPROPERTY()
	class UUserWidget* MainHUDInstance;

private:
	// 마우스 커서 아래에 무엇이 있는지 감지하는 헬퍼 함수
	void GetCursorHitResult(FHitResult& OutHitResult);

	// 헬퍼 함수 : 선택 비우기
	void ClearSelection();
};
