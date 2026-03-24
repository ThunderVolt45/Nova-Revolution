// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SNovaLoadingScreenWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Styling/CoreStyle.h"

void SNovaLoadingScreenWidget::Construct(const FArguments& InArgs)
{
	BackgroundBrush = InArgs._LoadingImageBrush;

	ChildSlot
	[
		SNew(SOverlay)
		// 1. 전체 배경색 (이미지가 없는 영역 보완)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SColorBlock)
			.Color(FLinearColor::Black)
		]
		// 2. 로딩 이미지 (비율 유지하며 채우기)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit) // 4:3 비율을 유지하며 수직으로 가득 차게 설정
			.StretchDirection(EStretchDirection::Both)
			[
				SNew(SImage)
				.Image(BackgroundBrush)
				.Visibility_Lambda([this]() { return BackgroundBrush ? EVisibility::Visible : EVisibility::Collapsed; })
			]
		]
	];
}

SNovaLoadingScreenWidget::~SNovaLoadingScreenWidget()
{
	// 할당된 브러쉬가 있다면 메모리 해제
	if (BackgroundBrush)
	{
		delete BackgroundBrush;
		BackgroundBrush = nullptr;
	}
}
