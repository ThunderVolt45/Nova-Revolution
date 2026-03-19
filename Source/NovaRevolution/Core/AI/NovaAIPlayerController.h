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
 * AI가 빌드 단계에서 취할 수 있는 행동 타입
 */
UENUM(BlueprintType)
enum class ENovaAIBuildStepType : uint8
{
    ProduceUnit,    // 유닛 생산 (TargetCount 도달 시 다음 스텝)
    UseSkill,       // 스킬 발동 (발동 후 다음 스텝)
    CommandAttack,  // 공격 개시 (모인 웨이브를 적진으로 진군시킴)
    Wait            // 대기 시간 (TargetCount 초만큼 대기)
};

/**
 * 특정 생산 단계나 행동을 정의
 */
USTRUCT(BlueprintType)
struct FNovaAIBuildStep
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|BuildOrder")
    ENovaAIBuildStepType ActionType;

    // 생산할 유닛 슬롯(0~9) 또는 시전할 스킬 슬롯 번호
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|BuildOrder")
    int32 TargetSlot;

    // 유닛 생산 시 목표 수량, Wait 시 대기 초 단위 등
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|BuildOrder")
    int32 TargetCount;
};

/**
 * 빌드 오더 프로필
 */
USTRUCT(BlueprintType)
struct FNovaAIBuildOrder
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|BuildOrder")
    FString ProfileName;

    // 1페이즈: 초반 1회만 수행되는 오프닝 빌드
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|BuildOrder")
    TArray<FNovaAIBuildStep> OpeningSteps; 

    // 2페이즈: 오프닝 이후 무한 반복되는 후반 운영 빌드 루프
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|BuildOrder")
    TArray<FNovaAIBuildStep> MacroLoopSteps; 
};

/**
 * 웨이브 지시 상태 
 */
UENUM(BlueprintType)
enum class ENovaAIWaveState : uint8
{
    Gathering, // 기지 주변에 모여 방어
    Attacking  // 목표를 향해 진격
};

/**
 * 유닛 웨이브 구조체
 */
USTRUCT(BlueprintType)
struct FNovaAIWave
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "AI|Wave")
    TArray<TWeakObjectPtr<class ANovaUnit>> AssignedUnits;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Wave")
    ENovaAIWaveState CurrentState = ENovaAIWaveState::Gathering;

    // 공격 개시 시점의 유닛 수 (손실율 계산용)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Wave")
    int32 InitialUnitCount = 0;
};

/**
 * 데이터 테이블에서 빌드 오더를 관리하기 위한 행 구조체
 */
USTRUCT(BlueprintType)
struct FNovaAIBuildOrderRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|BuildOrder")
    FNovaAIBuildOrder BuildOrder;
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

	/** 현재 선택된 빌드 오더를 반환합니다. */
	const FNovaAIBuildOrder& GetSelectedBuildOrder() const { return SelectedBuildOrder; }

	/** 현재 속한 빌드 페이즈 스텝을 가져옵니다. (유효하지 않으면 nullptr) */
	const FNovaAIBuildStep* GetCurrentBuildStep() const;

	/** 다음 빌드 스텝으로 진행합니다. 오프닝이 끝나면 매크로 루프로 자동 전환됩니다. */
	void AdvanceBuildStep();

	/** 긴급 방어 상태를 제어합니다. */
	void SetEmergencyDefenseActive(bool bActive);
	bool IsEmergencyDefenseActive() const { return bIsEmergencyDefenseActive; }

	/** 현재 오프닝 페이즈인지, 매크로 루프 페이즈인지 확인합니다. */
	bool IsMacroLooping() const { return bIsMacroLooping; }

	/** 현재 빌드 단계가 시작된 시간을 반환합니다. */
	float GetLastStepStartTime() const { return LastStepStartTime; }

	/** 팀 내 모든 유닛을 찾아 명령을 전달합니다. */
	void IssueCommandToAllUnits(const struct FCommandData& CommandData);

	/** 현재 개더링 중인 웨이브의 모든 유닛을 'Attacking' 웨이브로 전환하고 명령을 하달합니다. */
	void LaunchGatheringWaves(const struct FCommandData& CommandData);

	/** 새로 생성된 아군 유닛을 현재 개더링 중인 웨이브에 소속시킵니다. */
	void AddUnitToCurrentWave(class ANovaUnit* NewUnit);

	/** 기지 파괴 등 상황에서 AI 로직을 완전히 중지합니다. */
	void StopAI();

	/** 특정 생산 슬롯 번호에서 생성된 아군 유닛의 총 생존 수를 반환합니다. */
	int32 CountUnitsOfSlot(int32 TargetSlot) const;

protected:
	virtual void BeginPlay() override;

	/** 사용 가능한 빌드 오더 중 하나를 무작위로 선택합니다. */
	void RandomizeBuildOrder();

	
public: 
	/** AI 관리 기지 전용 팀 ID (PlayerState가 없거나 초기화 중일 때를 대비한 캐시) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|AI")
	int32 CachedTeamID = -1;
	
	/** 블랙보드 키 이름 정의 (Behavior Tree 서비스/태스크 등에서 참조) */
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

	/** AI가 참고할 빌드 오더 데이터 테이블 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDataTable> BuildOrderDataTable;

	/** 현재 게임에서 선택된 빌드 오더 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|AI", meta = (AllowPrivateAccess = "true"))
	FNovaAIBuildOrder SelectedBuildOrder;

	/** 현재 속해 있는 웨이브 배열 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|AI", meta = (AllowPrivateAccess = "true"))
	TArray<FNovaAIWave> ActiveWaves;

	/** 현재 진행 중인 빌드 스텝 인덱스 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|AI", meta = (AllowPrivateAccess = "true"))
	int32 CurrentBuildStepIndex = 0;

	/** 현재 빌드 스텝이 시작된 게임 시간 (Seconds) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|AI", meta = (AllowPrivateAccess = "true"))
	float LastStepStartTime = 0.0f;

	/** 현재 매크로 루프(2페이즈)에 돌입했는지 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|AI", meta = (AllowPrivateAccess = "true"))
	bool bIsMacroLooping = false;

	/** 긴급 방어 모드 동작 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|AI", meta = (AllowPrivateAccess = "true"))
	bool bIsEmergencyDefenseActive = false;

	/** 관리 중인 기지 (약참조) */
	UPROPERTY()
	TWeakObjectPtr<ANovaBase> ManagedBase;
};
