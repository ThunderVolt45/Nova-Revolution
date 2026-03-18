// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/NovaAssemblyTypes.h"
#include "PreviewUnit.generated.h"

/**
 * APreviewUnit
 * 로비에서 유닛 조립 모습을 실시간으로 보여주는 가벼운 프리뷰 액터입니다.
 * AActor를 상속받아 불필요한 물리/AI 부하 제거 및 Preview에서 필요한 로직만 추가
 */

UCLASS()
class NOVAREVOLUTION_API APreviewUnit : public AActor
{
	GENERATED_BODY()
	
public:
	APreviewUnit();

	/** 실시간 조립 데이터 적용 (UI 버튼 클릭 시 호출) */
	UFUNCTION(BlueprintCallable, Category = "Lobby|Preview")
	void ApplyAssemblyData(const FNovaUnitAssemblyData& Data);

protected:
	/** 기존에 생성된 부품 액터들을 깨끗하게 제거 (중복 겹침 방지) */
	void ClearParts();

	/** 개별 부품 액터를 생성하고 소켓에 부착하는 헬퍼 함수 */
	class ANovaPart* SpawnPart(TSubclassOf<class ANovaPart> PartClass);

	// --- 현재 조립된 부품 참조 ---
	UPROPERTY(BlueprintReadOnly, Category = "Lobby|Preview")
	TObjectPtr<class ANovaPart> CurrentLegs;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby|Preview")
	TObjectPtr<class ANovaPart> CurrentBody;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby|Preview")
	TArray<TObjectPtr<class ANovaPart>> CurrentWeapons;

	// --- 설정 데이터 (에디터에서 지정) ---
	/** 부품 정보 조회를 위한 데이터 테이블 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby|Preview")
	TObjectPtr<class UDataTable> PartSpecDataTable;

	/** 다리 위에 몸통이 붙을 소켓 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby|Preview")
	FName BodySocketName = TEXT("BodySocket");

	/** 몸통 위에 무기가 붙을 소켓 이름들 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby|Preview")
	TArray<FName> WeaponSocketNames;

};
