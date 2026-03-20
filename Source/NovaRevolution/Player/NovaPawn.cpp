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

	// 현재 ArmLength를 TargetArmLength로 부드럽게 보간
	if (!FMath::IsNearlyEqual(SpringArmComponent->TargetArmLength, TargetZoomLength))
	{
		SpringArmComponent->TargetArmLength = FMath::FInterpTo(
			SpringArmComponent->TargetArmLength,
			TargetZoomLength,
			DeltaTime,
			ZoomInterpSpeed
		);
	}
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
	FCameraViewOffsets Offsets;
	if (!CameraComponent) return Offsets;

	// 1. 현재 카메라의 월드 위치와 회전 정보
	FVector CamLoc = CameraComponent->GetComponentLocation();
	FRotator CamRot = CameraComponent->GetComponentRotation();
	float FOV = CameraComponent->FieldOfView;

	// 2. 화면 비율 계산
	float AspectRatio = 1.777f;
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		if (ViewportSize.Y > 0) AspectRatio = ViewportSize.X / ViewportSize.Y;
	}

	// 3. 카메라 시야의 4개 모서리 방향(Ray) 계산
	float HFOV_Rad = FMath::DegreesToRadians(FOV * 0.5f);
	float VFOV_Rad = FMath::Atan(FMath::Tan(HFOV_Rad) / AspectRatio);

	// 폰의 현재 위치 (Z=0 기준)
	FVector PawnLoc = GetActorLocation();
	FPlane GroundPlane(FVector::UpVector, 0.0f); // Z=0 평면

	auto GetGroundIntersection = [&](float HorizontalMult, float VerticalMult) -> FVector
	{
		// 카메라 앞방향을 기준으로 상하좌우 각도를 적용한 레이 생성
		FVector RayDir = CamRot.RotateVector(FVector(
			1.0f,
			FMath::Tan(HFOV_Rad) * HorizontalMult,
			FMath::Tan(VFOV_Rad) * VerticalMult
		)).GetSafeNormal();

		// 바닥(Z=0) 평면과의 교차점 계산 (P = CamLoc + T * RayDir)
		if (!FMath::IsNearlyZero(RayDir.Z))
		{
			float T = -CamLoc.Z / RayDir.Z;
			if (T > 0.f) // 카메라 앞쪽에서 교차할 때만
			{
				return CamLoc + (RayDir * T);
			}
		}
		return CamLoc + RayDir * 1000.f; // 실패 시 근사치
	};

	// 4개 모서리 교차점 구하기
	FVector TopLeft = GetGroundIntersection(-1.f, 1.f);
	FVector TopRight = GetGroundIntersection(1.f, 1.f);
	FVector BottomLeft = GetGroundIntersection(-1.f, -1.f);
	FVector BottomRight = GetGroundIntersection(1.f, -1.f);

	// 4. 폰의 위치(중심)를 기준으로 4방향 최대 마진 추출
	// 월드 X축이 위아래, 월드 Y축이 좌우라고 가정
	Offsets.Top = FMath::Max(TopLeft.X - PawnLoc.X, TopRight.X - PawnLoc.X);
	Offsets.Bottom = FMath::Max(PawnLoc.X - BottomLeft.X, PawnLoc.X - BottomRight.X);
	Offsets.Left = FMath::Max(PawnLoc.Y - TopLeft.Y, PawnLoc.Y - BottomLeft.Y);
	Offsets.Right = FMath::Max(TopRight.Y - PawnLoc.Y, BottomRight.Y - PawnLoc.Y);

	return Offsets;
}
