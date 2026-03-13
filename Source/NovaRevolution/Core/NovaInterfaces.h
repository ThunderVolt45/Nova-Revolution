// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "Core/NovaTypes.h"
#include "NovaInterfaces.generated.h"

// --- 팀 시스템 인터페이스 ---
UINTERFACE(MinimalAPI)
class UNovaTeamInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 팀 소속을 가지는 모든 액터들이 구현해야 하는 인터페이스
 */
class NOVAREVOLUTION_API INovaTeamInterface
{
	GENERATED_BODY()

public:
	// 현재 소속된 팀 ID를 반환
	virtual int32 GetTeamID() const = 0;

	// 아군 여부 확인
	virtual bool IsFriendly(int32 OtherTeamID) const { return GetTeamID() == OtherTeamID; }
};

// --- 유닛 선택 시스템 인터페이스 ---
UINTERFACE(MinimalAPI)
class UNovaSelectableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 선택 가능한 모든 액터들이 구현해야 하는 인터페이스
 */
class NOVAREVOLUTION_API INovaSelectableInterface
{
	GENERATED_BODY()

public:
	// 선택 시 호출 (데칼 켜기 등)
	virtual void OnSelected() = 0;

	// 선택 해제 시 호출 (데칼 끄기 등)
	virtual void OnDeselected() = 0;

	// 선택 가능한 타입 반환 (기본값 None)
	virtual ENovaSelectableType GetSelectableType() const { return ENovaSelectableType::None; }

	// 선택 가능한 상태인지 확인
	virtual bool IsSelectable() const { return true; }
};

// --- 자원 시스템 인터페이스 ---

/** 자원 변경 알림을 위한 델리게이트 (BlueprintAssignable 용도로 사용 가능) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNovaOnResourceChangedSignature, float, NewValue, float, MaxValue);

UINTERFACE(MinimalAPI, BlueprintType, NotBlueprintable)
class UNovaResourceInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 자원 정보(Watt, SP, Population)를 제공하는 인터페이스
 * HUD(UI)에서 플레이어의 현재 자원 상태를 파악할 때 사용합니다.
 */
class NOVAREVOLUTION_API INovaResourceInterface
{
	GENERATED_BODY()

public:
	/** 현재 와트(Watt) 정보를 가져옵니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Resource")
	virtual float GetCurrentWatt() const = 0;

	/** 최대 와트(Max Watt) 제한을 가져옵니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Resource")
	virtual float GetMaxWatt() const = 0;

	/** 현재 스킬 포인트(SP) 정보를 가져옵니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Resource")
	virtual float GetCurrentSP() const = 0;

	/** 최대 스킬 포인트(Max SP) 제한을 가져옵니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Resource")
	virtual float GetMaxSP() const = 0;

	/** 현재 인구수(Population) 정보를 가져옵니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Resource")
	virtual float GetCurrentPopulation() const = 0;

	/** 최대 인구수(Max Population) 제한을 가져옵니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Resource")
	virtual float GetMaxPopulation() const = 0;

	/** 현재 필드 위 유닛들의 와트 총합을 가져옵니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Resource")
	virtual float GetTotalUnitWatt() const = 0;

	/** 필드 위 유닛들의 와트 총합 제한을 가져옵니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Resource")
	virtual float GetMaxUnitWatt() const = 0;

	/** 플레이어 와트 생산 레벨 정보를 가져옵니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Resource")
	virtual float GetWattLevel() const = 0;

	/** 플레이어 SP 생산 레벨 정보를 가져옵니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Resource")
	virtual float GetSPLevel() const = 0;

	/** 자원 변경 델리게이트 접근자 */
	virtual FNovaOnResourceChangedSignature& GetOnWattChangedDelegate() = 0;
	virtual FNovaOnResourceChangedSignature& GetOnSPChangedDelegate() = 0;
	virtual FNovaOnResourceChangedSignature& GetOnPopulationChangedDelegate() = 0;
	virtual FNovaOnResourceChangedSignature& GetOnTotalUnitWattChangedDelegate() = 0;
	virtual FNovaOnResourceChangedSignature& GetOnWattLevelChangedDelegate() = 0;
	virtual FNovaOnResourceChangedSignature& GetOnSPLevelChangedDelegate() = 0;
};

// --- 명령 전달 시스템 인터페이스 ---
UINTERFACE(MinimalAPI)
class UNovaCommandInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 명령을 수행할 수 있는 모든 액터(또는 AIController)들이 구현해야 하는 인터페이스
 */
class NOVAREVOLUTION_API INovaCommandInterface
{
	GENERATED_BODY()

public:
	// 명령 데이터 패키지를 전달받아 처리
	virtual void IssueCommand(const FCommandData& CommandData) = 0;
};

// --- 생산 시스템 인터페이스 ---
UINTERFACE(MinimalAPI, BlueprintType, NotBlueprintable)
class UNovaProductionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 유닛을 생산할 수 있는 건물(기지 등)이 구현해야 하는 인터페이스
 */
class NOVAREVOLUTION_API INovaProductionInterface
{
	GENERATED_BODY()

public:
	/** 특정 덱 슬롯의 유닛 생산을 요청합니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Production")
	virtual bool ProduceUnit(int32 SlotIndex) = 0;

	/** 현재 해당 슬롯의 유닛 생산이 가능한지 확인합니다. (자원, 인구수 등) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Production")
	virtual bool CanProduceUnit(int32 SlotIndex) const = 0;

	/** 현재 생산 주체가 사용 중인 덱 정보를 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Production")
	virtual struct FNovaDeckInfo GetProductionDeckInfo() const = 0;
};


/**
 * 사령관 스킬 시스템 인터페이스 : 단축키 입력과 UI상 스킬버튼 입력에 대해 동일하게 발동 <CommandInterface와 헷갈리지 않게 주의>
 */
UINTERFACE(MinimalAPI, BlueprintType)
class UNovaSkillInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 사령관(PlayerState)이 구현하여 UI나 입력으로부터 스킬 실행 요청을 받는 인터페이스 <CommandInterface와 헷갈리지 않게 주의>
 */
class NOVAREVOLUTION_API INovaSkillInterface
{
	GENERATED_BODY()

public:
	/**
	 * 특정 태그에 해당하는 사령관 스킬(어빌리티) 실행을 요청합니다.
	 * BlueprintNativeEvent를 사용하여 C++과 블루프린트 모두에서 구현/호출 가능하게 합니다.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Nova|Skill")
	void ActivateSkillAbility(FGameplayTag AbilityTag);
};

// --- 오브젝트 풀링 인터페이스 ---
UINTERFACE(MinimalAPI, BlueprintType)
class UNovaObjectPoolable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 오브젝트 풀에서 관리될 수 있는 액터들이 구현해야 하는 인터페이스
 */
class NOVAREVOLUTION_API INovaObjectPoolable
{
	GENERATED_BODY()

public:
	/** 풀에서 꺼내질 때 호출됩니다. (위치/회전 이동 직후, 활성화 직전/직후) 초기화 작업을 수행합니다. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Nova|ObjectPool")
	void OnSpawnFromPool();

	/** 풀로 돌아갈 때 호출됩니다. 상태를 리셋하거나 비활성화합니다. (풀에 들어가기 직전 호출됨) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Nova|ObjectPool")
	void OnReturnToPool();
};