// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/NovaPawn.h"

#include "Camera/CameraComponent.h"
#include "Core/NovaMapManager.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

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

	// TargetZoomLength를 SpringArm의 길이로 부드럽게 보간
	SpringArmComponent->TargetArmLength = FMath::FInterpTo(
		SpringArmComponent->TargetArmLength,
		TargetZoomLength,
		DeltaTime,
		ZoomInterpSpeed
	);
	
	// 이동 계산이 끝난 후 즉시 위치 제한(렌더링 직전)
	ClampLocation();
}

void ANovaPawn::BeginPlay()
{
	Super::BeginPlay();
	
	// 월드에서 MapManager 찾아서 캐싱
	MapManager = Cast<ANovaMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ANovaMapManager::StaticClass()));

	// 이동 컴포넌트가 먼저 계산된 후 Pawn의 Tick이 실행되도록 설정 (떨림 방지)
	if (MovementComponent)
	{
		AddTickPrerequisiteComponent(MovementComponent);
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
	if (!CameraComponent || !SpringArmComponent) return Offsets;

	// 1. 카메라의 실제 월드 위치와 회전값 가져오기
	FVector CamLoc = CameraComponent->GetComponentLocation();
	FRotator CamRot = CameraComponent->GetComponentRotation();
	float CamZ = CamLoc.Z; // 지면(Z=0)으로부터의 실제 높이

	// 카메라가 지면에 너무 가까우면 계산 오류가 날 수 있으므로 최소값 방어
	if (CamZ < 10.f) CamZ = 10.f;

	// 2. FOV 및 화면 비율 계산
	float HalfHFOV = FMath::DegreesToRadians(CameraComponent->FieldOfView * 0.5f);
	float AspectRatio = 1.777f;
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		if (ViewportSize.Y > 0) AspectRatio = ViewportSize.X / ViewportSize.Y;
	}

	// 3. 카메라 방향 벡터들
	FVector CamForward = CamRot.Vector();
	FVector CamRight = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
	FVector CamUp = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Z);

	// 4. 카메라의 4개 모서리 방향 벡터 계산
	// 줌 0일 때도 카메라의 실제 위치에서 바닥(Z=0) 평면으로 레이를 쏴서 교차점을 찾습니다.
	auto GetPlaneIntersection = [&](float HorizontalMultiplier, float VerticalMultiplier) -> FVector
	{
		// 화면 모서리로 향하는 방향 벡터
		FVector RayDir = CamForward
			+ (CamRight * FMath::Tan(HalfHFOV) * HorizontalMultiplier)
			+ (CamUp * (FMath::Tan(HalfHFOV) / AspectRatio) * VerticalMultiplier);
		RayDir.Normalize();

		// 지면(Z=0)과의 교차점 계산
		if (FMath::IsNearlyZero(RayDir.Z)) return CamLoc + RayDir * 10000.f;

		float t = -CamLoc.Z / RayDir.Z;
		return CamLoc + RayDir * t;
	};

	// 4개 꼭짓점의 월드 좌표
	FVector TopLeft = GetPlaneIntersection(-1.f, 1.f);
	FVector TopRight = GetPlaneIntersection(1.f, 1.f);
	FVector BottomLeft = GetPlaneIntersection(-1.f, -1.f);
	FVector BottomRight = GetPlaneIntersection(1.f, -1.f);

	// 5. 폰 위치(GetActorLocation)를 기준으로 오프셋 계산
	FVector PawnLoc = GetActorLocation();

	// 월드 X는 위아래(Top/Bottom), 월드 Y는 좌우(Left/Right)
	Offsets.Top = (TopLeft.X + TopRight.X) * 0.5f - PawnLoc.X;
	Offsets.Bottom = PawnLoc.X - (BottomLeft.X + BottomRight.X) * 0.5f;
	Offsets.Left = PawnLoc.Y - (TopLeft.Y + BottomLeft.Y) * 0.5f;
	Offsets.Right = (TopRight.Y + BottomRight.Y) * 0.5f - PawnLoc.Y;

	return Offsets;
}

void ANovaPawn::ClampLocation()
{
	if (!MapManager) return;

	FBox MapBox = MapManager->GetMapBounds();
	FVector CurrentLoc = GetActorLocation();

	// 현재 카메라 시야에 따른 오프셋 계산
	FCameraViewOffsets Offsets = GetCameraViewOffsets();

	// 맵 경계 계산
	float MinX = FMath::Min(MapBox.Min.X + Offsets.Bottom - ExtraScrollMargins, MapBox.GetCenter().X);
	float MaxX = FMath::Max(MapBox.Max.X - Offsets.Top, MapBox.GetCenter().X);
	float MinY = FMath::Min(MapBox.Min.Y + Offsets.Left, MapBox.GetCenter().Y);
	float MaxY = FMath::Max(MapBox.Max.Y - Offsets.Right, MapBox.GetCenter().Y);

	FVector ClampedLoc = CurrentLoc;
	ClampedLoc.X = FMath::Clamp(CurrentLoc.X, MinX, MaxX);
	ClampedLoc.Y = FMath::Clamp(CurrentLoc.Y, MinY, MaxY);

	// 실제 위치가 범위를 벗어났다면
	if (!CurrentLoc.Equals(ClampedLoc, 0.1f))
	{
		SetActorLocation(ClampedLoc);

		// 어느 축이 나갔는지 판정하여 속도 제어 (떨림 방지 2단계)
		bool bOutX = !FMath::IsNearlyEqual(CurrentLoc.X, ClampedLoc.X, 0.1f);
		bool bOutY = !FMath::IsNearlyEqual(CurrentLoc.Y, ClampedLoc.Y, 0.1f);

		StopMovementOnAxis(bOutX, bOutY);
	}
}

void ANovaPawn::StopMovementOnAxis(bool bStopX, bool bStopY)
{
	if (MovementComponent)
	{
		FVector NewVelocity = MovementComponent->Velocity;

		if (bStopX) NewVelocity.X = 0.f;
		if (bStopY) NewVelocity.Y = 0.f;

		MovementComponent->Velocity = NewVelocity;

		// 만약 두 축 모두 멈춰야 한다면 아예 이동 입력을 비웁니다.
		if (bStopX && bStopY)
		{
			MovementComponent->StopMovementImmediately();
		}
	}
}
