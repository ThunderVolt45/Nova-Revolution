// Fill out your copyright notice in the Description page of Project Settings.
/*
Input Mapping 구조 (InputConfig) 만들기
이 작업의 목적은 블루프린트에서 "M 키(IA_Move)를 누르면 `Input.Action.Move` 태그가 발생한다"라고 설정할 수 있는 데이터 구조를 만드는 것입니다.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "NovaInputConfig.generated.h"

class UInputAction;

/**
 * FNovaInputAction
 * 입력 액션과 게임플레이 태그를 1:1로 매핑하는 구조체
 */
USTRUCT(BlueprintType)
struct FNovaInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	const UInputAction* InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag InputTag;
};

/**
 * UNovaInputConfig
 * 여러 개의 입력 액션 매핑 정보를 담고 있는 데이터 에셋
 */
UCLASS()
class NOVAREVOLUTION_API UNovaInputConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 특정 태그에 해당하는 입력 액션을 찾아 반환하는 헬퍼 함수
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = false) const;

	// 에디터에서 설정할 입력 액션 리스트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TArray<FNovaInputAction> AbilityInputActions;
};
