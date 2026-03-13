// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/NovaPartData.h"
#include "GameplayTagContainer.h"
#include "Core/NovaInterfaces.h"
#include "NovaPart.generated.h"

/**
 * Nova Revolution의 유닛 부품(Legs, Body, Weapon 등)의 베이스 클래스
 */
UCLASS()
class NOVAREVOLUTION_API ANovaPart : public AActor, public INovaObjectPoolable
{
	GENERATED_BODY()
	
public:	
	ANovaPart();

	// --- INovaObjectPoolable ---
	virtual void OnSpawnFromPool_Implementation() override;
	virtual void OnReturnToPool_Implementation() override;

	/** 부품 정보 설정을 위한 함수 (주로 ANovaUnit에서 호출) */
	void SetPartID(const FName& InID) { PartID = InID; }
	FName GetPartID() const { return PartID; }
	void SetPartDataTable(class UDataTable* InTable) 
	{ 
		PartDataTable = InTable; 
		InitializePartSpec();
	}

	// 외부에서 메시를 가져오기 위한 유틸리티 함수
	UPrimitiveComponent* GetMainMesh() const;

	// 데이터 테이블 기반 스펙을 가져오는 함수
	const FNovaPartSpecRow& GetPartSpec() const { return PartSpec; }

	/** 애니메이션 제어: 이동 속도 연동 (다리 부품용) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Part|Animation")
	virtual void SetMovementSpeed(float Speed);

	/** 애니메이션 제어: 회전 속도 연동 (다리 부품용) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Part|Animation")
	virtual void SetRotationRate(float Rate);

	/** 애니메이션 제어: 사망 상태 연동 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Part|Animation")
	virtual void SetIsDead(bool bDead);

	/** 애니메이션 제어: 공격 애니메이션 재생 (무기 부품용) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Part|Animation")
	virtual void PlayAttackAnimation();

	/** 공격 효과(애니메이션 + 발사 GameplayCue)를 재생합니다. (무기 부품용) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Part|Effects")
	void PlayFireEffects();

	/** 적중 시 사용할 GameplayCue 태그를 반환합니다. */
	FGameplayTag GetImpactCueTag() const { return ImpactCueTag; }

	/** 설정된 모든 총구 소켓 이름들을 반환합니다. */
	const TArray<FName>& GetMuzzleSocketNames() const { return MuzzleSocketNames; }

	/** 발사 효과가 생성될 소켓 이름들을 반환합니다. (주로 첫 번째 소켓 위치를 위해 사용하거나 내부적으로 활용) */
	// FName GetRandomMuzzleSocketName() const;
	
	// 무기 조준 관련 함수
	/** 목표 Pitch 각도를 설정합니다. (ANovaUnit에서 호출) */
	void SetTargetPitch(float NewPitch) { TargetPitch = NewPitch; }

	/** 조준 각도를 부드럽게 업데이트하고 ABP로 전달합니다. */
	void UpdateAiming(float DeltaTime);

	/** 파츠와 모든 자식 메시의 Charred 효과를 업데이트합니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Part|Effects")
	void SetCharredAlpha(float Alpha);

	/** 파츠를 본체에서 분리하고 물리력을 가해 튕겨나가게 합니다. */
	void ExplodeAndDetach(FVector Impulse);

	// 데이터 테이블에서 스펙을 불러오는 함수
	void InitializePartSpec();

protected:
	virtual void BeginPlay() override;

protected:
	// 부품 식별 ID (데이터 테이블의 Row Name과 일치해야 함)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Part")
	FName PartID;

	// 부품 스펙 데이터 테이블
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Part")
	TObjectPtr<class UDataTable> PartDataTable;

	// 현재 부품에 할당된 최종 스펙
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Part")
	FNovaPartSpecRow PartSpec;

	// --- 애니메이션 데이터 ---
	// 이 부품이 공격 시 재생할 몽타주 (무기 부품용)
	// 부품 에셋마다 고유한 애니메이션이 필요하므로 클래스에서 직접 들고 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Part|Animation")
	TObjectPtr<class UAnimMontage> AttackMontage;

	/** 발사 시 실행할 GameplayCue 태그 (무기 부품용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Part|Effects")
	FGameplayTag FireCueTag;

	/** 적중 시 실행할 GameplayCue 태그 (발사체나 히트스캔 로직에서 참조용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Part|Effects")
	FGameplayTag ImpactCueTag;

	/** 발사 효과가 생성될 소켓 이름들 (동시 생성) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Part|Effects")
	TArray<FName> MuzzleSocketNames;

	// 부품이 스켈레탈 메시일 경우 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Part")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	// 부품이 스태틱 메시일 경우 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Part")
	TObjectPtr<UStaticMeshComponent> StaticMesh;
	
	//무기 조준 관련 변수
protected:
	// 현재 보간 중인 각도
	float CurrentPitch = 0.0f;

	// 유닛 본체로부터 전달받은 목표 각도
	float TargetPitch = 0.0f;

	// 조준 회전 속도 (값이 클수록 타겟을 빠르게 쫓습니다.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Part|Rotation")
	float AimInterpSpeed = 40.0f;
};
