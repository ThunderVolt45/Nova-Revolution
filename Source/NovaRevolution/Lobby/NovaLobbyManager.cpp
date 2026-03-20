// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby/NovaLobbyManager.h"

#include "NovaRevolution.h"
#include "Core/NovaUnit.h"
#include "Core/NovaSaveGame.h"
#include "Core/NovaLog.h"
#include "Core/NovaPart.h"
#include "Kismet/GameplayStatics.h"
#include "Preview/PreviewUnit.h"

ANovaLobbyManager::ANovaLobbyManager() 
{ 
    PrimaryActorTick.bCanEverTick = false; 
}

void ANovaLobbyManager::BeginPlay()
{
    Super::BeginPlay();

    // 1. 기존에 저장된 덱 정보 로드
    LoadDeckFromSave();

    // [해결 1] 세이브가 없을 때 '기본 조립 세트'를 먼저 생성합니다.
    // 위젯이 카테고리의 첫 번째 부품(Index 0)을 기본으로 보여주는 것과 데이터 정합성을 맞춥니다.
    if (CurrentDeck.Units.IsValidIndex(SelectedSlotIndex) && CurrentDeck.Units[SelectedSlotIndex].LegsClass == nullptr)
    {
        // 데이터 테이블에서 각 카테고리의 첫 번째 부품을 찾아 기본 덱을 채워주는 전용 함수 호출
        InitializeDefaultDeck(); 
    }

    // 월드에 배치된 프리뷰 액터 참조 획득
    if (!PreviewUnit)
    {
        PreviewUnit = Cast<APreviewUnit>(UGameplayStatics::GetActorOfClass(GetWorld(), APreviewUnit::StaticClass()));
    }

    // [해결 2] 로비 진입 직후, 기본 데이터에 맞춰 조립된 유닛 외형을 즉시 출력합니다.
    // 이를 통해 플레이어는 빈 화면이 아닌 '기본 유닛'이 서 있는 완성된 로비를 처음부터 보게 됩니다.
    UpdatePreviewActor();
}

void ANovaLobbyManager::LoadDeckFromSave()
{
    const FString SaveSlotName = TEXT("NovaPlayerSaveSlot");

    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
    {
        if (UNovaSaveGame* LoadedGame = Cast<UNovaSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0)))
        {
            CurrentDeck = LoadedGame->SavedDeck;
            return;
        }
    }

    // 저장 파일이 없을 경우 초기 덱 슬롯(10개) 생성
    CurrentDeck.Units.SetNum(10);
}

void ANovaLobbyManager::InitializeDefaultDeck()
{
    // 현재 선택된 덱 슬롯 인덱스가 유효한지 확인합니다.
    if (!CurrentDeck.Units.IsValidIndex(SelectedSlotIndex)) return;

    // 현재 슬롯의 조립 데이터 참조를 획득합니다.
    FNovaUnitAssemblyData& DefaultData = CurrentDeck.Units[SelectedSlotIndex];

    // 각 카테고리(다리, 몸통, 무기)별로 데이터 테이블상의 첫 번째 부품 클래스를 할당합니다.
    // 이를 통해 신규 유저가 접속했을 때 빈 화면이 아닌 기본 유닛 조립 상태를 보게 됩니다.
    DefaultData.LegsClass = GetFirstPartClass(ENovaPartType::Legs);
    DefaultData.BodyClass = GetFirstPartClass(ENovaPartType::Body);
    DefaultData.WeaponClass = GetFirstPartClass(ENovaPartType::Weapon);
    
    // 초기 유닛 이름 설정
    DefaultData.UnitName = TEXT("Initial Unit");

    // 초기화 성공 로그 출력
    NOVA_LOG(Log, "Default Deck slots initialized for slot %d", SelectedSlotIndex);
}

TSubclassOf<class ANovaPart> ANovaLobbyManager::GetFirstPartClass(ENovaPartType Category)
{
    // 데이터 테이블 참조가 유효한지 먼저 확인합니다.
    if (!PartSpecTable || !PartAssetTable) return nullptr;

    // 1. 부품 스펙 테이블의 모든 행(Row) 이름을 가져옵니다.
    TArray<FName> RowNames = PartSpecTable->GetRowNames();

    for (const FName& RowName : RowNames)
    {
        // 2. 해당 행의 스펙 데이터를 읽어 요청한 카테고리와 일치하는지 확인합니다.
        FNovaPartSpecRow* Spec = PartSpecTable->FindRow<FNovaPartSpecRow>(RowName, TEXT(""));

        if (Spec && Spec->PartType == Category)
        {
            // 3. 일치하는 첫 번째 부품을 찾았다면, AssetTable에서 실제 스폰 가능한 클래스를 가져옵니다.
            static const FString Context(TEXT("GetFirstPart Lookup"));
            FNovaPartAssetRow* Asset = PartAssetTable->FindRow<FNovaPartAssetRow>(RowName, Context);

            if (Asset && Asset->PartClass)
            {
                // 찾은 첫 번째 유효한 클래스를 즉시 반환합니다.
                return Asset->PartClass;
            }
        }
    }

    // 일치하는 카테고리의 부품을 하나도 찾지 못한 경우
    return nullptr;
}

void ANovaLobbyManager::SelectPart(ENovaPartType PartType, FName PartID)
{
    if (!CurrentDeck.Units.IsValidIndex(SelectedSlotIndex) || !PartAssetTable) return;

    static const FString Context(TEXT("LobbyManager Part Lookup"));
    FNovaPartAssetRow* AssetRow = PartAssetTable->FindRow<FNovaPartAssetRow>(PartID, Context);

    if (AssetRow && AssetRow->PartClass)
    {
        FNovaUnitAssemblyData& UnitData = CurrentDeck.Units[SelectedSlotIndex];
        
        // 타입별 부품 클래스 정보 업데이트
        switch (PartType)
        {
            case ENovaPartType::Legs:   UnitData.LegsClass = AssetRow->PartClass; break;
            case ENovaPartType::Body:   UnitData.BodyClass = AssetRow->PartClass; break;
            case ENovaPartType::Weapon: UnitData.WeaponClass = AssetRow->PartClass; break;
        }

        // 데이터 변경 후 즉시 외형 갱신
        UpdatePreviewActor();
    }
}

void ANovaLobbyManager::SelectDeckSlot(int32 SlotIndex)
{
    if (CurrentDeck.Units.IsValidIndex(SlotIndex))
    {
        SelectedSlotIndex = SlotIndex;
        UpdatePreviewActor();
    }
}

void ANovaLobbyManager::UpdatePreviewActor()
{
    // 프리뷰 액터와 현재 선택된 덱 슬롯의 유효성을 검사합니다.
    if (PreviewUnit && CurrentDeck.Units.IsValidIndex(SelectedSlotIndex))
    {
        // 프리뷰 액터 고유의 조립 함수(ApplyAssemblyData)를 호출합니다.
        // 이 과정에서 풀링 시스템을 통한 부품 교체와 소켓 재부착이 한꺼번에 일어납니다.
        PreviewUnit->ApplyAssemblyData(CurrentDeck.Units[SelectedSlotIndex]);
        
        // 데이터와 시각적 요소의 동기화가 완료되었음을 알립니다.
        NOVA_LOG(Log, "Lobby: PreviewUnit successfully updated with Slot %d data.", SelectedSlotIndex);
    }
}

void ANovaLobbyManager::SaveCurrentDeck()
{
    const FString SaveSlotName = TEXT("NovaPlayerSaveSlot");

    if (UNovaSaveGame* SaveGameInstance = Cast<UNovaSaveGame>(UGameplayStatics::CreateSaveGameObject(UNovaSaveGame::StaticClass())))
    {
        SaveGameInstance->SavedDeck = CurrentDeck;
        
        if (UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveSlotName, 0))
        {
            NOVA_SCREEN(Log, "Deck Successfully Saved to File.");
        }
    }
}

