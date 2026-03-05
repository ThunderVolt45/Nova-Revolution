// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Core/NovaInterfaces.h"
#include "Core/NovaTypes.h"
#include "Core/NovaAssemblyTypes.h"
#include "NovaUnit.generated.h"

class UAbilitySystemComponent;
class UNovaAttributeSet;
struct FOnAttributeChangeData;

/**
 * Nova Revolution의 모든 유닛(로봇)의 기본 클래스
 */
UCLASS()
class NOVAREVOLUTION_API ANovaUnit : public ACharacter, public IAbilitySystemInterface, public INovaSelectableInterface, public INovaCommandInterface, public INovaTeamInterface
{
	GENERATED_BODY()

public:
	ANovaUnit();

	virtual void Tick(float DeltaTime) override;

	// 에디터 프리뷰 및 런타임 조립을 위한 건설 스크립트
	virtual void OnConstruction(const FTransform& Transform) override;

	// --- IAbilitySystemInterface ---
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// --- INovaSelectableInterface ---
	virtual void OnSelected() override;
	virtual void OnDeselected() override;
	virtual bool IsSelectable() const override;

	// --- INovaCommandInterface ---
	virtual void IssueCommand(const FCommandData& CommandData) override;

	/** 유닛을 특정 위치로 이동시키는 함수 */
	virtual bool MoveToLocation(const FVector& TargetLocation, float AcceptanceRadius = 5.0f);

	// --- INovaTeamInterface ---
	virtual int32 GetTeamID() const override { return TeamID; }

	// 사망 처리 함수
	virtual void Die();
	
public:
	/** 팩토리로부터 조립 설계도를 전달받아 멤버 변수 초기화 */
	void SetAssemblyData(const FNovaUnitAssemblyData& Data)
	{
		LegsPartClass = Data.LegsClass;
		BodyPartClass = Data.BodyClass;
		WeaponSlotConfigs = Data.WeaponSlots;
	}
	
	/** 팀 식별자 설정을 위한 세터 */
	void SetTeamID(int32 InTeamID) { TeamID = InTeamID; }

	/** 생성 시 초기 이동 목표(랠리 포인트) 설정 */
	void SetInitialRallyLocation(const FVector& InLocation) { InitialRallyLocation = InLocation; }

protected:
	virtual void BeginPlay() override;

	// --- 유닛 부품 설정 (에디터/데이터 주입용) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Parts")
	TSubclassOf<class ANovaPart> LegsPartClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Parts")
	TSubclassOf<class ANovaPart> BodyPartClass;

	// 몸통이 다리(Legs)에 부착될 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Parts")
	FName BodyTargetSocketName = TEXT("Socket_Body");

	// 무기 설정 리스트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Parts")
	TArray<FNovaWeaponPartSlot> WeaponSlotConfigs;

	// --- 실제 생성된 컴포넌트들 (런타임 관리용) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Unit|Parts|Runtime")
	TObjectPtr<UChildActorComponent> LegsPartComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Unit|Parts|Runtime")
	TObjectPtr<UChildActorComponent> BodyPartComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Unit|Parts|Runtime")
	TArray<TObjectPtr<UChildActorComponent>> WeaponPartComponents;

	// --- 부품 조립 로직 ---
	// 클래스 설정에 따라 실제 ChildActor를 생성하고 부착
	void ConstructUnitParts();
	void InitializePartAttachments();

	// 부품들의 스탯을 합산하여 유닛의 기본 스탯(AttributeSet) 초기화
	void InitializeAttributesFromParts();

	// --- GAS 구성 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UNovaAttributeSet> AttributeSet;

	// --- 유닛 기본 정보 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Unit")
	int32 TeamID = NovaTeam::None;

	// 선택 여부 (팀원 B가 하이라이트 로직 구현 시 사용)
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Unit")
	bool bIsSelected = false;

	// 사망 여부
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Unit")
	bool bIsDead = false;

	// 생성 시 초기 목표 지점 (랠리 포인트)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Unit")
	FVector InitialRallyLocation = FVector::ZeroVector;

private:
	// 이전 프레임의 Yaw (회전 속도 계산용)
	float LastYaw = 0.0f;

	// 속성 변경 시 호출될 콜백 함수 (UI 업데이트 등)
	void OnHealthChanged(const FOnAttributeChangeData& Data);
};
