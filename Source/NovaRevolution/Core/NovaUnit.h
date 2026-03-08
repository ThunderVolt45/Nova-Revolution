// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Core/NovaInterfaces.h"
#include "Core/NovaTypes.h"
#include "Core/NovaAssemblyTypes.h"
#include "BehaviorTree/BlackboardComponent.h"
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
	virtual ENovaSelectableType GetSelectableType() const override { return ENovaSelectableType::Unit; }

	// --- INovaCommandInterface ---
	virtual void IssueCommand(const FCommandData& CommandData) override;

	// --- INovaTeamInterface ---
	virtual int32 GetTeamID() const override { return TeamID; }

	// 사망 처리 함수
	virtual void Die();
	
public:
	/** 팩토리로부터 조립 설계도를 전달받아 멤버 변수 초기화 */
	void SetAssemblyData(const FNovaUnitAssemblyData& Data);
	
	/** 팀 식별자 설정을 위한 세터 */
	void SetTeamID(int32 InTeamID) { TeamID = InTeamID; }

	/** 생성 시 초기 이동 목표(랠리 포인트) 설정 */
	void SetInitialRallyLocation(const FVector& InLocation) { InitialRallyLocation = InLocation; }

	/** 유닛의 이름을 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Nova|Unit")
	FString GetUnitName() const { return UnitName; }

protected:
	virtual void BeginPlay() override;

	// --- 유닛 부품 설정 (에디터/데이터 주입용) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Parts")
	TObjectPtr<class UDataTable> PartDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Parts")
	TSubclassOf<class ANovaPart> LegsPartClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Parts")
	TSubclassOf<class ANovaPart> BodyPartClass;

	// 몸통이 다리(Legs)에 부착될 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Parts")
	FName BodyTargetSocketName = TEXT("Socket_Body");

	// 무기 클래스 (단일 종류)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Parts")
	TSubclassOf<class ANovaPart> WeaponPartClass;

	// 무기가 부착될 몸통(Body)의 소켓 이름들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Parts")
	TArray<FName> WeaponSocketNames;

	// --- 실제 생성된 컴포넌트들 (런타임 관리용) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Unit|Parts|Runtime")
	TObjectPtr<UChildActorComponent> LegsPartComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Unit|Parts|Runtime")
	TObjectPtr<UChildActorComponent> BodyPartComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Unit|Parts|Runtime")
	TArray<TObjectPtr<UChildActorComponent>> WeaponPartComponents;
	
	
	//유닛 몸통 회전 관련 변수 및 함수
#pragma region AI Body Rotation Logic
protected:
	/** 몸통이 타겟을 향해 회전하는 속도 (값이 클수록 빠르게 회전) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Parts")
	float BodyRotationInterpSpeed = 10.0f;
	
private:
	// 캐싱용 변수
	UPROPERTY()
	TObjectPtr<UBlackboardComponent> CachedBlackboard;

	//타겟이 입력될 시 저장
	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;
	
	// 추가: 나중에 해제할 때 사용하기 위해 키 ID를 보관합니다.
	FBlackboard::FKey TargetActorKeyID = FBlackboard::InvalidKey; // FBlackboard::InvalidKey의 기본값 (uint8)

	// 옵저버 핸들 (나중에 등록 해제용)
	FDelegateHandle TargetActorObserverHandle;

protected:
	// AI 컨트롤러의 블랙보드에서 현재 타겟 액터를 읽어오는 함수 -> 옵저버 설정으로 로직변경 
	// AActor* GetTargetFromBlackboard() const;

	// 매 프레임 몸통의 회전값을 계산하고 적용하는 함수 
	void UpdateBodyRotation(float DeltaTime);
	
	// 블랙보드 값이 변경될 때 호출될 콜백 함수
	EBlackboardNotificationResult OnTargetActorChanged(const UBlackboardComponent& Blackboard, FBlackboard::FKey KeyID);

	// 빙의 시 옵저버 등록을 위한 오버라이드
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	//ToDo: Note: 최적화를 위한 로직 고려 필요, 타겟이 없을 시 몸통의 회전이 다시 정면으로 돌아오는 로직 필요
	
#pragma endregion
	
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
	FString UnitName;

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
