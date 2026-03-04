// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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

	// 외부에서 메시를 가져오기 위한 유틸리티 함수
	UPrimitiveComponent* GetMainMesh() const;

	// 부품 스탯 데이터를 가져오는 함수
	const class UNovaPartData* GetPartData() const { return PartData; }

	/** 애니메이션 제어: 이동 속도 연동 (다리 부품용) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Part|Animation")
	virtual void SetMovementSpeed(float Speed);

	/** 애니메이션 제어: 공격 애니메이션 재생 (무기 부품용) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Part|Animation")
	virtual void PlayAttackAnimation();

protected:
	// 이 부품의 성능 데이터 (PrimaryDataAsset)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Part")
	TObjectPtr<class UNovaPartData> PartData;

	// 부품이 스켈레탈 메시일 경우 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Part")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	// 부품이 스태틱 메시일 경우 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Part")
	TObjectPtr<UStaticMeshComponent> StaticMesh;
};
