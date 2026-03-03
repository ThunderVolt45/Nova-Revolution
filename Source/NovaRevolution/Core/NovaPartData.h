// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NovaPartData.generated.h"

/**
 * 유닛 부품의 성능 수치를 담는 데이터 에셋
 */
UCLASS(BlueprintType)
class NOVAREVOLUTION_API UNovaPartData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 와트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Stats")
	float Watt = 0.0f;

	// 체력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Stats")
	float Health = 0.0f;

	// 공격력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Stats")
	float Attack = 0.0f;

	// 방어력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Stats")
	float Defense = 0.0f;

	// 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Stats")
	float Speed = 0.0f;

	// 연사력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Stats")
	float FireRate = 0.0f;

	// 시야
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Stats")
	float Sight = 0.0f;

	// 사거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Stats")
	float Range = 0.0f;

	// 최소 사거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Stats")
	float MinRange = 0.0f;

	// 스플래시 범위 (필요시)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Stats")
	float SplashRange = 0.0f;
};
