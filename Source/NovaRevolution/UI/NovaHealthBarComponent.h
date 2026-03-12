// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "NovaHealthBarComponent.generated.h"

class UProgressBar;

/**
 * 유닛 및 기지에 사용되는 체력바 컴포넌트
 */
UCLASS(ClassGroup=(Nova), meta = (BlueprintSpawnableComponent))
class NOVAREVOLUTION_API UNovaHealthBarComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UNovaHealthBarComponent();

protected:
	virtual void BeginPlay() override;

public:
	/** 현재 체력 상태와 최대 체력, 팀 색상을 받아 UI를 갱신합니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|UI")
	void UpdateHealthBar(float CurrentHP, float MaxHP, FLinearColor Color);

	/** 안개 시스템에 의해 가려짐 여부를 설정합니다. */
	void SetFogVisibility(bool bVis);

	/** 플레이어 컨트롤러의 체력바 표시 옵션 변경 시 호출됩니다. */
	UFUNCTION()
	void OnHealthBarOptionChanged(bool bShow);

	/** 현재 체력바가 보여야 하는지 최종 판단하여 가시성을 적용합니다. */
	void UpdateVisibility();

protected:
	// --- 내부 상태 변수 ---
	bool bIsVisibleByFog = true;
	bool bHealthBarOptionEnabled = true;

	// --- 체력바 설정 ---
	// 체력바의 세로 길이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	float HealthBarHeight = 15.0f;

	// 체력바의 최소 가로 길이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	float MinHealthBarWidth = 1.0f;

	// 체력바의 최대 가로 길이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	float MaxHealthBarWidth = 220.0f;

	// 로그 스케일 계산 시 사용할 비중 수치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	float HealthBarLogScaleFactor = 40.0f;

private:
	// 체력바 길이 계산 헬퍼 함수
	void UpdateHealthBarLength(float MaxHP);
};
