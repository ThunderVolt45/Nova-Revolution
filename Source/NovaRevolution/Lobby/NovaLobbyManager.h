// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/NovaAssemblyTypes.h"
#include "Core/NovaPartData.h"
#include "NovaLobbyManager.generated.h"

/** 조립 데이터가 변경되었을 때 UI나 다른 시스템에 알리는 2개 파라미터 델리게이트 선언 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAssemblyDataChanged, int32, SlotIndex, const FString&, UnitName);

class ANovaDeckManager; // 전시장 매니저 전방 선언
class APreviewUnit;     // 프리뷰 유닛 전방 선언

/**
 * ANovaLobbyManager
 * 로비에서 유닛의 덱(조립 정보)을 관리하고, UI와 월드 프리뷰 유닛 사이를 중계하는 매니저 액터입니다.
 * 세이브 데이터 로드, 임시 조립 데이터 관리, 격납고 전시장 갱신을 담당합니다.
 */
UCLASS()
class NOVAREVOLUTION_API ANovaLobbyManager : public AActor
{
    GENERATED_BODY()

public:
    ANovaLobbyManager();
    
    /** * UI 위젯이 구독(Bind)할 수 있도록 노출된 델리게이트입니다. 
     * BlueprintAssignable 설정으로 블루프린트 위젯에서도 직접 이벤트를 받을 수 있습니다.
     */
    UPROPERTY(BlueprintAssignable, Category = "Nova|Lobby|Event")
    FOnAssemblyDataChanged OnAssemblyDataChanged;

protected:
    virtual void BeginPlay() override;

public:
    /** 특정 카테고리의 부품을 선택하여 현재 임시 데이터(Pending)에 반영합니다. */
    UFUNCTION(BlueprintCallable, Category = "Nova|Lobby")
    void SelectPart(ENovaPartType PartType, FName PartID);

    /** 편집할 덱 슬롯을 선택합니다. (해당 슬롯 데이터를 Pending으로 불러오고 카메라 전환 트리거) */
    UFUNCTION(BlueprintCallable, Category = "Nova|Lobby")
    void SelectDeckSlot(int32 SlotIndex);

    /** [신규] 현재 임시 데이터를 선택된 슬롯에 최종 저장하고 격납고 전시장을 갱신합니다. */
    UFUNCTION(BlueprintCallable, Category = "Nova|Lobby")
    void ConfirmAssembly();

    /** [신규] 현재 슬롯의 데이터를 초기화하고 전시장에서 해당 유닛을 제거합니다. */
    UFUNCTION(BlueprintCallable, Category = "Nova|Lobby")
    void ReleaseAssembly();

    /** 현재 임시 데이터(Pending)에 맞춰 중앙 메인 프리뷰 유닛의 외형을 즉시 갱신합니다. */
    void UpdatePreviewActor();

    /** 현재 편집 중인 전체 덱 정보를 세이브 파일에 물리적으로 저장합니다. */
    UFUNCTION(BlueprintCallable, Category = "Nova|Lobby")
    void SaveCurrentDeck();

    // --- Getter Functions ---
    const FNovaDeckInfo& GetCurrentDeck() const { return CurrentDeck; }
    int32 GetSelectedSlotIndex() const { return SelectedSlotIndex; }
    const FNovaUnitAssemblyData& GetPendingData() const { return PendingAssemblyData; }

protected:
    /** 게임 시작 시 기존 저장된 덱 데이터를 로드하여 CurrentDeck을 채웁니다. */
    void LoadDeckFromSave();

    /** 특정 카테고리에 해당하는 첫 번째 부품 클래스를 데이터 테이블에서 검색하여 반환합니다. (초기화용) */
    TSubclassOf<class ANovaPart> GetFirstPartClass(ENovaPartType Category);

    /** 부품 스펙 테이블 참조 (카테고리 및 파츠 타입 판별용) */
    UPROPERTY(EditAnywhere, Category = "Nova|Lobby")
    TObjectPtr<UDataTable> PartSpecTable;

    /** 플레이어의 실제 덱 정보 (최대 10개 슬롯의 조립 데이터 보관) */
    UPROPERTY(BlueprintReadOnly, Category = "Nova|Lobby")
    FNovaDeckInfo CurrentDeck;

    /** [신규] 현재 UI에서 편집 중이지만 아직 확정(Confirm)되지 않은 임시 조립 데이터 */
    UPROPERTY(BlueprintReadOnly, Category = "Nova|Lobby")
    FNovaUnitAssemblyData PendingAssemblyData;

    /** 현재 UI 및 편집 모드에서 선택된 슬롯 인덱스 (0 ~ 9) */
    UPROPERTY(BlueprintReadOnly, Category = "Nova|Lobby")
    int32 SelectedSlotIndex = 0;

    /** 부품 ID로 실제 블루프린트 클래스를 찾기 위한 에셋 데이터 테이블 */
    UPROPERTY(EditAnywhere, Category = "Nova|Lobby")
    TObjectPtr<UDataTable> PartAssetTable;

    /** 화면 중앙에서 실시간 조립 모습을 보여줄 메인 프리뷰 유닛 */
    UPROPERTY(EditAnywhere, Category = "Nova|Lobby")
    TObjectPtr<APreviewUnit> PreviewUnit;

    /** [신규] 레벨에 배치되어 격납고 전체 유닛 전시를 관리하는 덱 매니저 참조 */
    UPROPERTY(BlueprintReadOnly, Category = "Nova|Lobby")
    TObjectPtr<ANovaDeckManager> DeckManager;
};
