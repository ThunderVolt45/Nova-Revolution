// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/NovaAssemblyTypes.h"
#include "PreviewUnit.generated.h"

/**
 * APreviewUnit
 * 로비 레벨에서 실시간으로 부품을 교체하며 유닛의 조립 상태를 보여주는 프리뷰 전용 액터입니다.
 * 오브젝트 풀링 시스템과 연동하여 부품 스폰 및 소켓 부착을 관리합니다.
 */
UCLASS()
class NOVAREVOLUTION_API APreviewUnit : public AActor
{
	GENERATED_BODY()

public:
	APreviewUnit();

	/** * 실시간 조립 데이터 적용 (LobbyManager 등에서 호출) 
	 * 전달받은 데이터를 바탕으로 현재 장착된 부품들을 즉시 교체합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby")
	void ApplyAssemblyData(const FNovaUnitAssemblyData& Data);

protected:
	/** * 풀링 시스템을 통해 파츠를 생성하거나 교체하는 헬퍼 함수 
	 * @param NewPartClass 새로 생성할 부품 클래스
	 * @param CurrentPart 현재 장착된 부품 참조 (교체 시 풀로 반환됨)
	 * @return 생성된 새로운 부품 액터의 포인터
	 */
	class ANovaPart* UpdatePart(TSubclassOf<class ANovaPart> NewPartClass, TObjectPtr<class ANovaPart>& CurrentPart);

	/** 모든 부품의 계층 구조(소켓 부착)를 데이터에 맞춰 다시 정렬합니다. */
	void RefreshAttachments();

	// --- 부품 실시간 참조 ---
    
	UPROPERTY(VisibleAnywhere, Category = "Nova|Lobby")
	TObjectPtr<class ANovaPart> CurrentLegs;

	UPROPERTY(VisibleAnywhere, Category = "Nova|Lobby")
	TObjectPtr<class ANovaPart> CurrentBody;

	UPROPERTY(VisibleAnywhere, Category = "Nova|Lobby")
	TArray<TObjectPtr<class ANovaPart>> CurrentWeapons;

	// --- 설정 데이터 및 소켓 정보 ---
    
	/** 부품 정보를 조회하기 위한 데이터 테이블 */
	UPROPERTY(EditAnywhere, Category = "Nova|Lobby")
	TObjectPtr<class UDataTable> PartDataTable;

	/** 다리(Legs) 위에 몸통(Body)이 붙을 소켓 이름 */
	UPROPERTY(EditAnywhere, Category = "Nova|Lobby")
	FName BodySocketName = TEXT("Socket_Body");

	/** 몸통(Body) 위에 무기(Weapon)들이 붙을 소켓 이름 리스트 */
	UPROPERTY(EditAnywhere, Category = "Nova|Lobby")
	TArray<FName> WeaponSocketNames;
};
