// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/NovaMapManager.h"

#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/GameplayStatics.h"
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
	AerialNavBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

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

	// 게임 시작 시 투명 벽 자동 생성
	SetupBoundaryWalls();

	// 맵 시작 시 자동으로 배경 캡처 수행
	CaptureMapBackground();

	// Standalone 모드에서는 월드 로딩 완료를 위해 약간의 지연 후 캡처하는 것이 안전함
	FTimerHandle CaptureTimerHandle;
	// GetWorldTimerManager().SetTimer(CaptureTimerHandle, this, &ANovaMapManager::CaptureMapBackground, 0.2f, false);
	GetWorldTimerManager().SetTimer(CaptureTimerHandle, [this]()
	{
		if (IsValid(this))
		{
			CaptureMapBackground(); // 최종 촬영
			ClearMinimapCapture(); // 이제 더 이상 필요 없으므로 파괴
		}
	}, 0.2f, false);
}

void ANovaMapManager::SetupBoundaryWalls()
{
	if (!MapBounds) return;

	// 1. 기준 정보 가져오기 (절반 크기인 Extent 사용)
	FVector Center = MapBounds->GetComponentLocation();
	FVector Extent = MapBounds->GetUnscaledBoxExtent();
	float HalfThick = BoundaryWallThickness * 0.5f;
	float HalfHeight = BoundaryWallHeight * 0.5f;

	// 2. 4가지 방향(위, 아래, 왼쪽, 오른쪽) 설정 데이터
	struct FWallData
	{
		FVector Location;
		FVector BoxExtent;
		FString Name;
	};

	TArray<FWallData> WallConfigs;
	// 위쪽 (X+)
	WallConfigs.Add({
		Center + FVector(Extent.X + HalfThick, 0.f, HalfHeight),
		FVector(HalfThick, Extent.Y + BoundaryWallThickness, HalfHeight), TEXT("Wall_North")
	});
	// 아래쪽 (X-)
	WallConfigs.Add({
		Center + FVector(-Extent.X - HalfThick, 0.f, HalfHeight),
		FVector(HalfThick, Extent.Y + BoundaryWallThickness, HalfHeight), TEXT("Wall_South")
	});
	// 오른쪽 (Y+)
	WallConfigs.Add({
		Center + FVector(0.f, Extent.Y + HalfThick, HalfHeight),
		FVector(Extent.X + BoundaryWallThickness, HalfThick, HalfHeight), TEXT("Wall_East")
	});
	// 왼쪽 (Y-)
	WallConfigs.Add({
		Center + FVector(0.f, -Extent.Y - HalfThick, HalfHeight),
		FVector(Extent.X + BoundaryWallThickness, HalfThick, HalfHeight), TEXT("Wall_West")
	});

	// 3. 실제 컴포넌트 생성 및 설정
	for (const FWallData& Config : WallConfigs)
	{
		UBoxComponent* NewWall = NewObject<UBoxComponent>(this, FName(*Config.Name));
		if (NewWall)
		{
			NewWall->RegisterComponent();
			NewWall->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);

			// 위치 및 크기 설정
			NewWall->SetWorldLocation(Config.Location);
			NewWall->SetBoxExtent(Config.BoxExtent);

			// 중요: 콜리전 설정
			// 마우스 클릭(Visibility)은 통과시키고, 유닛(Pawn)만 막음
			NewWall->SetCollisionProfileName(TEXT("Custom"));
			NewWall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			NewWall->SetCollisionObjectType(ECC_WorldStatic);
			NewWall->SetCollisionResponseToAllChannels(ECR_Block);
			NewWall->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore); // 클릭 통과
			NewWall->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // 카메라 통과

			NewWall->SetHiddenInGame(true); // 게임 중에는 안 보이게

			BoundaryWalls.Add(NewWall);
		}
	}
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

FBox ANovaMapManager::GetFogBounds() const
{
	FBox Bounds = GetMapBounds();
	// 상하좌우로 FogPadding만큼 영역을 확장합니다. (Z는 확장 불필요)
	return Bounds.ExpandBy(FVector(FogPadding, FogPadding, 0.f));
}

FVector2D ANovaMapManager::WorldToFogUV(const FVector& WorldLocation) const
{
	FBox FogBounds = GetFogBounds();
	FVector Center = FogBounds.GetCenter();

	// 확장된 영역의 가로/세로 중 긴 쪽을 기준으로 정규화
	float MaxSide = FMath::Max(FogBounds.Max.X - FogBounds.Min.X, FogBounds.Max.Y - FogBounds.Min.Y);

	float U = (WorldLocation.X - Center.X) / MaxSide + 0.5f;
	float V = (WorldLocation.Y - Center.Y) / MaxSide + 0.5f;

	// 안개 그리기용이므로 Clamp를 수행하여 0~1 밖으로 나가지 않게 합니다.
	return FVector2D(FMath::Clamp(U, 0.f, 1.f), FMath::Clamp(V, 0.f, 1.f));
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

	// 관리 중인 액터(RegisteredActors) 조회
	TArray<AActor*> ActorsToHide;
	for (const TWeakObjectPtr<AActor>& WeakActor : RegisteredActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			ActorsToHide.Add(Actor);
		}
	}

	// MinimapCapture에서 지정한 Actor들을 제외
	MinimapCapture->HiddenActors = ActorsToHide;

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

	// 숨김 리스트 비우기 (메모리 참조 해제)
	MinimapCapture->HiddenActors.Empty();

	// NOVA_LOG(Log, TEXT("MapManager: Map Background Captured (Width: %.f)"), MinimapCapture->OrthoWidth);
}

void ANovaMapManager::ClearMinimapCapture()
{
	if (MinimapCapture)
	{
		// 1. 렌더 타겟 연결 해제 (참조 제거)
		MinimapCapture->TextureTarget = nullptr;

		// 2. 컴포넌트 비활성화
		MinimapCapture->SetActive(false);

		// 3. 컴포넌트 완전 제거 (메모리 해제)
		MinimapCapture->DestroyComponent();

		// 4. 포인터 초기화 (안전성 확보)
		MinimapCapture = nullptr;
	}
}
