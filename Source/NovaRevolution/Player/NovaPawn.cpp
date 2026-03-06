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
	SpringArmComponent->TargetArmLength = DefaultZoomLength; // 초기 거리
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
	if (!FMath::IsNearlyEqual(SpringArmComponent->TargetArmLength, DefaultZoomLength))
	{
		SpringArmComponent->TargetArmLength = FMath::FInterpTo(
			SpringArmComponent->TargetArmLength,
			DefaultZoomLength,
			DeltaTime,
			ZoomInterpSpeed
		);
	}
}

void ANovaPawn::UpdateZoom(float Direction)
{
	// 입력 방향에 따라 목표 거리 계산 (한번 굴릴 때 마다 200 unit씩 증감 예시)
	DefaultZoomLength = FMath::Clamp(DefaultZoomLength + (Direction * -200.f), MinZoomLength, MaxZoomLength);
}
