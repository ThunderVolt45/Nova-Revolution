// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "NovaPartData.h"
#include "Core/NovaInterfaces.h"
#include "Core/NovaTypes.h"
#include "Core/NovaAssemblyTypes.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NovaUnit.generated.h"

class UWidgetComponent;
class UAbilitySystemComponent;
class UNovaAttributeSet;
struct FOnAttributeChangeData;

/**
 * Nova Revolution의 모든 유닛(로봇)의 기본 클래스
 */
UCLASS()
class NOVAREVOLUTION_API ANovaUnit : public ACharacter, public IAbilitySystemInterface, public INovaSelectableInterface,
                                     public INovaCommandInterface, public INovaTeamInterface
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

	/** 유닛의 이동 타입을 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Nova|Unit")
	ENovaMovementType GetMovementType() const { return MovementType; }

	/** 유닛의 무기 공격 가능 타입 (대지/대공/모두) */
	UFUNCTION(BlueprintPure, Category = "Nova|Unit")
	ENovaTargetType GetTargetType() const { return TargetType; }

	/** 타겟이 사거리 내에 있는지 캡슐(원기둥) 방식으로 판정합니다. */
	UFUNCTION(BlueprintPure, Category = "Nova|Unit")
	bool IsTargetInRange(const AActor* Target, float Range) const;

	// 유닛의 생존 여부를 반환합니다.
	UFUNCTION(BlueprintCallable, Category = "Nova|Unit")
	bool IsDead() const { return bIsDead; }

	/** 무기 부품 컴포넌트들을 반환합니다. */
	const TArray<TObjectPtr<UChildActorComponent>>& GetWeaponPartComponents() const { return WeaponPartComponents; }

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
	float BodyRotationInterpSpeed = 40.0f;

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
	// 매 프레임 몸통의 회전값을 계산하고 적용하는 함수 
	void UpdateBodyRotation(float DeltaTime);

	// 매 프레임 무기의 조준 각도를 계산하고 업데이트 명령을 내립니다.
	void UpdateWeaponAiming(float DeltaTime);

	// 블랙보드 값이 변경될 때 호출될 콜백 함수
	EBlackboardNotificationResult OnTargetActorChanged(const UBlackboardComponent& Blackboard, FBlackboard::FKey KeyID);

	// 빙의 시 옵저버 등록을 위한 오버라이드
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

#pragma endregion

	// --- 부품 조립 로직 ---
	// 클래스 설정에 따라 실제 ChildActor를 생성하고 부착
	void ConstructUnitParts();
	void InitializePartAttachments();

	// 부품들의 스탯을 합산하여 유닛의 기본 스탯(AttributeSet) 초기화
	void InitializeAttributesFromParts();

	// 부품들의 어빌리티를 수집하여 유닛에게 부여 (중복 방지)
	void InitializeAbilitiesFromParts();

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

	/** 유닛의 최종 이동 타입 (지상/공중) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Unit")
	ENovaMovementType MovementType = ENovaMovementType::Ground;

	/** 유닛의 무기 공격 가능 타입 (대지/대공/모두) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Unit")
	ENovaTargetType TargetType = ENovaTargetType::All;

protected:
	/** 공중 유닛의 기본 고도 (지면 Z=0 기준) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Air")
	float DefaultAirZ = 800.0f;

	/** 지면으로부터 유지해야 할 최소 안전 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Air")
	float MinSafetyHeight = 300.0f;

	/** 고도 조절 시 보간 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Air")
	float HeightInterpSpeed = 3.0f;

private:
	// 이전 프레임의 Yaw (회전 속도 계산용)
	float LastYaw = 0.0f;

	// 속성 변경 시 호출될 콜백 함수 (UI 업데이트 등)
	void OnHealthChanged(const FOnAttributeChangeData& Data);

protected:
	// 유닛 선택 표시용 WidgetComponent
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	TObjectPtr<UWidgetComponent> SelectionWidget;

	// 공중 유닛의 위젯 위치 조절용 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	float AirWidgetHeightOffset = 50.f;

	// 항상 바닥에 투사되는 선택 원 (공중 유닛용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	TObjectPtr<UWidgetComponent> GroundSelectionWidget;

	// 공중 유닛의 지면 투영 원(Ground Circle)의 고정 지름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	float GroundSelectionCircleSize = 20.0f;

	// 유닛과 바닥을 잇는 수직 고도 안내선
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	TObjectPtr<UStaticMeshComponent> HeightIndicatorLine;

	// 상단 체력바 위젯 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	TObjectPtr<UWidgetComponent> HealthBarWidget;

	// 체력바의 세로 길이
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	float HealthBarHeight = 15.0f;

	// 체력바의 최소 가로 길이
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	float MinHealthBarWidth = 1.0f;

	// 체력바의 최대 가로 길이
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	float MaxHealthBarWidth = 200.0f;

	// 로그 스케일 계산 시 사용할 비중 수치 (값이 클수록 길이가 더 많이 늘어남)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	float HealthBarLogScaleFactor = 40.0f;

	// 플레이어 옵션에 의한 체력바 표시 여부 (기본값: true)
	bool bHealthBarOptionEnabled = true;
	
	// TeamID별 색 변경 함수
	void UpdateSelectionCircleColor();

	// 캡슐 크기에 맞춰 선택 위젯의 지름을 업데이트 하는 함수
	void UpdateSelectionCircleTransform();

	// 지면 위젯과 안내선의 위치 및 길이를 업데이트.
	void UpdateGroundCircleAndLine();

	// 체력바의 퍼센트와 색상을 업데이트
	void UpdateHealthBar();

	// 체력바의 3D 앵커 위치를 캡슐 크기에 맞춰 업데이트합니다. (Screen Space 왜곡 및 확대 대응)
	void UpdateHealthBarTransform();

	// 최대 체력에 따른 체력바 가로 길이를 업데이트
	void UpdateHealthBarLength();
	
	// 캐싱된 UI 색상 (성능 최적화용)
	FLinearColor CachedUIColor = FLinearColor::White;
	
	// TeamID를 확인하여 UI 색상을 결정하는 함수
	void InitializeUIColors();
	
	// --- 안개에 의한 가시성 설정 변수 및 함수 ---
	// 안개에 의해 보이고 있는지 여부
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Unit")
	bool bIsVisibleByFog = true;

public:
	// 안개 가시성 설정 함수
	void SetFogVisibility(bool bVisible);
	
	// 체력바 표시 옵션 설정 함수
	void SetHealthBarVisibilityOption(bool bEnable);
};
