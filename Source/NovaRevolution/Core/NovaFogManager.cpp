// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/NovaFogManager.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "NovaInterfaces.h"
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

	// FogVolume
	FogVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("FogVolume"));
	RootComponent = FogVolume;
}

// Called when the game starts or when spawned
void ANovaFogManager::BeginPlay()
{
	Super::BeginPlay();

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

	UpdateMPCParameters();
}

void ANovaFogManager::UpdateFog()
{
	if (!CurrentFogRT || !HistoryFogRT || !FogVolume) return;

	// 현재 시야 RT 초기화 (검은색)
	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), CurrentFogRT, FLinearColor::Black);

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

	// 월드 내 모든 아군 유닛/건물 순회 -> 나중에 최적화할 부분. 유닛이 생산될 때 FogManager에 올리는 방법 권장.
	// 직접 순회해서 찾는건 성능에 영향을 미칠 수 있음
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UNovaTeamInterface::StaticClass(), FoundActors);

	// 시야 계산을 위한 임시 저장소
	struct FSightSource
	{
		FVector Location;
		float RadiusSq; // 계산 최적화를 위해 반지름의 제곱 저장
	};
	TArray<FSightSource> FriendlySights;
	TArray<ANovaUnit*> EnemyUnits;

	// --- 드로잉 준비 ---
	FCanvas Canvas(CurrentFogRT->GameThread_GetRenderTargetResource(),
	               nullptr,
	               FGameTime::GetTimeSinceAppStart(),
	               GetWorld()->GetFeatureLevel()
	);

	float WorldWidth = FogVolume->Bounds.GetBox().Max.X - FogVolume->Bounds.GetBox().Min.X;

	// --- 1차 순회 : 아군 시야 그리기 및 정보 수집 ---
	for (AActor* Actor : FoundActors)
	{
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
			if (ANovaUnit* EnemyUnit = Cast<ANovaUnit>(Actor))
			{
				EnemyUnits.Add(EnemyUnit);
			}
		}
	}
	Canvas.Flush_GameThread();

	// --- 2차 순회: 적군 유닛 가시성 판단 (O(N*M)) ---
	for (ANovaUnit* Enemy : EnemyUnits)
	{
		bool bIsVisible = false;
		FVector EnemyLoc = Enemy->GetActorLocation();

		// 모든 아군 시야 범위와 비교
		for (const auto& Sight : FriendlySights)
		{
			float DistSq = FVector::DistSquared(EnemyLoc, Sight.Location);
			if (DistSq < Sight.RadiusSq)
			{
				bIsVisible = true;
				break; // 하나라도 겹치면 더 볼 필요 없음
			}
		}
		// 적군 유닛의 가시성 상태 업데이트
		Enemy->SetFogVisibility(bIsVisible);
	}
}

FVector2D ANovaFogManager::WorldToFogUV(const FVector& WorldLocation) const
{
	if (!FogVolume)
	{
		return FVector2D(0.5f, 0.5f);
	}

	FBox Bounds = FogVolume->Bounds.GetBox();

	// 월드 X, Y를 0~1 범위의 UV로 변환
	float U = (WorldLocation.X - Bounds.Min.X) / (Bounds.Max.X - Bounds.Min.X);
	float V = (WorldLocation.Y - Bounds.Min.Y) / (Bounds.Max.Y - Bounds.Min.Y);

	// Y축(V) 반전 여부는 머터리얼 설정에 따라 다를 수 있지만 기본적으로는 0~1 클램핑만 수행
	return FVector2D(FMath::Clamp(U, 0.f, 1.f), FMath::Clamp(V, 0.f, 1.f));
}

void ANovaFogManager::UpdateMPCParameters()
{
	if (!FogVolume || !FogMPC) return;

	// FogVolume(BoxComponent)의 월드 좌표 바운드 가져오기
	FBox Bounds = FogVolume->Bounds.GetBox();

	// FogOrigin: 박스의 왼쪽 아래 (Min) 좌표
	// PostProcess 머터리얼에서 (WorldPos - Origin) / Size 계산을 위해 필요
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), FogMPC, "FogOrigin", FLinearColor(Bounds.Min));

	// FogSize: 박스의 가로(X) 크기 (정사각형 맵 기준)
	float WorldSize = Bounds.Max.X - Bounds.Min.X;
	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), FogMPC, "FogSize", WorldSize);
}
