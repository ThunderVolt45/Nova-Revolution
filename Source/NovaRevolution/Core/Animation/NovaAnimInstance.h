// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NovaAnimInstance.generated.h"

/**
 * Nova Revolution 유닛 부품들의 공통 애니메이션 인스턴스 베이스
 */
UCLASS()
class NOVAREVOLUTION_API UNovaAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	// 이동 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Animation")
	float MovementSpeed = 0.0f;

	// 회전 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Animation")
	float RotationRate = 0.0f;

	// 사망 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Animation")
	bool bIsDead = false;
	
	// 원본 메쉬 대비 조정된 스케일 비율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Animation")
	float MeshScaleValue = 0.0f;
	
	// 해당 애니메이션의 원본 이동 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Animation")
	float AnimMaxSpeed = 0.0f;
};
