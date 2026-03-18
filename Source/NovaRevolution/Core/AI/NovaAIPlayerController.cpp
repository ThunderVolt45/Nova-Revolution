// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaAIPlayerController.h"

#include "NovaRevolution.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Core/NovaBase.h"
#include "Core/NovaPlayerState.h"
#include "Core/NovaLog.h"
#include "Kismet/GameplayStatics.h"

const FName ANovaAIPlayerController::WattKey = TEXT("CurrentWatt");
const FName ANovaAIPlayerController::PopulationKey = TEXT("CurrentPopulation");
const FName ANovaAIPlayerController::MaxPopulationKey = TEXT("MaxPopulation");
const FName ANovaAIPlayerController::RecommendedUnitSlotKey = TEXT("RecommendedUnitSlot");
const FName ANovaAIPlayerController::RecommendedSkillSlotKey = TEXT("RecommendedSkillSlot");
const FName ANovaAIPlayerController::EnemyBaseLocationKey = TEXT("EnemyBaseLocation");

ANovaAIPlayerController::ANovaAIPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
}

void ANovaAIPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 1. 전략 프로필 선정
	RandomizeComposition();

	// 2. 행동 트리 실행
	if (StrategyBTAsset)
	{
		RunBehaviorTree(StrategyBTAsset);
	}
	else
	{
		NOVA_LOG(Warning, "StrategyBTAsset is NOT assigned in %s!", *GetName());
	}
}

void ANovaAIPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANovaAIPlayerController::RandomizeComposition()
{
	if (AvailableCompositions.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, AvailableCompositions.Num() - 1);
		SelectedComposition = AvailableCompositions[RandomIndex];
		
		NOVA_LOG(Log, "AI Player selected strategy composition: %s", *SelectedComposition.ProfileName);
	}
	else
	{
		NOVA_LOG(Warning, "No AvailableCompositions set for AI Player!");
		
		// 기본값 설정 (슬롯 0번에 몰아주기 등)
		SelectedComposition.ProfileName = TEXT("Default");
		SelectedComposition.SlotTargetWeights.Add(0, 1.0f);
	}
}

void ANovaAIPlayerController::SetManagedBase(ANovaBase* InBase)
{
	ManagedBase = InBase;
	
	if (ManagedBase.IsValid())
	{
		// 적 기지 위치 캐싱
		TArray<AActor*> FoundBases;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaBase::StaticClass(), FoundBases);

		for (AActor* Actor : FoundBases)
		{
			ANovaBase* OtherBase = Cast<ANovaBase>(Actor);
			if (OtherBase && OtherBase->GetTeamID() != GetTeamID())
			{
				BlackboardComponent->SetValueAsVector(EnemyBaseLocationKey, OtherBase->GetActorLocation());
				break;
			}
		}
	}
}

int32 ANovaAIPlayerController::GetTeamID() const
{
	if (ANovaPlayerState* PS = GetPlayerState<ANovaPlayerState>())
	{
		return PS->GetTeamID();
	}
	return -1;
}
