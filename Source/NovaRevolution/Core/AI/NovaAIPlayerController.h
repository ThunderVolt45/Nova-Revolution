// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NovaAIPlayerController.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;
class ANovaBase;
class ANovaPlayerState;

/**
 * AI가 지향하는 유닛 구성 프로필
 */
USTRUCT(BlueprintType)
struct FNovaAIUnitComposition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FString ProfileName;

	// 각 슬롯(0~9) 별 목표 생산 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TMap<int32, int32> SlotTargetCounts;
};

/**
 * Nova Revolution의 AI 플레이어(지휘관) 전용 컨트롤러
 * 개별 유닛이 아닌, 팀 전체의 전략(생산, 스킬 사용)을 담당합니다.
 */
UCLASS()
class NOVAREVOLUTION_API ANovaAIPlayerController : public AAIController
{
	GENERATED_BODY()

public:
	ANovaAIPlayerController();

	virtual void Tick(float DeltaTime) override;

	/** AI가 관리하는 기지를 설정합니다. */
	void SetManagedBase(ANovaBase* InBase);

	/** 현재 관리 중인 기지를 반환합니다. */
	ANovaBase* GetManagedBase() const { return ManagedBase.Get(); }

	/** AI의 팀 ID를 반환합니다. */
	int32 GetTeamID() const;

	/** 현재 선택된 유닛 구성 프로필을 반환합니다. */
	const FNovaAIUnitComposition& GetSelectedComposition() const { return SelectedComposition; }

	/** 팀 내 모든 유닛을 찾아 명령을 전달합니다. */
	void IssueCommandToAllUnits(const struct FCommandData& CommandData);

protected:
	virtual void BeginPlay() override;

	/** 사용 가능한 프로필 중 하나를 무작위로 선택합니다. */
	void RandomizeComposition();

	/** 블랙보드 키 이름 정의 */
	static const FName WattKey;
	static const FName PopulationKey;
	static const FName MaxPopulationKey;
	static const FName RecommendedUnitSlotKey;
	static const FName RecommendedSkillSlotKey;
	static const FName EnemyBaseLocationKey;
	static const FName MyBaseLocationKey;

private:
	/** AI 전략용 Behavior Tree 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent;

	/** AI 전략용 Blackboard 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBlackboardComponent> BlackboardComponent;

	/** 에디터에서 할당할 전략 행동 트리 에셋 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UBehaviorTree> StrategyBTAsset;

	/** AI가 선택할 수 있는 유닛 구성 프로필 리스트 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|AI", meta = (AllowPrivateAccess = "true"))
	TArray<FNovaAIUnitComposition> AvailableCompositions;

	/** 현재 게임에서 선택된 프로필 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|AI", meta = (AllowPrivateAccess = "true"))
	FNovaAIUnitComposition SelectedComposition;

	/** 관리 중인 기지 (약참조) */
	UPROPERTY()
	TWeakObjectPtr<ANovaBase> ManagedBase;
};
