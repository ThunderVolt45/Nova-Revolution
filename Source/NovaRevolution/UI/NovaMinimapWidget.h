// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovaMinimapWidget.generated.h"

class ANovaMapManager;
class ANovaFogManager;
class ANovaPlayerController;

/**
 * 
 */
UCLASS()
class NOVAREVOLUTION_API UNovaMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** 좌표 변환을 담당하는 맵 매니저 참조 */
	UPROPERTY(Transient)
	TObjectPtr<ANovaMapManager> MapManager;

	// --- 에디터 설정 변수 ---

	/** 미니맵 배경 머터리얼 (M_Minimap) */
	UPROPERTY(EditAnywhere, Category="Nova|Minimap")
	TObjectPtr<UMaterialInterface> MinimapMaterial;

	/** 유닛을 나타낼 점의 크기 */
	UPROPERTY(EditAnywhere, Category="Nova|Minimap")
	float UnitIconSize = 5.0f;

	/** 카메라 영역 선의 두께 */
	UPROPERTY(EditAnywhere, Category="Nova|Minimap")
	float CameraLineThickness = 1.0f;

	/** 유닛 아이콘에 사용할 브러시 (에디터에서 흰색 사각형이나 원형 텍스처 지정) */
	UPROPERTY(EditAnywhere, Category = "Nova|Minimap")
	FSlateBrush UnitIconBrush;

	// --- 런타임 참조 ---

	UPROPERTY()
	TObjectPtr<ANovaFogManager> FogManager;

	UPROPERTY()
	TObjectPtr<ANovaPlayerController> NovaPC;

	// 다이나믹 머터리얼 인스턴스 (런타임에 안개 텍스처 주입용)
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MinimapMaterialInst;

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 슬레이트 엔진의 그리기 함수 오버라이드 (유닛 점, 카메라 사각형 그리기) */
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	                          const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	                          const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// --- 마우스 상호 작용 ---
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	// 미니맵을 클릭한 채로 마우스를 움직일 때 카메라가 실시간으로 따라오게 만들기 위한 상태 값
	bool bIsDragging = false;

	/** 마우스 위치(위젯 좌표)를 월드 좌표로 변환하여 카메라 이동 */
	void HandleMinimapClick(const FGeometry& MyGeometry, const FVector2D& MouseScreenPos);

	/** UV 좌표를 월드 좌표로 변환 */
	FVector MinimapUVToWorld(const FVector2D& UV) const;
};
