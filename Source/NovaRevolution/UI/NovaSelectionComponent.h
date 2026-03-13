// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "NovaSelectionComponent.generated.h"

class UWidgetComponent;
class UStaticMeshComponent;

/**
 * 유닛 및 기지의 선택 표시를 관리하는 컴포넌트
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOVAREVOLUTION_API UNovaSelectionComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UNovaSelectionComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	/** 선택 가시성을 설정합니다. */
	void SetSelectionVisible(bool bVis);

	/** 팀 색상을 일괄 적용합니다. (위젯 및 안내선 재질) */
	void SetTeamColor(const FLinearColor& Color);

	/** 액터의 크기와 공중 유닛 여부에 따라 컴포넌트 설정을 초기화합니다. */
	void SetupSelection(float Radius, float HalfHeight, bool bIsAir);

protected:
	// --- 내부 관리 컴포넌트 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	TObjectPtr<UWidgetComponent> SelectionUIWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	TObjectPtr<UWidgetComponent> GroundMarkerWidget; // GroundMarkerWidget로 이름 변경 예정

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	TObjectPtr<UStaticMeshComponent> TraceLineMesh; // TraceLineMesh으로 이름 변경 예정

	// --- 설정값 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	float GroundSelectionCircleSize = 20.0f;

	// 위젯이 바닥에 묻히는 걸 막기위해 높이를 올려줄 Offset값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	float WidgetHeightOffset = 2.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	float AirWidgetHeightOffset = 50.0f;

private:
	// 현재 상태 캐싱
	bool bIsAirUnit = false;
	bool bIsSelected = false;
	float CachedRadius = 0.0f;
	float CachedHalfHeight = 0.0f;
	FLinearColor CachedColor = FLinearColor::White;

	/** 공중 유닛의 바닥 투영 업데이트합니다. */
	void UpdateGroundProjection();
};
