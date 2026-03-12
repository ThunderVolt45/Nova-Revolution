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
#include "Kismet/GameplayStatics.h"

ANovaBase::ANovaBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// 메시 컴포넌트 생성 및 루트 설정
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;

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

	// 랠리 포인트 초기값 설정 (기지 앞쪽 500 유닛 지점 예시)
	RallyPoint = GetActorLocation() + GetActorForwardVector() * 500.0f;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		
		AttributeSet->SetMaxHealth(20000.f);
		AttributeSet->SetHealth(20000.f);

		// 체력 변경 콜백 등록
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
			.AddUObject(this, &ANovaBase::OnHealthChanged);
	}
}

void ANovaBase::SetTeamID(int32 NewTeamID)
{
	TeamID = NewTeamID;

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
	UE_LOG(LogTemp, Log, TEXT("Base Selected: %s (TeamID: %d)"), *GetName(), TeamID);
}

void ANovaBase::OnDeselected()
{
	bIsSelected = false;
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

	Destroy();
}

void ANovaBase::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.0f)
	{
		DestroyBase();
	}
}
