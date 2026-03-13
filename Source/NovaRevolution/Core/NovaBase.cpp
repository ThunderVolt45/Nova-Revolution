// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaBase.h"
#include "Core/NovaGameMode.h"
#include "Core/NovaPlayerState.h"
#include "Core/NovaResourceComponent.h"
#include "Core/NovaAssemblyTypes.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GAS/NovaAttributeSet.h"
#include "Core/Production/NovaUnitFactory.h"
#include "NovaRevolution.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/NovaPlayerController.h"
#include "UI/NovaHealthBarComponent.h"
#include "UI/NovaSelectionComponent.h"

ANovaBase::ANovaBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 박스 컴포넌트 생성 및 루트 설정 (유닛의 캡슐 역할)
	BaseCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BaseCollision"));
	RootComponent = BaseCollision;
	BaseCollision->SetBoxExtent(FVector(200.f, 200.f, 100.f)); // 기본 크기 설정
	BaseCollision->SetCollisionProfileName(TEXT("Pawn"));

	// 메시 컴포넌트 생성 및 루트 설정
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	// RootComponent = BaseMesh;
	if (BaseMesh)
	{
		BaseMesh->SetupAttachment(RootComponent);
	}

	// 선택 원 위젯
	SelectionComponent = CreateDefaultSubobject<UNovaSelectionComponent>(TEXT("SelectionComponent"));
	SelectionComponent->SetupAttachment(RootComponent);

	// 체력바 위젯
	HealthBarComponent = CreateDefaultSubobject<UNovaHealthBarComponent>(TEXT("HealthBarComponent"));
	HealthBarComponent->SetupAttachment(RootComponent);

	// ASC 및 AttributeSet 구성
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UNovaAttributeSet>(TEXT("AttributeSet"));

	// 초기 설정
	TeamID = NovaTeam::None;
	RallyPoint = FVector::ZeroVector;
}

void ANovaBase::BeginPlay()
{
	Super::BeginPlay();

	// 색상 캐싱
	InitializeUIColors();

	// 초기 UI 트랜스폼 및 데이터 설정
	if (SelectionComponent && BaseCollision)
	{
		FVector BoxExtent = BaseCollision->GetScaledBoxExtent();
		// 박스의 X, Y 중 큰 값을 반지름으로 사용
		float Radius = FMath::Max(BoxExtent.X, BoxExtent.Y);
		float HalfHeight = BoxExtent.Z;

		SelectionComponent->SetupSelection(Radius, HalfHeight, false); // 기지는 지상 판정
		SelectionComponent->SetTeamColor(CachedUIColor);
	}
	UpdateHealthBar();

	// 랠리 포인트 초기값 설정 (기지 앞쪽 500 유닛 지점 예시)
	RallyPoint = GetActorLocation() + GetActorForwardVector() * 500.0f;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		AttributeSet->SetSight(1200.f);

		AttributeSet->SetMaxHealth(20000.f);
		AttributeSet->SetHealth(20000.f);

		// 체력 변경 콜백 등록
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
		                      .AddUObject(this, &ANovaBase::OnHealthChanged);
	}
}

void ANovaBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 체력바 화면 아래 방향 앵커링
	if (HealthBarComponent && HealthBarComponent->IsVisible())
	{
		if (APlayerCameraManager* CamManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager)
		{
			FVector ScreenDownDir = -CamManager->GetCameraRotation().Quaternion().GetUpVector();
			FVector BoxBottom = GetActorLocation() - FVector(0.f, 0.f, BaseCollision->GetScaledBoxExtent().Z);

			// 기지 반지름(MaxSide) + 추가 오프셋
			float Radius = FMath::Max(BaseCollision->GetScaledBoxExtent().X, BaseCollision->GetScaledBoxExtent().Y);
			FVector FinalAnchorLoc = BoxBottom + (ScreenDownDir * (Radius + 50.0f));

			HealthBarComponent->SetWorldLocation(FinalAnchorLoc);
		}
	}
}

void ANovaBase::SetTeamID(int32 NewTeamID)
{
	TeamID = NewTeamID;

	// 팀 ID가 설정되었으므로, 이에 맞춰 UI 색상을 다시 계산하고 업데이트
	InitializeUIColors();
	if (SelectionComponent)
	{
		SelectionComponent->SetTeamColor(CachedUIColor);
	}
	UpdateHealthBar(); // 체력바 색상 즉시 갱신

	// 해당 팀의 PlayerState를 찾아 자신을 등록
	UWorld* World = GetWorld();
	if (!World) return;

	// 월드 내의 모든 PlayerState를 검색하여 TeamID가 일치하는 것을 찾음
	TArray<AActor*> FoundPlayerStates;
	UGameplayStatics::GetAllActorsOfClass(World, ANovaPlayerState::StaticClass(), FoundPlayerStates);

	for (AActor* Actor : FoundPlayerStates)
	{
		if (ANovaPlayerState* PS = Cast<ANovaPlayerState>(Actor))
		{
			if (PS->GetTeamID() == TeamID)
			{
				PS->SetPlayerBase(this);
				NOVA_LOG(Log, "Base '%s' registered to PlayerState of team %d", *GetName(), TeamID);
				return;
			}
		}
	}
}

UAbilitySystemComponent* ANovaBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ANovaBase::OnSelected()
{
	bIsSelected = true;
	if (SelectionComponent)
	{
		SelectionComponent->SetSelectionVisible(true);
		SelectionComponent->SetTeamColor(CachedUIColor);
	}
	UE_LOG(LogTemp, Log, TEXT("Base Selected: %s (TeamID: %d)"), *GetName(), TeamID);
}

void ANovaBase::OnDeselected()
{
	bIsSelected = false;
	if (SelectionComponent) SelectionComponent->SetSelectionVisible(false);
	UE_LOG(LogTemp, Log, TEXT("Base Deselected: %s"), *GetName());
}

void ANovaBase::IssueCommand(const FCommandData& CommandData)
{
	// 기지에서의 '이동' 명령은 랠리 포인트 설정을 의미함
	if (CommandData.CommandType == ECommandType::Move)
	{
		SetRallyPoint(CommandData.TargetLocation);
		NOVA_LOG(Log, "Base '%s' Rally Point updated to: %s", *GetName(), *RallyPoint.ToString());

		// TODO: 팀원 B가 랠리 포인트 시각화(데칼 등) 로직을 추가할 자리
	}
}

bool ANovaBase::ProduceUnit(int32 SlotIndex)
{
	if (UNovaUnitFactory* Factory = GetWorld()->GetSubsystem<UNovaUnitFactory>())
	{
		// 인터페이스를 통한 호출이므로 실제 구현은 팩토리에 위임
		return Factory->RequestSpawnUnitFromDeck(SlotIndex, this, RallyPoint);
	}

	return false;
}

bool ANovaBase::CanProduceUnit(int32 SlotIndex) const
{
	UNovaUnitFactory* Factory = GetWorld()->GetSubsystem<UNovaUnitFactory>();
	ANovaGameMode* GM = Cast<ANovaGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!Factory || !GM) return false;

	// 1. 덱 정보 가져오기 (TeamID 기준)
	FNovaDeckInfo CurrentDeck = GM->GetPlayerDeck(TeamID);
	if (!CurrentDeck.Units.IsValidIndex(SlotIndex)) return false;

	const FNovaUnitAssemblyData& TargetData = CurrentDeck.Units[SlotIndex];

	// 2. 와트 비용 계산
	float ProductionCost = Factory->CalculateTotalWattCost(TargetData);

	// 3. 플레이어 리소스 컴포넌트 확인 (해당 팀의 PlayerState 찾기)
	ANovaPlayerState* PS = nullptr;
	UWorld* World = GetWorld();
	if (World)
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PC = Iterator->Get();
			if (PC)
			{
				ANovaPlayerState* TempPS = PC->GetPlayerState<ANovaPlayerState>();
				if (TempPS)
				{
					// INovaTeamInterface를 통해 팀 ID를 확인합니다.
					INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(TempPS);
					if (TeamInterface && TeamInterface->GetTeamID() == TeamID)
					{
						PS = TempPS;
						break;
					}
				}
			}
		}
	}

	if (!PS) return false;

	UNovaResourceComponent* ResourceComp = PS->FindComponentByClass<UNovaResourceComponent>();
	if (!ResourceComp) return false;

	// 4. 자원 및 인구수 여유 확인 (Watt 비용만 체크)
	return ResourceComp->CanAfford(ProductionCost, 0.0f) && ResourceComp->CanSpawnUnit(ProductionCost);
}

FNovaDeckInfo ANovaBase::GetProductionDeckInfo() const
{
	if (ANovaGameMode* GM = Cast<ANovaGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		return GM->GetPlayerDeck(TeamID);
	}

	return FNovaDeckInfo();
}

void ANovaBase::DestroyBase()
{
	// 기지 파괴 시 시각적/게임 로직 처리
	UE_LOG(LogTemp, Error, TEXT("Base Destroyed: %s (TeamID: %d)"), *GetName(), TeamID);

	// 자원 재생 중단 (해당 팀의 PlayerState를 찾아 ResourceComponent 접근)
	UWorld* World = GetWorld();
	if (World)
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			if (APlayerController* PC = Iterator->Get())
			{
				if (ANovaPlayerState* PS = PC->GetPlayerState<ANovaPlayerState>())
				{
					// INovaTeamInterface를 통해 팀 ID를 확인합니다.
					INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(PS);
					if (TeamInterface && TeamInterface->GetTeamID() == TeamID)
					{
						if (UNovaResourceComponent* ResourceComp = PS->FindComponentByClass<UNovaResourceComponent>())
						{
							ResourceComp->StopResourceRegen();
						}
					}
				}
			}
		}
	}

	// PlayerState에서 기지 참조 해제
	if (APlayerController* PC = GetNetOwningPlayer() ? GetNetOwningPlayer()->GetPlayerController(GetWorld()) : nullptr)
	{
		if (ANovaPlayerState* PS = PC->GetPlayerState<ANovaPlayerState>())
		{
			// 현재 자신이 등록된 기지인 경우에만 해제
			if (PS->GetPlayerBase() == this)
			{
				PS->SetPlayerBase(nullptr);
			}
		}
	}

	// GameMode에 기지 파괴 이벤트 전달
	if (ANovaGameMode* GM = GetWorld()->GetAuthGameMode<ANovaGameMode>())
	{
		GM->OnBaseDestroyed(this);
	}

	// 체력바 컴포넌트 숨기기
	if (HealthBarComponent)
	{
		HealthBarComponent->SetVisibility(false);
		// 컴포넌트 비활성화
		HealthBarComponent->Deactivate();
	}

	Destroy();
}

void ANovaBase::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.0f)
	{
		DestroyBase();
	}

	// 실시간 업데이트 호출
	UpdateHealthBar();
}

void ANovaBase::UpdateHealthBar()
{
	if (!HealthBarComponent || !AttributeSet) return;

	HealthBarComponent->UpdateHealthBar(AttributeSet->GetHealth(),
	                                    AttributeSet->GetMaxHealth(),
	                                    CachedUIColor);
}

void ANovaBase::InitializeUIColors()
{
	int32 LocalPlayerTeamID = -1;
	if (auto* PC = GetWorld()->GetFirstPlayerController())
	{
		if (auto* PS = PC->GetPlayerState<ANovaPlayerState>())
		{
			LocalPlayerTeamID = PS->GetTeamID();
		}
	}

	// 색상 결정 및 캐싱
	CachedUIColor = FLinearColor::Red; // 적군
	if (TeamID == LocalPlayerTeamID) CachedUIColor = FLinearColor::Green; // 아군
	else if (TeamID == NovaTeam::None || LocalPlayerTeamID == -1) CachedUIColor = FLinearColor::Yellow; // 중립
}

void ANovaBase::SetFogVisibility(bool bVisible)
{
	// 상태가 같으면 return
	if (bIsVisibleByFog == bVisible) return;
	bIsVisibleByFog = bVisible;

	// 시각적 처리 (건물 메시 숨김)
	SetActorHiddenInGame(!bVisible);

	// 마우스 클릭(Visibility)만 선택적으로 무시
	if (BaseCollision)
	{
		BaseCollision->SetCollisionResponseToChannel(ECC_Visibility, bVisible ? ECR_Block : ECR_Ignore);
	}

	// 체력바 위젯 숨김/표시
	if (HealthBarComponent)
	{
		HealthBarComponent->SetFogVisibility(bVisible);
	}

	// 선택된 상태에서 안개 속으로 사라졌을 때 처리
	if (!bVisible && bIsSelected)
	{
		OnDeselected();

		// 로컬 플레이어 컨트롤러를 찾아 선택 해제 알림
		if (auto* NovaPC = Cast<ANovaPlayerController>(GetWorld()->GetFirstPlayerController()))
		{
			NovaPC->NotifyTargetUnselectable(this);
		}
	}

	// 선택 표시 위젯 강제 끄기
	if (!bVisible && SelectionComponent)
	{
		SelectionComponent->SetSelectionVisible(false);
	}
}
