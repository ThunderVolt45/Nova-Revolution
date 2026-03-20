// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "Core/NovaInterfaces.h"
#include "Core/NovaTypes.h"
#include "NovaBase.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UNovaSelectionComponent;
class UNovaHealthBarComponent;
class UBoxComponent;
class UWidgetComponent;
class UAbilitySystemComponent;
class UNovaAttributeSet;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaseAttributeChanged, ANovaBase*, Base);

/**
 * 플레이어의 거점 기지 클래스
 */
UCLASS()
class NOVAREVOLUTION_API ANovaBase : public AActor, public IAbilitySystemInterface, public INovaSelectableInterface,
                                     public INovaTeamInterface, public INovaCommandInterface,
                                     public INovaProductionInterface, public INovaHighlightInterface
{
	GENERATED_BODY()

public:
	ANovaBase();

	// --- IAbilitySystemInterface ---
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// --- INovaSelectableInterface ---
	virtual void OnSelected() override;
	virtual void OnDeselected() override;
	virtual bool IsSelectable() const override;
	virtual ENovaSelectableType GetSelectableType() const override { return ENovaSelectableType::Base; }

	// --- INovaCommandInterface ---
	virtual void IssueCommand(const FCommandData& CommandData) override;

	// --- INovaTeamInterface ---
	virtual int32 GetTeamID() const override { return TeamID; }

	/** 팀 ID를 설정합니다. (GameMode에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Team")
	void SetTeamID(int32 NewTeamID);

	// --- INovaProductionInterface ---
	UFUNCTION(BlueprintCallable, Category = "Nova|Production")
	virtual bool ProduceUnit(int32 SlotIndex) override;

	UFUNCTION(BlueprintCallable, Category = "Nova|Production")
	virtual bool CanProduceUnit(int32 SlotIndex) const override;

	UFUNCTION(BlueprintCallable, Category = "Nova|Production")
	virtual struct FNovaDeckInfo GetProductionDeckInfo() const override;

	// 랠리 포인트 설정 및 가져오기
	UFUNCTION(BlueprintCallable, Category = "Nova|Base")
	void SetRallyPoint(const FVector& NewLocation) { RallyPoint = NewLocation; }

	UFUNCTION(BlueprintPure, Category = "Nova|Base")
	FVector GetRallyPoint() const { return RallyPoint; }

	// 기지 파괴 처리
	virtual void DestroyBase();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// --- 컴포넌트 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Base")
	TObjectPtr<UBoxComponent> BaseCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Base")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UNovaAttributeSet> AttributeSet;

	// --- 속성 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Base")
	int32 TeamID = NovaTeam::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Base")
	FVector RallyPoint;

	UPROPERTY(BlueprintReadOnly, Category = "Nova|Base")
	bool bIsSelected = false;

private:
	// 체력 변경 콜백
	void OnHealthChanged(const FOnAttributeChangeData& Data);

protected:
	// --- UI 컴포넌트 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	TObjectPtr<UNovaSelectionComponent> SelectionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	TObjectPtr<UNovaHealthBarComponent> HealthBarComponent;

	// --- UI 업데이트 함수 ---
	// 체력바 갱신 
	void UpdateHealthBar();

	// 캐싱된 UI 색상 (성능 최적화용)
	FLinearColor CachedUIColor = FLinearColor::White;

	// TeamID를 확인하여 UI 색상을 결정하는 함수
	void InitializeUIColors();

	// 현재 안개에 의해 보이는 상태인지 여부
	bool bIsVisibleByFog = true;

public:
	// 안개 가시성 설정 함수
	void SetFogVisibility(bool bVisible);

	bool GetFogVisibility() const { return bIsVisibleByFog; }
	
	// UI에서 바인딩할 수 있도록 선언
	UPROPERTY(BlueprintAssignable, Category="Nova|UI")
	FOnBaseAttributeChanged OnBaseAttributeChanged;

	// UI에서 기지 이름을 표시하기 위한 함수
	UFUNCTION(BlueprintPure, Category = "Nova|UI")
	FString GetBaseName() const { return TEXT("중앙 기지"); }

	// UI에서 속성(체력 등)을 읽어갈 수 있도록 Getter 추가
	UFUNCTION(BlueprintPure, Category = "Nova|UI")
	UNovaAttributeSet* GetAttributeSet() const { return AttributeSet; }

protected:
	// --- UI 포트레이트 관련 컴포넌트 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	TObjectPtr<USceneCaptureComponent2D> PortraitCapture;

	/** 캡처한 이미지를 저장할 렌더 타겟 (유닛과 동일한 에셋을 사용하거나 전용 에셋 사용) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|UI")
	TObjectPtr<UTextureRenderTarget2D> PortraitRenderTarget;

	// 카메라 위치 설정을 위한 변수들 (기지는 유닛보다 크므로 값이 다를 수 있음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	FVector PortraitCaptureLocation = FVector(300.0f, 0.f, 100.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	FRotator PortraitCaptureRotation = FRotator(-30.f, -180.f, 0.f);

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	// float PortraitCaptureFOVAngle = 90.f;

public:
	// 하이라이트 효과 적용
	void SetHighlight(bool bEnable, FLinearColor HighlightColor = FLinearColor::White);

	// --- INovaHighlightInterface 구현 ---
	virtual void SetHighlightStatus(ENovaHighlightPriority Priority, bool bActive,
	                                FLinearColor Color = FLinearColor::White) override;
	virtual void UpdateHighlight() override;

protected:
	// --- 엔진 마우스 이벤트 오버라이드 ---
	virtual void NotifyActorBeginCursorOver() override;
	virtual void NotifyActorEndCursorOver() override;

	/** 에디터에서 할당할 하이라이트 마스터 머티리얼 (M_Highlight) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Base|Effects")
	TObjectPtr<UMaterialInterface> HighlightMasterMaterial;

	/** 런타임에 색상을 바꾸기 위한 동적 인스턴스 */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> HighlightDynamicMaterial;

private:
	// 하이라이트 상태 플래그
	bool bIsHovered = false;
	bool bIsDragHighlighted = false;
	bool bIsSkillHighlighted = false;
	FLinearColor SkillHighlightColor = FLinearColor::White;
	
public:
	/** 특정 팀에게 이 유닛이 보이는지 확인 (비트 연산) */
	UFUNCTION(BlueprintPure, Category = "Nova|Fog")
	bool IsVisibleToTeam(int32 CurrentTeamID) const;

	/** 특정 팀에 대한 가시성 상태를 업데이트 (비트 연산) */
	void SetVisibilityForTeam(int32 CurrentTeamID, bool bVisible);

private:
	/** 팀별 가시성 비트마스크 (0번 비트: 팀 0, 1번 비트: 팀 1 ...) */
	uint32 VisibilityMask = 0;
};
