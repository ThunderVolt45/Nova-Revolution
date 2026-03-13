// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/NovaSelectionComponent.h"

#include "Components/WidgetComponent.h"

// Sets default values for this component's properties
UNovaSelectionComponent::UNovaSelectionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // 선택 전에는 Tick 꺼둠

	// 1. 메인 선택 원 (유닛 발밑)
	SelectionUIWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("SelectionUIWidget"));
	
	SelectionUIWidget->SetupAttachment(this);
	
	SelectionUIWidget->SetUsingAbsoluteLocation(false);
	SelectionUIWidget->SetUsingAbsoluteScale(true);
	SelectionUIWidget->SetUsingAbsoluteRotation(true);
	SelectionUIWidget->SetWidgetSpace(EWidgetSpace::World);
	SelectionUIWidget->SetWorldRotation(FRotator(90.f, 0.f, 0.f));
	SelectionUIWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SelectionUIWidget->SetVisibility(false);

	// 2. 바닥 투영 원 (공중 유닛용)
	GroundMarkerWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("GroundMarkerWidget"));
	GroundMarkerWidget->SetupAttachment(this);
	GroundMarkerWidget->SetWidgetSpace(EWidgetSpace::World);
	GroundMarkerWidget->SetUsingAbsoluteScale(true);
	GroundMarkerWidget->SetUsingAbsoluteRotation(true);
	GroundMarkerWidget->SetWorldRotation(FRotator(90.f, 0.f, 0.f));
	GroundMarkerWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundMarkerWidget->SetVisibility(false);

	// 3. 수직 안내선 (공중 유닛용)
	TraceLineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TraceLineMesh"));
	TraceLineMesh->SetupAttachment(this);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded())
	{
		TraceLineMesh->SetStaticMesh(CubeMesh.Object);
	}
	TraceLineMesh->SetUsingAbsoluteRotation(true);
	TraceLineMesh->SetWorldRotation(FRotator::ZeroRotator);
	TraceLineMesh->SetCastShadow(false);
	TraceLineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TraceLineMesh->SetVisibility(false);
}


// Called when the game starts
void UNovaSelectionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* Owner = GetOwner())
	{
		if (USceneComponent* Root = Owner->GetRootComponent())
		{
			SelectionUIWidget->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
			SelectionUIWidget->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
}


// Called every frame
void UNovaSelectionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 공중 유닛인 경우 바닥 투영
	if (bIsSelected && bIsAirUnit)
	{
		UpdateGroundProjection();
	}
}

void UNovaSelectionComponent::SetSelectionVisible(bool bVis)
{
	bIsSelected = bVis;

	// 메인 원은 항상 선택 시 보임
	if (SelectionUIWidget)
	{
		// 1. 가시성을 먼저 켬
		SelectionUIWidget->SetVisibility(bVis);
		if (bVis)
		{
			// 2. 가시성이 켜진 시점에 한 번 더 위치와 크기를 갱신 (안전장치)
			SetupSelection(CachedRadius, CachedHalfHeight, bIsAirUnit);

			// 3. 색상 적용
			SetTeamColor(CachedColor);
		}
	}

	// 공중 유닛 전용 요소들은 UpdateGroundProjection에서 제어
	if (!bVis)
	{
		if (GroundMarkerWidget)
		{
			GroundMarkerWidget->SetVisibility(false);
		}
		if (TraceLineMesh)
		{
			TraceLineMesh->SetVisibility(false);
		}
	}

	// 공중 유닛일 때만 Tick 활성
	SetComponentTickEnabled(bVis && bIsAirUnit);
}

void UNovaSelectionComponent::SetTeamColor(const FLinearColor& Color)
{
	CachedColor = Color;

	// 위젯 색상 적용
	if (SelectionUIWidget && SelectionUIWidget->GetUserWidgetObject())
	{
		SelectionUIWidget->GetUserWidgetObject()->SetColorAndOpacity(Color);
	}
	if (GroundMarkerWidget && GroundMarkerWidget->GetUserWidgetObject())
	{
		GroundMarkerWidget->GetUserWidgetObject()->SetColorAndOpacity(Color);
	}

	// 투영선 재질 색상 적용 (Dynamic Material)
	if (TraceLineMesh)
	{
		UMaterialInstanceDynamic* DynMat = TraceLineMesh->CreateDynamicMaterialInstance(0);
		if (DynMat)
		{
			DynMat->SetVectorParameterValue(TEXT("TeamColor"), Color);
		}
	}
}

void UNovaSelectionComponent::SetupSelection(float Radius, float HalfHeight, bool bIsAir)
{
	CachedRadius = Radius;
	CachedHalfHeight = HalfHeight;
	bIsAirUnit = bIsAir;
	
	// 메인 원 크기 및 위치 초기화
	float Diameter = Radius * 2.0f * 1.2f;
	SelectionUIWidget->SetDrawSize(FVector2D(Diameter, Diameter));

	// Unit의 CapsuleCollision의 높이만큼 | Base의 BoxCollision의 높이만큼 Offset으로 지정
	float VerticalOffset = HalfHeight;

	// 공중 유닛이라면 OffSet 수정
	if (bIsAirUnit) VerticalOffset += AirWidgetHeightOffset;

	SelectionUIWidget->SetRelativeLocation(FVector(0.f, 0.f, -VerticalOffset + WidgetHeightOffset));
	// Absolute Scale 사용 시 부모 스케일에 영향을 받지 않도록 1.0으로 고정
	SelectionUIWidget->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));
}

void UNovaSelectionComponent::UpdateGroundProjection()
{
	if (!GroundMarkerWidget || !TraceLineMesh) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	FVector CapsuleBottom = Owner->GetActorLocation() - FVector(0.f, 0.f, CachedHalfHeight);
	FVector Start = Owner->GetActorLocation();
	// 5000만큼 밑으로 여유있게 쏨
	FVector End = Start - FVector(0.f, 0.f, 5000.f);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
	{
		FVector GroundLoc = Hit.Location;
		// 기준점을 유닛 CollisionCapsule의 바닥 지점으로 지정(CapsuleBottom)
		float Height = CapsuleBottom.Z - GroundLoc.Z;

		// 바닥 원 업데이트
		GroundMarkerWidget->SetVisibility(true);
		GroundMarkerWidget->SetWorldLocation(GroundLoc + FVector(0.f, 0.f, 10.f));
		GroundMarkerWidget->SetDrawSize(FVector2D(GroundSelectionCircleSize, GroundSelectionCircleSize));

		// 안내선 업데이트
		if (Height > 20.f)
		{
			TraceLineMesh->SetVisibility(true);
			FVector MidPoint = (CapsuleBottom + GroundLoc) * 0.5f;
			TraceLineMesh->SetWorldLocation(MidPoint);
			TraceLineMesh->SetWorldScale3D(FVector(0.02f, 0.02f, Height / 100.f));
		}
		else
		{
			TraceLineMesh->SetVisibility(false);
		}
	}
}
