// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "NovaPlayerController.generated.h"

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
	
	// --- 입력 시스템 설정 ---
	// 커스텀 입력 컴포넌트를 사용하기 위해 오버라이드 합니다.
	virtual void SetupInputComponent() override;
	
	// 에디터에서 할당할 입력 설정 데이터 에셋
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Input")
	TObjectPtr<class UNovaInputConfig> InputConfig;
	
	// --- 입력 처리 함수 (콜백) ---
	// UNovaInputComponent에서 태그를 매개변수로 넘겨주게 됩니다.
	void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);
	void Input_AbilityInputTagHeld(FGameplayTag InputTag);
	
	// --- 유닛 선택 관리 ---
	// 현재 마우스로 선택한 액터들을 담아두는 배열입니다.
	UPROPERTY(BlueprintReadOnly, Category="Nova|Selection")
	TArray<TObjectPtr<AActor>> SelectedUnits;
	
private:
	// 마우스 커서 아래에 무엇이 있는지 감지하는 헬퍼 함수
	void GetCursorHitResult(FHitResult& OutHitResult);
	
	// 헬퍼 함수 : 선택 비우기
	void ClearSelection();
};
