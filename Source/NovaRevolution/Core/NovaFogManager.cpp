// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/NovaFogManager.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "HairStrandsInterface.h"
#include "NovaBase.h"
#include "NovaInterfaces.h"
#include "NovaMapManager.h"
#include "NovaPlayerState.h"
#include "NovaUnit.h"
#include "Components/BoxComponent.h"
#include "GAS/NovaAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"

// Sets default values
ANovaFogManager::ANovaFogManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ANovaFogManager::BeginPlay()
{
	Super::BeginPlay();

	// 월드에서 맵 매니저 찾기
	MapManager = Cast<ANovaMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ANovaMapManager::StaticClass()));
	if (!MapManager)
	{
		UE_LOG(LogTemp, Error, TEXT("FogManager: MapManager NOT FOUND in World!"));
	}
	// 시작 시 모든 RT 초기화 (검은색)
	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), CurrentFogRT, FLinearColor::Black);
	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), HistoryFogRT, FLinearColor::White);

	// 게임 시작 시 타이머를 등록
	// UpdateInterval 간격으로 UpdateFog 함수를 무한 반복(true) 호출
	if (UpdateInterval > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(FogUpdateTimerHandle,
		                                       this,
		                                       &ANovaFogManager::UpdateFog,
		                                       UpdateInterval, // 실행 간격
		                                       true
		);
	}

	// UpdateMPCParameters();
}

void ANovaFogManager::UpdateFog()
{
	if (!CurrentFogRT || !HistoryFogRT || !MapManager) return;

	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), CurrentFogRT, FLinearColor::Black);

	// 1. Standalone 안전장치: 첫 번째 유효한 프레임에 초기화 수행
	if (!bIsFogInitialized)
	{
		// History를 White로 확실히 밀어줌
		UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), HistoryFogRT, FLinearColor::White);

		// MPC 파라미터도 이때 확실히 업데이트
		UpdateMPCParameters();

		bIsFogInitialized = true;
	}

	// 로컬 플레이어 정보 가져오기
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;
	ANovaPlayerState* PS = PC ? PC->GetPlayerState<ANovaPlayerState>() : nullptr;
	if (!PS)
	{
		UE_LOG(LogTemp, Warning, TEXT("FogManager: PlayerState is NULL!"));
		return;
	}

	// INovaTeamInterface를 통해 플레이어의 팀 확인
	int32 PlayerTeamID = PS->GetTeamID();

	// --- [최적화 적용] 등록된 액터 리스트 사용 ---
	const TArray<TWeakObjectPtr<AActor>>& RegisteredActors = MapManager->GetRegisteredActors();;

	// 시야 계산을 위한 임시 저장소
	struct FSightSource
	{
		FVector Location;
		float RadiusSq; // 계산 최적화를 위해 반지름의 제곱 저장
	};
	TArray<FSightSource> FriendlySights;
	TArray<AActor*> EnemyActors;

	// --- 드로잉 준비 ---
	FCanvas Canvas(CurrentFogRT->GameThread_GetRenderTargetResource(),
	               nullptr,
	               FGameTime::GetTimeSinceAppStart(),
	               GetWorld()->GetFeatureLevel()
	);

	// 월드 너비 계산 방식 변경
	FBox Bounds = MapManager->GetMapBounds();
	float WorldWidth = Bounds.Max.X - Bounds.Min.X;

	// --- 1차 순회 : 아군 시야 그리기 및 정보 수집 ---
	for (const TWeakObjectPtr<AActor>& WeakActor : RegisteredActors)
	{
		AActor* Actor = WeakActor.Get();
		// PlayerState에 의해 생긴 유령시야 해결 (INovaTeamInterface를 상속받았기 때문에 생긴 문제)
		if (Actor->IsA<APlayerState>()) continue;

		INovaTeamInterface* TeamActor = Cast<INovaTeamInterface>(Actor);

		if (TeamActor && TeamActor->GetTeamID() == PlayerTeamID)
		{
			float SightRadius = 800.f; // 기본 시야 값
			if (ANovaUnit* Unit = Cast<ANovaUnit>(Actor))
			{
				if (Unit->GetAbilitySystemComponent())
				{
					SightRadius = Unit->GetAbilitySystemComponent()->GetNumericAttribute(
						UNovaAttributeSet::GetSightAttribute());
				}
				// 아군 유닛은 항상 보이도록 설정
				Unit->SetFogVisibility(true);
			}
			else if (ANovaBase* Base = Cast<ANovaBase>(Actor))
			{
				// 기지의 시야 범위 설정
				if (Base->GetAbilitySystemComponent())
				{
					SightRadius = Base->GetAbilitySystemComponent()->GetNumericAttribute(
						UNovaAttributeSet::GetSightAttribute());
				}
				// 아군 기지는 항상 보이도록 설정
				Base->SetFogVisibility(true);
			}
			// 시야 정보 저장 (나중에 적군 가시성 체크용)
			FriendlySights.Add({Actor->GetActorLocation(), FMath::Square(SightRadius)});

			// UV 좌표 계산
			FVector2D UV = WorldToFogUV(Actor->GetActorLocation());

			// 캔버스에서 크기 계산 (Box 전체 너비 대비 시야 지름의 비율)
			float CanvasSize = (SightRadius * 2.0f / WorldWidth) * TextureResolution;

			// 시야 원 그리기 (중앙 정렬을 위해 CanvasSize * 0.5f 차감)
			FCanvasTileItem TileItem(UV * TextureResolution - (CanvasSize * 0.5f),
			                         BrushMaterial ? BrushMaterial->GetRenderProxy() : nullptr,
			                         FVector2D(CanvasSize, CanvasSize));

			// 블렌딩 모드 설정 (시야 원들이 서로 겹치 떄 자연스럽게 합쳐짐)
			// TileItem.BlendMode = SE_BLEND_MAX;
			TileItem.BlendMode = SE_BLEND_Additive;
			Canvas.DrawItem(TileItem);
		}
		// 적군인 경우: 일단 리스트에 담아둠 (2차 순회에 한꺼번에 체크)
		else
		{
			EnemyActors.Add(Actor);
		}
	}
	Canvas.Flush_GameThread();

	// --- 2차 순회: 적군 유닛 가시성 판단 (O(N*M)) ---
	for (AActor* Enemy : EnemyActors)
	{
		bool bIsVisible = false;
		FVector EnemyLoc = Enemy->GetActorLocation();

		// 모든 아군 시야 범위와 비교
		for (const auto& Sight : FriendlySights)
		{
			if (FVector::DistSquared(EnemyLoc, Sight.Location) < Sight.RadiusSq)
			{
				bIsVisible = true;
				break; // 하나라도 겹치면 더 볼 필요 없음
			}
		}
		// 적군 유닛의 가시성 상태 업데이트
		// Enemy->SetFogVisibility(bIsVisible);
		// 가시성 적용
		if (ANovaUnit* Unit = Cast<ANovaUnit>(Enemy))
		{
			Unit->SetFogVisibility(bIsVisible);
		}
		else if (ANovaBase* Base = Cast<ANovaBase>(Enemy))
		{
			Base->SetFogVisibility(bIsVisible);
		}
	}
}

FVector2D ANovaFogManager::WorldToFogUV(const FVector& WorldLocation) const
{
	if (MapManager)
	{
		// MapManager가 제공하는 공용 좌표 변환 함수 사용
		return MapManager->WorldToMapUV(WorldLocation);
	}

	return FVector2D(0.5f, 0.5f);
}

void ANovaFogManager::UpdateMPCParameters()
{
	if (!MapManager || !FogMPC) return;

	// 월드 바운드 가져오기
	FBox Bounds = MapManager->GetMapBounds();

	// FogOrigin: 박스의 왼쪽 아래 (Min) 좌표
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), FogMPC, "FogOrigin", FLinearColor(Bounds.Min));

	// FogSize: 박스의 가로(X) 크기 (정사각형 맵 기준)
	float WorldSize = Bounds.Max.X - Bounds.Min.X;
	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), FogMPC, "FogSize", WorldSize);
}
