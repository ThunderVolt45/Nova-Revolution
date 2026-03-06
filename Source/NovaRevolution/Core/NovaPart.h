// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/NovaPartData.h"
#include "NovaPart.generated.h"

/**
 * Nova Revolution의 유닛 부품(Legs, Body, Weapon 등)의 베이스 클래스
 */
UCLASS()
class NOVAREVOLUTION_API ANovaPart : public AActor
{
	GENERATED_BODY()
	
public:	
	ANovaPart();

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

protected:
	virtual void BeginPlay() override;

	// 데이터 테이블에서 스펙을 불러오는 함수
	void InitializePartSpec();

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

	// 부품이 스켈레탈 메시일 경우 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Part")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	// 부품이 스태틱 메시일 경우 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Part")
	TObjectPtr<UStaticMeshComponent> StaticMesh;
};
