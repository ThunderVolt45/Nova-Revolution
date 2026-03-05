// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NovaPartData.generated.h"

/**
 * 부품의 종류 구분
 */
UENUM(BlueprintType)
enum class ENovaPartType : uint8
{
	None,
	Legs,
	Body,
	Weapon,
	Accessory
};

/**
 * 데이터 테이블에서 부품 스펙을 관리하기 위한 구조체
 */
USTRUCT(BlueprintType)
struct FNovaPartSpecRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 부품 이름 (표시용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Part")
	FString PartName;

	// 부품 종류
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Part")
	ENovaPartType PartType = ENovaPartType::None;

	// 와트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Stats")
	float Watt = 0.0f;

	// 무게 (조립 시 유효성 검사용: 다리 무게 >= 몸통+무기+악세 무게)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Stats")
	float Weight = 0.0f;

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

	// 스플래시 범위
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Stats")
	float SplashRange = 0.0f;
};
