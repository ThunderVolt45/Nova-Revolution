// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NovaAssemblyTypes.generated.h"

/**
 * 유닛의 무기 슬롯 정보를 담는 데이터 구조체
 */
USTRUCT(BlueprintType)
struct FNovaWeaponPartSlot
{
	GENERATED_BODY()

	// 부착할 무기 부품 클래스 (ANovaPart 상속 클래스)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Parts")
	TSubclassOf<class ANovaPart> WeaponPartClass;

	// 이 무기가 부착될 몸통(Body)의 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Unit|Parts")
	FName TargetSocketName;
};

USTRUCT(BlueprintType)
struct FNovaUnitAssemblyData
{
	GENERATED_BODY()

	// 다리 부품 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Assembly")
	TSubclassOf<class ANovaPart> LegsClass;

	// 몸통 부품 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Assembly")
	TSubclassOf<class ANovaPart> BodyClass;

	// 무기 부품 클래스 (단일 종류)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Assembly")
	TSubclassOf<class ANovaPart> WeaponClass;

	// 유닛의 커스텀 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Assembly")
	FString UnitName = TEXT("Custom Unit");
};

/**
 * 플레이어의 전체 덱 정보 (최대 10슬롯)
 */
USTRUCT(BlueprintType)
struct FNovaDeckInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Assembly", meta = (TitleProperty = "UnitName"))
	TArray<FNovaUnitAssemblyData> Units;

	// 최대 슬롯 제한 (기획상 10개)
	static constexpr int32 MaxDeckSize = 10;
};
