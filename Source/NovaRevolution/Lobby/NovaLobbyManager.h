// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/NovaAssemblyTypes.h"
#include "Core/NovaPartData.h"
#include "NovaLobbyManager.generated.h"

/**
 * ANovaLobbyManager
 * 로비에서 유닛의 덱(조립 정보)을 관리하고, UI와 월드 프리뷰 유닛 사이를 중계하는 매니저 액터입니다.
 */
UCLASS()
class NOVAREVOLUTION_API ANovaLobbyManager : public AActor
{
	GENERATED_BODY()

public:
	ANovaLobbyManager();

protected:
	virtual void BeginPlay() override;

public:
	/** * 특정 카테고리의 부품을 선택하여 현재 덱에 반영합니다.
	 * @param PartType 부품 종류 (다리, 몸통, 무기 등)
	 * @param PartID 데이터 테이블의 행 이름(ID)
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby")
	void SelectPart(ENovaPartType PartType, FName PartID);

	/** 편집할 덱 슬롯을 선택합니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby")
	void SelectDeckSlot(int32 SlotIndex);

	/** 현재 편집 중인 덱 정보를 세이브 파일이나 서버에 저장합니다. */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby")
	void SaveCurrentDeck();

	/** 현재 덱 데이터에 맞춰 월드에 배치된 프리뷰 유닛의 외형을 즉시 갱신합니다. */
	void UpdatePreviewActor();

	// --- Getter ---
	const FNovaDeckInfo& GetCurrentDeck() const { return CurrentDeck; }
	int32 GetSelectedSlotIndex() const { return SelectedSlotIndex; }

protected:
	/** 게임 시작 시 기존 저장된 덱 데이터를 로드합니다. */
	void LoadDeckFromSave();
	
	/** 데이터 테이블의 첫 번째 부품들로 현재 덱 슬롯을 초기화합니다. */
	void InitializeDefaultDeck(); 
	
	// 특정 카테고리(Legs, Body, Weapon)에 해당하는 첫 번째 부품 클래스를  데이터 테이블에서 검색하여 반환합니다. 
	TSubclassOf<class ANovaPart> GetFirstPartClass(ENovaPartType Category);

	// [중요] 부품의 카테고리(Type) 정보를 파악하기 위해 Spec 테이블 참조가 필요합니다.
	// 에디터의 LobbyManager 디테일 패널에서 반드시 할당해야 합니다.
	UPROPERTY(EditAnywhere, Category = "Nova|Lobby")
	TObjectPtr<UDataTable> PartSpecTable;
    
	/** 현재 선택된 유닛의 조립 정보 (슬롯 데이터 포함) */
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Lobby")
	FNovaDeckInfo CurrentDeck;

	/** 현재 UI에서 편집 중인 슬롯 인덱스 */
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Lobby")
	int32 SelectedSlotIndex = 0;

	/** 부품 ID로 클래스를 찾기 위한 에셋 데이터 테이블 (에디터에서 할당) */
	UPROPERTY(EditAnywhere, Category = "Nova|Lobby")
	TObjectPtr<UDataTable> PartAssetTable;

	UPROPERTY(EditAnywhere, Category = "Nova|Lobby")
	TObjectPtr<class APreviewUnit> PreviewUnit; // 타입을 실시간 프리뷰 전용 액터로 변경
};
