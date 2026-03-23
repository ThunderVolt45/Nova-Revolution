// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/NovaPawn.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"

// Sets default values
ANovaPawn::ANovaPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Root 컴포넌트(충돌체로 사용 가능)
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Pawn이 회전해도 카메라 지지대는 회전하지 않도록 설정
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 스프링암 설정
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = TargetZoomLength; // 초기 거리
	SpringArmComponent->SetRelativeRotation(FRotator(-75.f, 0.f, 0.f)); // 기본 각도
	SpringArmComponent->bDoCollisionTest = false; // 지형 충돌로 카메라가 튀는 걸 방지

	// SpringArm이 컨트롤러의 회전을 상속받지 않도록 명시
	SpringArmComponent->bUsePawnControlRotation = false;
	SpringArmComponent->bInheritPitch = false;
	SpringArmComponent->bInheritYaw = false;
	SpringArmComponent->bInheritRoll = false;

	// 카메라 이동 지연(부드러운 이동을 위해)
	// SpringArmComponent->bEnableCameraLag = true;
	// SpringArmComponent->CameraLagSpeed = 5.f;
	// 체감상 불편하여 제외
	SpringArmComponent->bEnableCameraLag = false;

	// 카메라 설정
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
	CameraComponent->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	CameraComponent->ProjectionMode = ECameraProjectionMode::Perspective;
	// CameraComponent->OrthoWidth = TargetZoomLength;// 초기값 설정

	// 이동 컴포넌트
	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->bConstrainToPlane = true;
	MovementComponent->SetPlaneConstraintNormal(FVector(0.f, 0.f, 1.f));

	// 변수 값을 컴포넌트에 적용
	MovementComponent->MaxSpeed = CameraMoveSpeed;
	MovementComponent->Acceleration = CameraAcceleration;
	MovementComponent->Deceleration = CameraDeceleration;
}

void ANovaPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	/*
	// TargetZoomLength를 OrthoWidth로 부드럽게 보간
	CameraComponent->OrthoWidth = FMath::FInterpTo(
		CameraComponent->OrthoWidth,
		TargetZoomLength, // TargetZoomLength를 직접 사용
		DeltaTime,
		ZoomInterpSpeed
	);
	*/
	
	// TargetZoomLength를 SpringArm의 길이로 부드럽게 보간
	SpringArmComponent->TargetArmLength = FMath::FInterpTo(
	SpringArmComponent->TargetArmLength,
	TargetZoomLength,
	DeltaTime,
	ZoomInterpSpeed
);
}

void ANovaPawn::UpdateZoom(float Direction)
{
	// 입력 방향에 따라 목표 거리 계산 (한번 굴릴 때 마다 200 unit씩 증감 예시)
	TargetZoomLength = FMath::Clamp(TargetZoomLength + (Direction * -200.f), MinZoomLength, MaxZoomLength);
}

void ANovaPawn::ResetCamera()
{
	// 초기 줌 값으로 설정
	TargetZoomLength = DefaultZoomLength;
}

FVector2D ANovaPawn::GetCameraViewExtent() const
{
	if (!CameraComponent || !SpringArmComponent) return FVector2D::ZeroVector;

	// 1. 카메라의 FOV 절반값 (라디안)
	float HalfFOV = FMath::DegreesToRadians(CameraComponent->FieldOfView * 0.5f);

	// 2. 현재 카메라 지지대(SpringArm)의 길이
	float ArmLength = SpringArmComponent->TargetArmLength;

	// 3. 화면 비율(Aspect Ratio) 가져오기
	float AspectRatio = 1.777f; // 기본 16:9
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		if (ViewportSize.Y > 0)
		{
			AspectRatio = ViewportSize.X / ViewportSize.Y;
		}
	}

	// 4. 삼각함수를 이용한 바닥 투영 거리 계산
	// 카메라가 약 -75도로 누워있으므로, ArmLength에 따른 앞뒤 시야 범위를 근사치로 계산합니다.
	float VerticalMargin = ArmLength * FMath::Tan(HalfFOV);
	float HorizontalMargin = VerticalMargin * AspectRatio;

	return FVector2D(VerticalMargin, HorizontalMargin);
}

FCameraViewOffsets ANovaPawn::GetCameraViewOffsets() const
{
	/*
	FCameraViewOffsets Offsets;
	if (!CameraComponent) return Offsets;

	// 1. 현재 화면의 가로 너비 (줌에 따라 실시간 반영)
	float OrthoWidth = CameraComponent->OrthoWidth;

	// 2. 화면 비율(Aspect Ratio) 계산 (기본 16:9)
	float AspectRatio = 1.777f;
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		if (ViewportSize.Y > 0) AspectRatio = ViewportSize.X / ViewportSize.Y;
	}

	// 3. 직사각형 시야 범위 계산 (PawnLoc 중심에서 화면 끝까지의 거리)
	// Orthographic 모드에서는 좌우 거리가 Width의 절반입니다.
	float HalfWidth = OrthoWidth * 0.5f;

	// 상하 거리는 AspectRatio에 따라 달라집니다 (Height = Width / AspectRatio)
	float HalfHeight = HalfWidth / AspectRatio;

	// 4. 오프셋 설정 (모든 방향이 대칭인 직사각형)
	// RTS 조작계가 카메라를 75도로 내려다보고 있더라도 직교 투영은 왜곡이 없으므로
	// 상하좌우가 동일한 거리로 계산되어야 완벽한 직사각형 클램핑이 가능합니다.
	Offsets.Left = HalfWidth;
	Offsets.Right = HalfWidth;
	Offsets.Top = HalfHeight;
	Offsets.Bottom = HalfHeight;

	return Offsets;
	*/
	FCameraViewOffsets Offsets;
	if (!CameraComponent || !SpringArmComponent) return Offsets;

	// 1. 필요한 기본 정보 가져오기
	float ArmLength = SpringArmComponent->TargetArmLength;
	float Pitch = -SpringArmComponent->GetRelativeRotation().Pitch; // 보통 75도
	float HalfHFOV = FMath::DegreesToRadians(CameraComponent->FieldOfView * 0.5f);

	// 화면 비율 계산
	float AspectRatio = 1.777f;
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		if (ViewportSize.Y > 0) AspectRatio = ViewportSize.X / ViewportSize.Y;
	}

	// 2. 수직 FOV(VFOV) 계산
	float HalfVFOV = FMath::Atan(FMath::Tan(HalfHFOV) / AspectRatio);

	// 3. 카메라 높이(Z)와 바닥 오프셋(X) 계산
	float RadPitch = FMath::DegreesToRadians(Pitch);
	float CamZ = ArmLength * FMath::Sin(RadPitch);
	float CamXOffset = ArmLength * FMath::Cos(RadPitch);

	// 4. 상하단 바닥 투영 거리 계산 (수직선 기준 각도 사용)
	float CenterAngle = FMath::DegreesToRadians(90.f - Pitch); // 카메라 중심축이 수직선과 이루는 각도

	// Pawn 위치(중심)로부터 상단/하단 경계까지의 월드 거리
	float DistToTop = CamZ * FMath::Tan(CenterAngle + HalfVFOV);
	float DistToBottom = CamZ * FMath::Tan(CenterAngle - HalfVFOV);

	Offsets.Top = DistToTop - CamXOffset;
	Offsets.Bottom = CamXOffset - DistToBottom;

	// 5. 좌우 너비 계산 (RTS 클램핑을 위해 가장 넓은 상단 지점 기준 너비 사용)
	// 실제로는 사다리꼴이지만, 현재 구조(Left/Right 하나씩)를 유지하기 위해 가장 넉넉한 값을 반환합니다.
	float RayToTopDist = CamZ / FMath::Cos(CenterAngle + HalfVFOV); // 카메라에서 상단 경계까지의 직선 거리
	float HalfWidthAtTop = RayToTopDist * FMath::Tan(HalfHFOV);

	Offsets.Left = HalfWidthAtTop;
	Offsets.Right = HalfWidthAtTop;

	return Offsets;
}
