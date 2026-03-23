// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/NovaFogManager.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "NovaInterfaces.h"
#include "NovaMapManager.h"
#include "NovaPlayerState.h"
#include "NovaRevolution.h"
#include "GameFramework/GameStateBase.h"
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

	// 로컬 플레이어 정보 가져오기
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		NOVA_LOG(Warning, "PlayerController is NULL!");
		return;
	}
	ANovaPlayerState* PS = PC->GetPlayerState<ANovaPlayerState>();
	if (!PS)
	{
		NOVA_LOG(Warning, "PlayerState is NULL!");
		return;
	}

	// 1. Standalone 안전장치: 첫 번째 유효한 프레임에 초기화 수행
	if (!bIsFogInitialized)
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), HistoryFogRT, FLinearColor::White);
		UpdateMPCParameters();
		bIsFogInitialized = true;
	}

	// 2. 초기 설정 및 팀 정보 수집
	int32 LocalPlayerTeamID = PS->GetTeamID();
	const TArray<TWeakObjectPtr<AActor>>& RegisteredActors = MapManager->GetRegisteredActors();
	FBox Bounds = MapManager->GetMapBounds();
	float WorldWidth = Bounds.Max.X - Bounds.Min.X;

	TSet<int32> ActiveTeamSet;
	ActiveTeamSet.Add(LocalPlayerTeamID);
	if (AGameStateBase* GS = GetWorld()->GetGameState())
	{
		for (APlayerState* PlayerState : GS->PlayerArray)
		{
			if (INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(PlayerState))
			{
				int32 TeamID = TeamInterface->GetTeamID();
				if (TeamID >= 0 && TeamID < 32) ActiveTeamSet.Add(TeamID);
			}
		}
	}

	// --- 1차 순회: 모든 액터를 한 번만 순회하며 팀별 시야 소스 분류 ---
	struct FSightSource
	{
		FVector Location;
		float RadiusSq;
	};
	
	// 팀 ID를 키로 하는 시야 소스 맵
	TMap<int32, TArray<FSightSource>> TeamSightsMap;
	
	// 가시성 판단 대상이 될 모든 유닛/기지 리스트
	TArray<AActor*> AllSelectableActors;

	for (const TWeakObjectPtr<AActor>& WeakActor : RegisteredActors)
	{
		AActor* Actor = WeakActor.Get();
		if (!Actor || Actor->IsA<APlayerState>()) continue;

		INovaTeamInterface* TeamActor = Cast<INovaTeamInterface>(Actor);
		INovaVisibilityInterface* VisibilityInterface = Cast<INovaVisibilityInterface>(Actor);

		// 가시성 판단 대상 목록에 추가 (유닛, 기지, GCN 등)
		if (TeamActor || VisibilityInterface)
		{
			AllSelectableActors.Add(Actor);
		}

		if (!TeamActor) continue;

		int32 ActorTeamID = TeamActor->GetTeamID();

		// 이 액터가 시야를 제공할 수 있는지 확인 (ASC의 Sight 속성)
		float SightRadius = 800.f;
		if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Actor))
		{
			if (UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent())
			{
				SightRadius = ASC->GetNumericAttribute(UNovaAttributeSet::GetSightAttribute());
			}
		}

		// 해당 팀의 시야 리스트에 추가 (시야 제공 주체)
		if (ActiveTeamSet.Contains(ActorTeamID) && VisibilityInterface)
		{
			TeamSightsMap.FindOrAdd(ActorTeamID).Add({Actor->GetActorLocation(), FMath::Square(SightRadius)});

			// 자기 팀 유닛은 해당 팀에게 항상 보이도록 설정
			INovaVisibilityInterface::Execute_SetVisibilityForTeam(Actor, ActorTeamID, true);
		}
	}

	// 확장된 안개 바운드 정보 사용
	FBox FogBounds = MapManager->GetFogBounds();
	float FogWorldWidth = FogBounds.Max.X - FogBounds.Min.X;
	
	// --- 2차 순회: 각 팀별 가시성 판단 및 렌더링 ---
	for (int32 CurrentTeamID : ActiveTeamSet)
	{
		bool bIsLocalPlayerTeam = (CurrentTeamID == LocalPlayerTeamID);
		const TArray<FSightSource>* CurrentTeamSights = TeamSightsMap.Find(CurrentTeamID);

		// 3. 로컬 플레이어 팀인 경우에만 렌더 타겟 그리기 (시각적 안개)
		if (bIsLocalPlayerTeam)
		{
			UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), CurrentFogRT, FLinearColor::Black);
			FCanvas Canvas(CurrentFogRT->GameThread_GetRenderTargetResource(), nullptr, FGameTime::GetTimeSinceAppStart(), GetWorld()->GetFeatureLevel());

			if (CurrentTeamSights)
			{
				for (const auto& Sight : *CurrentTeamSights)
				{
					// FVector2D UV = WorldToFogUV(Sight.Location);
					// [수정] 확장된 영역 전용 UV 함수 사용
					FVector2D UV = MapManager->WorldToFogUV(Sight.Location);
					float Radius = FMath::Sqrt(Sight.RadiusSq);
					
					// float CanvasSize = (Radius * 2.0f / WorldWidth) * TextureResolution;
					// [수정] 확장된 가로 길이를 기준으로 캔버스 크기 계산
					float CanvasSize = (Radius * 2.0f / FogWorldWidth) * TextureResolution;

					FCanvasTileItem TileItem(UV * TextureResolution - (CanvasSize * 0.5f),
					                         BrushMaterial ? BrushMaterial->GetRenderProxy() : nullptr,
					                         FVector2D(CanvasSize, CanvasSize));
					TileItem.BlendMode = SE_BLEND_Additive;
					Canvas.DrawItem(TileItem);
				}
			}
			Canvas.Flush_GameThread();
		}

		// 4. 모든 대상 액터들에 대해 현재 팀(CurrentTeamID)의 가시성 판단
		for (AActor* TargetActor : AllSelectableActors)
		{
			INovaTeamInterface* TargetTeam = Cast<INovaTeamInterface>(TargetActor);

			// 팀이 있는 경우, 자기 팀 유닛은 이미 위에서 시야 소스 단계에서 처리했으므로 건너뜀 (최적화)
			if (TargetTeam && TargetTeam->GetTeamID() == CurrentTeamID)
			{
				if (bIsLocalPlayerTeam)
				{
					if (INovaVisibilityInterface::Execute_GetFogVisibility(TargetActor) == false)
					{
						INovaVisibilityInterface::Execute_SetFogVisibility(TargetActor, true);
					}
				}
				continue;
			}

			bool bVisible = false;
			if (CurrentTeamSights)
			{
				FVector TargetLoc = TargetActor->GetActorLocation();
				for (const auto& Sight : *CurrentTeamSights)
				{
					if (FVector::DistSquared2D(TargetLoc, Sight.Location) < Sight.RadiusSq)
					{
						bVisible = true;
						break;
					}
				}
			}

			// 가시성 인터페이스를 통한 통합 업데이트
			if (INovaVisibilityInterface* VisibilityInterface = Cast<INovaVisibilityInterface>(TargetActor))
			{
				INovaVisibilityInterface::Execute_SetVisibilityForTeam(TargetActor, CurrentTeamID, bVisible);
				if (bIsLocalPlayerTeam)
				{
					INovaVisibilityInterface::Execute_SetFogVisibility(TargetActor, bVisible);
				}
			}
		}
	}
}

FVector2D ANovaFogManager::WorldToFogUV(const FVector& WorldLocation) const
{
	if (MapManager)
	{
		// MapManager가 제공하는 공용 좌표 변환 함수 사용
		return MapManager->WorldToFogUV(WorldLocation);
	}

	return FVector2D(0.5f, 0.5f);
}

void ANovaFogManager::UpdateMPCParameters()
{
	if (!MapManager || !FogMPC) return;

	// 확장된 안개 바운드 정보를 셰이더에 전달
	FBox Bounds = MapManager->GetFogBounds();

	// FogOrigin: 박스의 왼쪽 아래 (Min) 좌표
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), FogMPC, "FogOrigin", FLinearColor(Bounds.Min));

	// FogSize: 박스의 가로(X) 크기 (정사각형 맵 기준)
	float WorldSize = Bounds.Max.X - Bounds.Min.X;
	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), FogMPC, "FogSize", WorldSize);
}
