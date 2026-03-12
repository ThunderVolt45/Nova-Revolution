// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "Core/NovaInterfaces.h"
#include "Core/NovaTypes.h"
#include "NovaBase.generated.h"

class UBoxComponent;
class UWidgetComponent;
class UAbilitySystemComponent;
class UNovaAttributeSet;
struct FOnAttributeChangeData;

/**
 * 플레이어의 거점 기지 클래스
 */
UCLASS()
class NOVAREVOLUTION_API ANovaBase : public AActor, public IAbilitySystemInterface, public INovaSelectableInterface,
                                     public INovaTeamInterface, public INovaCommandInterface,
                                     public INovaProductionInterface
{
	GENERATED_BODY()

public:
	ANovaBase();

	// --- IAbilitySystemInterface ---
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// --- INovaSelectableInterface ---
	virtual void OnSelected() override;
	virtual void OnDeselected() override;
	virtual bool IsSelectable() const override { return true; }
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
	TObjectPtr<UWidgetComponent> SelectionWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|UI")
	TObjectPtr<UWidgetComponent> HealthBarWidget;

	// --- 로그 스케일 관련 변수 ---
	// 체력바의 세로 길이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	float HealthBarHeight = 15.0f;

	// 체력바의 최소 가로 길이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	float MinHealthBarWidth = 150.0f;

	// 체력바의 최대 가로 길이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	float MaxHealthBarWidth = 350.0f;

	// 로그 스케일 계산 시 사용할 비중 수치 (값이 클수록 길이가 더 많이 늘어남)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	float HealthBarLogScaleFactor = 80.0f;

	// --- UI 업데이트 함수 ---
	// 캡슐 크기에 맞춰 선택 위젯의 지름을 업데이트 하는 함수
	void UpdateSelectionCircleTransform();

	// TeamID별 색 변경 함수
	void UpdateSelectionCircleColor();

	// 체력바 갱신 
	void UpdateHealthBar();

	// 체력바의 가로 길이를 기지 체력에 맞춰 로그 스케일로 업데이트
	void UpdateHealthBarLength();

	// 캐싱된 UI 색상 (성능 최적화용)
	FLinearColor CachedUIColor = FLinearColor::White;

	// TeamID를 확인하여 UI 색상을 결정하는 함수
	void InitializeUIColors();
	
	// 현재 안개에 의해 보이는 상태인지 여부
	bool bIsVisibleByFog = true;
	
	// 플레이어 옵션에 의한 체력바 표시 여부 (기본값 : true)
	bool bHealthBarOptionEnabled = true;
	
public:
	// 안개 가시성 설정 함수
	void SetFogVisibility(bool bVisible);
	
	// 체력바 표시 옵션 설정 함수
	UFUNCTION()
	void SetHealthBarVisibilityOption(bool bEnable);
};
