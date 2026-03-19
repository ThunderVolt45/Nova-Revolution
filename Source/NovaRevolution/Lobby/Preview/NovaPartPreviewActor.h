// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NovaPartPreviewActor.generated.h"

/**
 * ANovaPartPreviewActor
 * 부품을 특정 위치에 스폰하고, 실시간으로 회전시키며 전용 카메라로 촬영하는 클래스입니다.
 */
UCLASS()
class NOVAREVOLUTION_API ANovaPartPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	ANovaPartPreviewActor();

	/** 매 프레임 부품을 회전시키기 위해 Tick을 활성화합니다. */
	virtual void Tick(float DeltaTime) override;

	/** * 새로운 부품 클래스를 받아 프리뷰를 교체하고 출력될 렌더 타겟을 설정합니다.
	 * @param PartClass 스폰할 부품 클래스
	 * @param TargetTexture 촬영 결과물이 투사될 렌더 타겟
	 */
	void UpdatePreview(TSubclassOf<class ANovaPart> PartClass, class UTextureRenderTarget2D* TargetTexture);

protected:
	/** 부품이 부착되어 회전의 중심이 될 루트 컴포넌트 */
	UPROPERTY(VisibleAnywhere, Category = "Nova|Preview")
	TObjectPtr<USceneComponent> PreviewRoot;

	/** 부품만을 선별적으로 촬영할 전용 캡처 컴포넌트 */
	UPROPERTY(VisibleAnywhere, Category = "Nova|Preview")
	TObjectPtr<class USceneCaptureComponent2D> CaptureComponent;

	/** 부품이 자동으로 회전하는 속도 (도/초) */
	UPROPERTY(EditAnywhere, Category = "Nova|Preview")
	float RotationSpeed = 45.0f;

	/** 현재 월드에 스폰되어 프리뷰 중인 부품 인스턴스 */
	UPROPERTY()
	TObjectPtr<class ANovaPart> CurrentPreviewPart;
};