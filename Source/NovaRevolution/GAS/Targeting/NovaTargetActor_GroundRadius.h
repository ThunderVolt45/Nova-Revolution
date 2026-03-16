// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "NovaTargetActor_GroundRadius.generated.h"


/**
 * 타겟팅 필터링 타입: 누구를 타겟팅할지 결정합니다.
 */
UENUM(BlueprintType)
enum class ENovaTargetingFilter : uint8
{
	Ally    UMETA(DisplayName = "아군만"),
	Enemy   UMETA(DisplayName = "적군만"),
	Both    UMETA(DisplayName = "모두")
};

/**
 * ANovaTargetActor_GroundRadius
 * 마우스 커서 위치를 지면에 투사하여 일정 반지름 내의 유닛들을 찾는 타겟 액터입니다.
 */
UCLASS()
class NOVAREVOLUTION_API ANovaTargetActor_GroundRadius : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()
	
public:
	ANovaTargetActor_GroundRadius();
	
	/** 타겟팅 영역 내 유닛의 하이라이트 색상 (에디터에서 분홍색으로 설정 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Targeting")
	FLinearColor CurrentHighlightColor = FLinearColor::White; // 기본 흰색

	/** 타겟팅 반지름 (기본값: 700 units / Nova 기준 7) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Targeting")
	float Radius = 700.0f;

	/** 누구를 타겟팅할 것인가? (GA에서 설정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Targeting")
	ENovaTargetingFilter TargetingFilter = ENovaTargetingFilter::Ally;
	
	// 타겟팅이 시작될 때 GA로부터 넘어온 Radius를 실제 캡슐 크기에 적용합니다.
	virtual void StartTargeting(UGameplayAbility* Ability) override;

	// --- GameplayAbilityTargetActor 인터페이스 오버라이드 ---
	/** 사용자가 마우스를 클릭(Confirm)했을 때 호출되어 실제 데이터를 확정합니다. */
	virtual void ConfirmTargetingAndContinue() override;

	/** 매 프레임 마우스 위치를 추적하여 액터 위치를 갱신합니다. */
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Targeting")
	TObjectPtr<class UCapsuleComponent> CollisionCapsule;
	
	// NovaUnit Material Highlight 관련
	/** 이전 프레임에서 하이라이트되었던 유닛들 (영역 이탈 시 끄기 위함) */
	UPROPERTY()
	TArray<TWeakObjectPtr<class ANovaUnit>> LastHighlightedUnits;

	/** 액터가 파괴될 때(스킬 종료/취소) 모든 하이라이트를 끕니다. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	/** 마우스 아래의 지면 좌표를 가져오는 헬퍼 함수 */
	FVector GetMouseLocationOnGround() const;

	/** 팀 ID와 필터링 설정을 비교하여 유효한 타겟인지 판별합니다. */
	bool IsValidTarget(int32 MyTeamID, int32 TargetTeamID) const;
	
	/** 현재 범위 내에 있는 유효한 아군 유닛들을 찾아 반환하는 공통 함수 */
	void GetFilteredActorsInRange(TArray<AActor*>& OutActors) const;
	
};
