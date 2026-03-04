// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NovaHUD.generated.h"

/**
 * 
 */
UCLASS()
class NOVAREVOLUTION_API ANovaHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	ANovaHUD();
	
	// 매 프레임 그리기 이벤트를 처리하는 엔진 기본 함수
	virtual void DrawHUD() override;
	
	/**
	 * 컨트롤러에서 호출할 드래그 정보 업데이트 함수
	 * @param Start 클릭 시작 지점
	 * @param End 현재 마우스 지점
	 * @param bInIsDragging 현재 드래그 중인지 여부
	 */
	void UpdateDragRect(FVector2D Start, FVector2D End, bool bInIsDragging);
	
protected:
	// 박스 내부 색상
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|DragBox")
	FLinearColor BoxFillColor = FLinearColor(1.f, 1.f, 1.f, 0.5f);
	
	// 박스 테두리 색상
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|DragBox")
	FLinearColor BoxOutlineColor = FLinearColor(1.f, 1.f, 1.f, 1.f);
	
	// 테두리 선의 두께
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|DragBox")
	float BoxOutlineThickness = 1.5f;
	
private:
	// 런타임 드래그 데이터
	FVector2D StartPosition;
	FVector2D EndPosition;
	bool bIsDragging = false;
};
