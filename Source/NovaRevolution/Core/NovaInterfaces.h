// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Core/NovaTypes.h"
#include "NovaInterfaces.generated.h"

// --- 팀 시스템 인터페이스 ---
UINTERFACE(MinimalAPI)
class UNovaTeamInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 팀 소속을 가지는 모든 액터들이 구현해야 하는 인터페이스
 */
class NOVAREVOLUTION_API INovaTeamInterface
{
	GENERATED_BODY()

public:
	// 현재 소속된 팀 ID를 반환
	virtual int32 GetTeamID() const = 0;

	// 아군 여부 확인
	virtual bool IsFriendly(int32 OtherTeamID) const { return GetTeamID() == OtherTeamID; }
};

// --- 유닛 선택 시스템 인터페이스 ---
UINTERFACE(MinimalAPI)
class UNovaSelectableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 선택 가능한 모든 액터들이 구현해야 하는 인터페이스
 */
class NOVAREVOLUTION_API INovaSelectableInterface
{
	GENERATED_BODY()

public:
	// 선택 시 호출 (데칼 켜기 등)
	virtual void OnSelected() = 0;

	// 선택 해제 시 호출 (데칼 끄기 등)
	virtual void OnDeselected() = 0;

	// 선택 가능한 상태인지 확인
	virtual bool IsSelectable() const { return true; }
};

// --- 명령 전달 시스템 인터페이스 ---
UINTERFACE(MinimalAPI)
class UNovaCommandInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 명령을 수행할 수 있는 모든 액터(또는 AIController)들이 구현해야 하는 인터페이스
 */
class NOVAREVOLUTION_API INovaCommandInterface
{
	GENERATED_BODY()

public:
	// 명령 데이터 패키지를 전달받아 처리
	virtual void IssueCommand(const FCommandData& CommandData) = 0;
};
