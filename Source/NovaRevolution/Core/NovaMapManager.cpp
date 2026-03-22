// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/NovaMapManager.h"

#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/KismetRenderingLibrary.h"

// Sets default values
ANovaMapManager::ANovaMapManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// 실제 맵 범위를 정의할 박스 컴포넌트 생성
	MapBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("MapBounds"));
	RootComponent = MapBounds;
	// 기본 설정 (에디터에서 육안으로 확인하기 편하게 설정)
	MapBounds->SetBoxExtent(FVector(10000.f, 10000.f, 500.f));
	MapBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 공중 유닛용 내비게이션 바닥 추가
	AerialNavBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("AerialNavBounds"));
	AerialNavBounds->SetupAttachment(RootComponent);
	// 중요: 내비게이션에 영향을 주도록 설정
	AerialNavBounds->SetCanEverAffectNavigation(true);
	// 충돌 설정: 유닛은 통과하지만 NavMesh는 생성되도록 'QueryOnly' 설정
	AerialNavBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AerialNavBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	// 게임 내 가시성 제거
	AerialNavBounds->SetHiddenInGame(true); 
	// 에디터에서만 영역을 확인할 수 있도록 설정 (선택 사항)
	AerialNavBounds->SetLineThickness(2.0f);
	
	// 미니맵 캡처 컴포넌트 설정
	MinimapCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapCapture"));
	MinimapCapture->SetupAttachment(RootComponent);

	// 평행 투영(Orthographic) 모드 설정 (원근감 제거)
	MinimapCapture->ProjectionType = ECameraProjectionMode::Orthographic;

	// 성능 최적화: 매 프레임 찍지 않고 수동(CaptureScene)으로만 찍음
	MinimapCapture->bCaptureEveryFrame = false;
	MinimapCapture->bCaptureOnMovement = false;

	// 라이팅과 지형만 찍도록 설정 (필요 시 조정) 수정 SCS_FinalColorLDR -> SCS_BaseColor
	MinimapCapture->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
}

void ANovaMapManager::BeginPlay()
{
	Super::BeginPlay();

	// 맵 시작 시 자동으로 배경 캡처 수행
	CaptureMapBackground();
	
	// Standalone 모드에서는 월드 로딩 완료를 위해 약간의 지연 후 캡처하는 것이 안전함
	FTimerHandle CaptureTimerHandle;
	GetWorldTimerManager().SetTimer(CaptureTimerHandle, this, &ANovaMapManager::CaptureMapBackground, 0.2f, false);
}

void ANovaMapManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	if (MapBounds && AerialNavBounds)
	{
		// 1. 부모(MapBounds)의 현재 Extent(가로, 세로, 높이) 가져오기
		FVector ParentExtent = MapBounds->GetUnscaledBoxExtent();

		// 2. 자식(AerialNavBounds)의 Extent 설정
		// X, Y는 부모와 똑같이, Z(두께)는 얇게(10) 고정
		AerialNavBounds->SetBoxExtent(FVector(ParentExtent.X, ParentExtent.Y, 10.0f));
		
		AerialNavBounds->SetRelativeLocation(FVector(0.f, 0.f, AerialNavHeight));
	}
}

FBox ANovaMapManager::GetMapBounds() const
{
	if (!MapBounds) return FBox(ForceInit);
	return MapBounds->Bounds.GetBox();
}

FVector2D ANovaMapManager::WorldToMapUV(const FVector& WorldLocation) const
{
	FBox Bounds = GetMapBounds();
	float MaxSide = FMath::Max(Bounds.Max.X - Bounds.Min.X, Bounds.Max.Y - Bounds.Min.Y);
	FVector Center = Bounds.GetCenter();

	// 중앙(0.5, 0.5) 기준 비율 계산 (U=X, V=Y)
	float U = (WorldLocation.X - Center.X) / MaxSide + 0.5f;
	float V = (WorldLocation.Y - Center.Y) / MaxSide + 0.5f;

	return FVector2D(FMath::Clamp(U, 0.f, 1.f), FMath::Clamp(V, 0.f, 1.f));
}

FVector ANovaMapManager::UVToWorldLocation(const FVector2D& UV, float ZHeight) const
{
	/*
	FBox Bounds = GetMapBounds();
	FVector Size = Bounds.GetSize();

	// UV(0~1) 비율을 실제 월드 좌표로 역산 (Lerp와 동일한 원리)
	float WorldX = Bounds.Min.X + (UV.X * Size.X);
	float WorldY = Bounds.Min.Y + (UV.Y * Size.Y);

	return FVector(WorldX, WorldY, ZHeight);
	*/
	FBox Bounds = GetMapBounds();
	float MaxSide = FMath::Max(Bounds.Max.X - Bounds.Min.X, Bounds.Max.Y - Bounds.Min.Y);
	FVector Center = Bounds.GetCenter();

	// WorldToMapUV의 역산: (UV - 0.5) * MaxSide + Center
	float WorldX = (UV.X - 0.5f) * MaxSide + Center.X;
	float WorldY = (UV.Y - 0.5f) * MaxSide + Center.Y;

	return FVector(WorldX, WorldY, ZHeight);
}

void ANovaMapManager::RegisterActor(AActor* Actor)
{
	RegisteredActors.AddUnique(Actor);
}

void ANovaMapManager::UnregisterActor(AActor* Actor)
{
	RegisteredActors.Remove(Actor);
}

void ANovaMapManager::CaptureMapBackground()
{
	if (!MinimapCapture || !MinimapBackgroundRT || !MapBounds) return;

	// 1. 맵 박스의 중앙 위치와 크기 계산
	FBox Bounds = MapBounds->CalcBounds(GetActorTransform()).GetBox();
	FVector Center = Bounds.GetCenter();
	float WorldWidth = Bounds.Max.X - Bounds.Min.X;
	float WorldHeight = Bounds.Max.Y - Bounds.Min.Y;

	// 2. 카메라를 맵 중앙 위로 이동 (수직 아래를 보도록 설정)
	MinimapCapture->SetWorldLocation(FVector(Center.X, Center.Y, CaptureHeight));
	MinimapCapture->SetWorldRotation(FRotator(-90.0f, 0.0f, 0.0f));

	// 3. 카메라의 평행 투영 너비(OrthoWidth)를 맵 가로 크기에 맞춤
	// (정사각형 맵이라면 X, Y 중 큰 값을 선택)
	MinimapCapture->OrthoWidth = FMath::Max(WorldWidth, WorldHeight);

	// 4. 렌더 타겟 연결 및 캡처 실행
	MinimapCapture->TextureTarget = MinimapBackgroundRT;

	// 캡처 전 렌더 타겟 초기화 (선택 사항)
	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), MinimapBackgroundRT, FLinearColor::Black);

	// 실제 1회 캡처 수행
	MinimapCapture->CaptureScene();

	// NOVA_LOG(Log, TEXT("MapManager: Map Background Captured (Width: %.f)"), MinimapCapture->OrthoWidth);
}
