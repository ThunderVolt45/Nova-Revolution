// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "NovaPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UFloatingPawnMovement;

/**
 * 카메라의 4개 모서리 레이(Ray)를 바닥 평면(Z=0)과 교차시켜서, 폰의 위치로부터의 실제 거리를 계산
 */
USTRUCT(BlueprintType)
struct FCameraViewOffsets
{
	GENERATED_BODY()

	UPROPERTY()
	float Top = 0.f;
	UPROPERTY()
	float Bottom = 0.f;
	UPROPERTY()
	float Left = 0.f;
	UPROPERTY()
	float Right = 0.f;
};

UCLASS()
class NOVAREVOLUTION_API ANovaPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ANovaPawn();

	// 부드러운 줌 처리를 위해 Tick 활성화
	virtual void Tick(float DeltaTime) override;

protected:
	// 스프링암 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Camera")
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	// 카메라 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Camera")
	TObjectPtr<UCameraComponent> CameraComponent;

	// 이동을 담당하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Camera")
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	// --- 이동 관련 변수 ---
	UPROPERTY(EditAnywhere, Category = "Nova|Movement")
	float CameraMoveSpeed = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Nova|Movement")
	float CameraAcceleration = 4000.f;

	UPROPERTY(EditAnywhere, Category = "Nova|Movement")
	float CameraDeceleration = 8000.f;

	// --- 줌 관련 변수 ---

	// 기본 줌 거리
	UPROPERTY(EditAnywhere, Category="Nova|Camera")
	float DefaultZoomLength = 4000.f; //1500.f;

	// 줌 최소/최대 제한
	UPROPERTY(EditAnywhere, Category="Nova|Camera")
	float MinZoomLength = 3000.f; //500.f;

	UPROPERTY(EditAnywhere, Category="Nova|Camera")
	float MaxZoomLength = 5000.f; //2000.f;

	// 타겟 줌 거리
	UPROPERTY(EditAnywhere, Category="Nova|Camera")
	float TargetZoomLength = DefaultZoomLength;

	// 줌 속도 (보간 속도)
	UPROPERTY(EditAnywhere, Category="Nova|Camera")
	float ZoomInterpSpeed = 10.f;

public:
	void UpdateZoom(float Direction);

	void ResetCamera();

	UFUNCTION(BlueprintPure, Category = "Nova|Camera")
	FVector2D GetCameraViewExtent() const;

	/** 사다리꼴 시야 범위를 계산하여 4방향 오프셋 반환 */
	UFUNCTION(BlueprintPure, Category = "Nova|Camera")
	FCameraViewOffsets GetCameraViewOffsets() const;
};
