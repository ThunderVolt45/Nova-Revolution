// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "NovaInputConfig.h"
#include "NovaInputComponent.generated.h"

/**
 * UNovaInputComponent
 * NovaInputConfig의 태그 기반 설정을 실제 콜백 함수에 바인딩하는 커스텀 입력 컴포넌트
 */
UCLASS()
class NOVAREVOLUTION_API UNovaInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	/**
	* 입력 액션을 특정 객체의 함수와 연결하는 템플릿 함수입니다.
    * @param InputConfig 데이터 에셋 (IA와 Tag 매핑 정보)
    * @param Object 입력 콜백을 받을 대상 (보통 PlayerController 또는 Character)
    * @param PressedFunc 키를 눌렀을 때 실행될 함수
    * @param ReleasedFunc 키를 뗐을 때 실행될 함수
    * @param HeldFunc 키를 유지하고 있을 때 실행될 함수
    * 
    * UNovaInputConfig는 PrimaryDataAsset
    * 
    * 다양한 입력처리(드래그 기능 등)를 위해 입력처리를 세분화.
    * 예시)
    * PressedFunc : 마우스 좌클릭 시작 지점 저장 (드래그 시작)
    * HeldFunc : 마우스 이동에 따라 화면에 드래그 박스(UI) 업데이트
    * ReleasedFunc : 드래그 종료 시점에 사각형 안의 유닛들을 실제 선택
    * 만약에 Pressed만 필요하다면 나머지에는 nullptr을 넣어주면 됨(stop같은 경우).
	*/
	template <class UserClass, typename PressedFuncType, typename ReleasedFuncType, typename HeldFuncType>
	void BindAbilityActions(const UNovaInputConfig* InputConfig, UserClass* Object,
	                                             PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc,
	                                             HeldFuncType HeldFunc);
};

// 구현부
template <class UserClass, typename PressedFuncType, typename ReleasedFuncType, typename HeldFuncType>
void UNovaInputComponent::BindAbilityActions(const UNovaInputConfig* InputConfig, UserClass* Object,
                                             PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc,
                                             HeldFuncType HeldFunc)
{
	checkf(InputConfig, TEXT("Input Config data Asset is Null, can not proceed with binding"));
	for (const FNovaInputAction& Action : InputConfig->AbilityInputActions)
	{
		// 눌렀을 때 (Triggered)
		if (PressedFunc)
		{
			BindAction(Action.InputAction, ETriggerEvent::Started, Object, PressedFunc, Action.InputTag);
		}

		// 뗐을 때 (Completed)
		if (ReleasedFunc)
		{
			BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag);
		}

		// 유지 중일 때 (Triggered - 주로 매 프레임 발생)
		if (HeldFunc)
		{
			BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, HeldFunc, Action.InputTag);
		}
	}
}
