// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NovaTypes.generated.h"

/**
 * RTS 팀 구분 상수
 */
namespace NovaTeam
{
	constexpr int32 None = -1;
	constexpr int32 Neutral = 0;
	constexpr int32 Team1 = 1;
	constexpr int32 Team2 = 2;
}

/**
 * 선택 가능한 액터의 타입 구분
 */
UENUM(BlueprintType)
enum class ENovaSelectableType : uint8
{
	None,
	Unit,       // 유닛 (로봇)
	Base,       // 기지 (건물)
	Interactive // 기타 상호작용 가능한 대상
};

/**
 * RTS 명령의 종류 (노바 1492 기준)
 */
UENUM(BlueprintType)
enum class ECommandType : uint8
{
	None,
	Move,       // 우클릭 이동
	Attack,     // A + 좌클릭 / 적 우클릭
	Patrol,     // P + 좌클릭
	Hold,       // H (제자리 대기 및 공격)
	Stop,       // S (현재 명령 취소)
	Spread,     // C (유닛 산개)
	Halt        // L (완전 정지)
};

/**
 * 명령 전달을 위한 패키지 데이터
 */
USTRUCT(BlueprintType)
struct FCommandData
{
	GENERATED_BODY()

	// 명령 종류
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
	ECommandType CommandType = ECommandType::None;

	// 목표 지점 (이동, 패트롤 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
	FVector TargetLocation = FVector::ZeroVector;

	// 목표 대상 (공격 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
	TObjectPtr<AActor> TargetActor = nullptr;
};
