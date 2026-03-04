// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/NovaPawn.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

// Sets default values
ANovaPawn::ANovaPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Root 컴포넌트(충돌체로 사용 가능)
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	
	// 카메라 설정
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(RootComponent);
	CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 1500.f));
	CameraComponent->SetRelativeRotation(FRotator(-90.f, 0.0f, 0.0f));
	
	// 이동 컴포넌트
	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->bConstrainToPlane = true;
	MovementComponent->SetPlaneConstraintNormal(FVector(0.f, 0.f, 1.f));
}

