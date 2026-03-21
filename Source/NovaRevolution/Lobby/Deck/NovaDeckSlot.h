// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NovaDeckSlot.generated.h"

/**
 * ANovaDeckSlot
 * 로비 레벨(격납고)에 배치되어 개별 유닛 덱을 대표하는 액터입니다.
 * 마우스 클릭 시 해당 슬롯을 선택하거나, 마우스 오버 시 시각적 하이라이트 효과를 제공합니다.
 */
UCLASS()
class NOVAREVOLUTION_API ANovaDeckSlot : public AActor
{
	GENERATED_BODY()

public:
	ANovaDeckSlot();

protected:
	virtual void BeginPlay() override;

public:
	// --- 마우스 상호작용 (엔진 기본 이벤트 오버라이드) ---
	// 플레이어 컨트롤러의 'Enable Click/Mouse Over Events'가 켜져 있어야 동작합니다.
	virtual void NotifyActorBeginCursorOver() override;
	virtual void NotifyActorEndCursorOver() override;
	virtual void NotifyActorOnClicked(FKey ButtonPressed = EKeys::LeftMouseButton) override;

	/** 슬롯의 시각적 강조(하이라이트 및 조명) 상태를 설정합니다. */
	UFUNCTION(BlueprintNativeEvent, Category = "Nova|Deck")
	void SetHighlight(bool bIsOn);
	virtual void SetHighlight_Implementation(bool bIsOn);

	/** 유닛이 스폰되어 배치될 정확한 월드 트랜스폼을 반환합니다. */
	FTransform GetUnitSpawnTransform() const;

	/** 슬롯 인덱스 Getter */
	int32 GetSlotIndex() const { return SlotIndex; }

protected:
	/** 기준이 되는 루트 컴포넌트 (DefaultSceneRoot 역할) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Deck")
	TObjectPtr<USceneComponent> SceneRoot;

	/** [핵심] 유닛이 서 있을 위치 (에디터에서 자유롭게 위치 조정 가능) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Deck")
	TObjectPtr<USceneComponent> UnitSpawnPoint;

	/** 슬롯의 외형을 담당하는 메쉬 (하이라이트 효과의 기준점) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Deck")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	/** 이 슬롯의 고유 번호 (0~9, 레벨에 배치된 액터 인스턴스마다 수동 지정) */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Nova|Deck")
	int32 SlotIndex = 0;

	/** BaseMesh의 머티리얼 파라미터를 실시간으로 조절하기 위한 다이나믹 머티리얼 */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BaseDynamicMaterial;
};
