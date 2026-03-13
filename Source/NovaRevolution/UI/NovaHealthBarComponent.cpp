// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/NovaHealthBarComponent.h"

#include "Components/ProgressBar.h"
#include "Player/NovaPlayerController.h"

UNovaHealthBarComponent::UNovaHealthBarComponent()
{
	// 기본 설정
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(false);
	SetUsingAbsoluteScale(true);

	// 초기에는 보이도록 설정
	SetVisibility(true);
}

void UNovaHealthBarComponent::BeginPlay()
{
	Super::BeginPlay();

	// 플레이어 컨트롤러의 델리게이트 바인딩 및 초기 상태 동기화
	if (ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		// 초기 상태 동기화
		bHealthBarOptionEnabled = PC->GetShowHealthBars();

		// 델리게이트 바인딩
		PC->OnShowHealthBarsChanged.AddDynamic(this, &UNovaHealthBarComponent::OnHealthBarOptionChanged);
	}

	// 초기 가시성 업데이트
	UpdateVisibility();
}

void UNovaHealthBarComponent::UpdateHealthBar(float CurrentHP, float MaxHP, FLinearColor Color)
{
	if (!GetUserWidgetObject()) return;

	// 위젯 내부의 ProgressBar 찾기 (BP에서 HealthBar로 이름 설정)
	UProgressBar* HealthBar = Cast<UProgressBar>(GetUserWidgetObject()->GetWidgetFromName(TEXT("HealthBar")));
	if (HealthBar)
	{
		float HPPercent = (MaxHP > 0.f) ? (CurrentHP / MaxHP) : 0.f;
		HealthBar->SetPercent(HPPercent);
		HealthBar->SetFillColorAndOpacity(Color);
	}

	// 길이 업데이트도 함께 수행
	UpdateHealthBarLength(MaxHP);
}

void UNovaHealthBarComponent::SetFogVisibility(bool bVis)
{
	bIsVisibleByFog = bVis;
	UpdateVisibility();
}

void UNovaHealthBarComponent::OnHealthBarOptionChanged(bool bShow)
{
	bHealthBarOptionEnabled = bShow;
	UpdateVisibility();
}

void UNovaHealthBarComponent::UpdateVisibility()
{
	bool bShouldBeVisible = bIsVisibleByFog && bHealthBarOptionEnabled;
	SetVisibility(bShouldBeVisible);
}

void UNovaHealthBarComponent::UpdateHealthBarLength(float MaxHP)
{
	// 로그 스케일 계산
	float LogHPVal = FMath::Loge(FMath::Max(1.0f, MaxHP / 100.0f));

	// 가로 길이 계산
	float TargetWidth = MinHealthBarWidth + (LogHPVal * HealthBarLogScaleFactor);

	// 범위 제한
	TargetWidth = FMath::Clamp(TargetWidth, MinHealthBarWidth, MaxHealthBarWidth);

	// 적용
	SetDrawSize(FVector2D(TargetWidth, HealthBarHeight));
}
