// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Styling/SlateBrush.h"
#include "Layout/Visibility.h"

/**
 * Slate 기반의 로딩 화면 위젯
 * 부드러운 진행률 표시를 위해 실제 로딩값과 보간(Interpolation) 처리를 수행합니다.
 */
class SNovaLoadingScreenWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNovaLoadingScreenWidget)
		: _LoadingImageBrush(nullptr)
	{}
		/** 배경으로 표시할 이미지 브러쉬 */
		SLATE_ARGUMENT(const FSlateBrush*, LoadingImageBrush)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SNovaLoadingScreenWidget();
	
private:
	/** 배경 이미지 브러쉬 */
	const FSlateBrush* BackgroundBrush = nullptr;
};
