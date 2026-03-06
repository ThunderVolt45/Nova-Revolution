// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "Core/NovaInterfaces.h"
#include "Core/NovaTypes.h"
#include "NovaBase.generated.h"

class UAbilitySystemComponent;
class UNovaAttributeSet;
struct FOnAttributeChangeData;

/**
 * 플레이어의 거점 기지 클래스
 */
UCLASS()
class NOVAREVOLUTION_API ANovaBase : public AActor, public IAbilitySystemInterface, public INovaSelectableInterface, public INovaTeamInterface, public INovaCommandInterface, public INovaProductionInterface
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

	// --- 컴포넌트 ---
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
};
