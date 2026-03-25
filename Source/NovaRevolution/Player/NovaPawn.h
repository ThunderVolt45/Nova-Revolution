// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "NovaPawn.generated.h"

class ANovaMapManager;
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
	ANovaPawn();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	// 스프링암 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Camera")
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	// 카메라 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Camera")
	TObjectPtr<UCameraComponent> CameraComponent;

	// 이동을 담당하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Camera")
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	// 맵 매니저 캐싱
	UPROPERTY(Transient)
	TObjectPtr<ANovaMapManager> MapManager;

	// --- 이동 관련 변수 ---

	UPROPERTY(EditAnywhere, Category = "Nova|Movement")
	float CameraMoveSpeed = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Nova|Movement")
	float CameraAcceleration = 4000.f;

	UPROPERTY(EditAnywhere, Category = "Nova|Movement")
	float CameraDeceleration = 8000.f;

	// 카메라의 추가 이동 허용치 (UI에 플레이 화면 가려짐 방지)
	UPROPERTY(EditAnywhere, Category = "Nova|Camera")
	float ExtraScrollMargins = 700.f;

	// --- 줌 관련 변수 ---

	// 기본 줌 거리
	UPROPERTY(EditAnywhere, Category="Nova|Camera")
	float DefaultZoomLength = 1500.f;

	// 줌 최소 제한
	UPROPERTY(EditAnywhere, Category="Nova|Camera")
	float MinZoomLength = 0.f;

	// 줌 최대 제한
	UPROPERTY(EditAnywhere, Category="Nova|Camera")
	float MaxZoomLength = 2000.f;

	// 타겟 줌 거리
	UPROPERTY(EditAnywhere, Category="Nova|Camera")
	float TargetZoomLength = DefaultZoomLength;

	// 줌 속도 (보간 속도)
	UPROPERTY(EditAnywhere, Category="Nova|Camera")
	float ZoomInterpSpeed = 10.f;

public:
	// 확대 적용
	void UpdateZoom(float Direction);

	// Default 높이로 회귀
	void ResetCamera();

	UFUNCTION(BlueprintPure, Category = "Nova|Camera")
	FVector2D GetCameraViewExtent() const;

	/** 사다리꼴 시야 범위를 계산하여 4방향 오프셋 반환 */
	UFUNCTION(BlueprintPure, Category = "Nova|Camera")
	FCameraViewOffsets GetCameraViewOffsets() const;

	/** 스스로의 위치를 맵 범위 내로 제한하는 함수 */
	void ClampLocation();

	/** 맵 끝에 도달했을 때 특정 축의 속도를 0으로 만들어 떨림 방지 */
	void StopMovementOnAxis(bool bStopX, bool bStopY);
};
