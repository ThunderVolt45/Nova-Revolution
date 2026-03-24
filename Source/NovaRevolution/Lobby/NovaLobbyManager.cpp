// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby/NovaLobbyManager.h"

#include "NovaRevolution.h"
#include "Core/NovaUnit.h"
#include "Core/NovaSaveGame.h"
#include "Core/NovaLog.h"
#include "Core/NovaPart.h"
#include "Deck/NovaDeckManager.h"
#include "Kismet/GameplayStatics.h"
#include "Preview/PreviewUnit.h"
#include "Core/NovaPartData.h"

ANovaLobbyManager::ANovaLobbyManager() 
{ 
    PrimaryActorTick.bCanEverTick = false; 
}

bool ANovaLobbyManager::IsCurrentAssemblyValid(FString& OutErrorMessage)
{
    float LegCapacity = 0.0f;
    float TotalWeight = 0.0f;

    // 1. 다리(Legs)의 적재량(Weight) 가져오기
    if (PendingAssemblyData.LegsClass)
    {
        if (ANovaPart* DefaultPart = PendingAssemblyData.LegsClass.GetDefaultObject())
        {
            FNovaPartSpecRow Spec = GetSpecByID(DefaultPart->GetPartID());
            LegCapacity = Spec.Weight; // Legs의 Weight 필드를 '적재량'으로 정의하여 사용
        }
    }

    // 2. 몸통(Body) 무게 합산
    if (PendingAssemblyData.BodyClass)
    {
        if (ANovaPart* DefaultPart = PendingAssemblyData.BodyClass.GetDefaultObject())
        {
            TotalWeight += GetSpecByID(DefaultPart->GetPartID()).Weight;
        }
    }

    // 3. 무기(Weapon) 무게 합산
    if (PendingAssemblyData.WeaponClass)
    {
        if (ANovaPart* DefaultPart = PendingAssemblyData.WeaponClass.GetDefaultObject())
        {
            TotalWeight += GetSpecByID(DefaultPart->GetPartID()).Weight;
        }
    }

    // 4. 비교 검사: 현재 합계가 다리의 적재량을 초과하는지 확인
    if (TotalWeight > LegCapacity)
    {
        OutErrorMessage = FString::Printf(TEXT("무게 초과: %.1f / %.1f"), TotalWeight, LegCapacity);
        return false;
    }

    return true;
    
}

void ANovaLobbyManager::BeginPlay()
{
    Super::BeginPlay();

    // 1. 세이브 파일로부터 기존 덱 데이터 로드 (최대 10개 슬롯)
    LoadDeckFromSave();

    // 2. 월드 내 핵심 매니저 및 프리뷰 액터 참조 자동 획득
    if (!PreviewUnit)
    {
        PreviewUnit = Cast<APreviewUnit>(UGameplayStatics::GetActorOfClass(GetWorld(), APreviewUnit::StaticClass()));
    }

    if (!DeckManager)
    {
        DeckManager = Cast<ANovaDeckManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ANovaDeckManager::StaticClass()));
    }

    // 3. 진입 시 기본적으로 0번 슬롯 데이터를 편집 대상으로 로드
    SelectDeckSlot(0);
    
    // 4. [수정] 로드된 전체 덱 정보를 순회하며 초기화
    if (DeckManager)
    {
        for (int32 i = 0; i < CurrentDeck.Units.Num(); ++i)
        {
            // 루트가 되는 다리(Legs) 클래스가 존재한다면 유닛이 조립된 상태로 판단
            if (CurrentDeck.Units[i].LegsClass)
            {
                // 유닛 액터를 배치하고 SceneCapture를 통해 썸네일 생성
                DeckManager->UpdateSlotUnit(i, CurrentDeck.Units[i]);
            }
            else
            {
                // [핵심] 조립되지 않은 빈 슬롯일 경우에도 명시적으로 클리어 호출!
                // 이를 통해 SceneCapture가 실행되어 렌더 타겟의 ClearColor(투명)를 반영하게 됩니다.
                DeckManager->ClearSlotUnit(i);
            }
        }
    }
}

FNovaPartSpecRow ANovaLobbyManager::GetTotalPendingSpec()
{
    FNovaPartSpecRow FinalSpec;
    FinalSpec.PartName = TEXT("Combined Stats");

    // 합산할 클래스 리스트 (다리, 몸통, 무기)
    TArray<TSubclassOf<ANovaPart>> Classes;
    Classes.Add(PendingAssemblyData.LegsClass);
    Classes.Add(PendingAssemblyData.BodyClass);
    Classes.Add(PendingAssemblyData.WeaponClass);

    for (auto& PartClass : Classes)
    {
        if (!PartClass) continue;

        if (ANovaPart* DefaultPart = PartClass.GetDefaultObject())
        {
            // [헬퍼 함수 호출!] 코드가 매우 직관적으로 변합니다.
            FNovaPartSpecRow PartSpec = GetSpecByID(DefaultPart->GetPartID());

            // 수치 합산
            FinalSpec.Watt += PartSpec.Watt;
            FinalSpec.Health += PartSpec.Health;
            FinalSpec.Attack += PartSpec.Attack;
            FinalSpec.Defense += PartSpec.Defense;
            FinalSpec.Speed += PartSpec.Speed;
            FinalSpec.FireRate += PartSpec.FireRate;
            FinalSpec.Sight += PartSpec.Sight;
            FinalSpec.Range += PartSpec.Range;
            FinalSpec.MinRange += PartSpec.MinRange;
            FinalSpec.SplashRange += PartSpec.SplashRange;

            // 타입별 고유 속성 전파
            if (PartSpec.PartType == ENovaPartType::Legs)
            {
                FinalSpec.MovementType = PartSpec.MovementType;
            }
            else if (PartSpec.PartType == ENovaPartType::Weapon)
            {
                FinalSpec.TargetType = PartSpec.TargetType;
                FinalSpec.bHomingProjectile = PartSpec.bHomingProjectile;
            }
        }
    }
    
    // [로직 추가] 방어력이 음수가 될 경우 0으로 보정
    // 특정 부품 조합이나 수치 합산 과정에서 발생할 수 있는 오류를 방지합니다.
    if (FinalSpec.Defense < 0.0f)
    {
        FinalSpec.Defense = 0.0f;
    }

    return FinalSpec;
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

FNovaPartSpecRow ANovaLobbyManager::GetSpecByID(FName PartID)
{
    // 데이터가 없을 경우를 대비한 기본 생성 구조체
    FNovaPartSpecRow FoundSpec;

    if (PartSpecTable && !PartID.IsNone())
    {
        // 데이터 테이블에서 해당 ID의 로우를 찾습니다.
        static const FString ContextString(TEXT("LobbyManager_GetSpecByID"));
        FNovaPartSpecRow* Row = PartSpecTable->FindRow<FNovaPartSpecRow>(PartID, ContextString);

        if (Row)
        {
            // 찾았다면 해당 데이터를 복사해서 반환합니다.
            FoundSpec = *Row;
        }
    }
    return FoundSpec;
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
    if (!PartAssetTable) return;

    static const FString Context(TEXT("LobbyManager Part Lookup"));
    FNovaPartAssetRow* AssetRow = PartAssetTable->FindRow<FNovaPartAssetRow>(PartID, Context);

    if (AssetRow && AssetRow->PartClass)
    {
        // [핵심] 실제 저장된 덱이 아닌 '임시 편집 데이터(Pending)'에 부품을 먼저 적용합니다.
        // 이를 통해 유저가 '확정' 버튼을 누르기 전까지는 원본 데이터를 보존합니다.
        switch (PartType)
        {
        case ENovaPartType::Legs:   PendingAssemblyData.LegsClass = AssetRow->PartClass; break;
        case ENovaPartType::Body:   PendingAssemblyData.BodyClass = AssetRow->PartClass; break;
        case ENovaPartType::Weapon:
            PendingAssemblyData.WeaponClass = AssetRow->PartClass;
           
            // [수정] 무기 부품의 경우, 스펙 테이블에서 'PartName'을 가져와 유닛의 대표 이름으로 설정합니다.
            FNovaPartSpecRow WeaponSpec = GetSpecByID(PartID);
            if (!WeaponSpec.PartName.IsEmpty())
            {
                // 데이터 테이블에 정의된 실제 이름을 사용
                PendingAssemblyData.UnitName = WeaponSpec.PartName;
            }
            else
            {
                // 데이터 테이블에 이름 정보가 없을 경우를 대비한 폴백(Fallback): ID를 문자열로 사용
                PendingAssemblyData.UnitName = PartID.ToString();
            }
            break;
        }

        // 중앙 메인 프리뷰 유닛의 외형을 즉시 갱신하여 유저에게 시각적 피드백 제공
        UpdatePreviewActor();
        
        // 추가
        OnAssemblyDataChanged.Broadcast(SelectedSlotIndex, PendingAssemblyData.UnitName, PendingAssemblyData);
    }
}

void ANovaLobbyManager::SelectDeckSlot(int32 SlotIndex)
{
    if (CurrentDeck.Units.IsValidIndex(SlotIndex))
    {
        SelectedSlotIndex = SlotIndex;

        // [복사] 선택된 슬롯의 원본 데이터를 편집용 임시 공간(Pending)으로 복제해옵니다.
        PendingAssemblyData = CurrentDeck.Units[SlotIndex];

        // 만약 데이터가 완전히 비어있는 신규 슬롯이라면 기본 지급 부품으로 초기화
        if (PendingAssemblyData.LegsClass == nullptr)
        {
            PendingAssemblyData.LegsClass = GetFirstPartClass(ENovaPartType::Legs);
            PendingAssemblyData.BodyClass = GetFirstPartClass(ENovaPartType::Body);
            PendingAssemblyData.WeaponClass = GetFirstPartClass(ENovaPartType::Weapon);
            PendingAssemblyData.UnitName = TEXT("New Unit");
        }
        
        // 2. [추가] 불러온 유닛 이름이 ID 형태이거나 비어있다면 실제 이름으로 보정
        if (PendingAssemblyData.WeaponClass)
        {
            if (ANovaPart* DefaultPart = PendingAssemblyData.WeaponClass.GetDefaultObject())
            {
                // 무기 ID를 기반으로 데이터 테이블에서 상세 스펙 조회
                FNovaPartSpecRow WeaponSpec = GetSpecByID(DefaultPart->GetPartID());

                // 보정 조건: 이름이 비어있거나, "New Unit"이거나, 혹은 ID 형태(예: Part_...)인 경우
                // 유저가 직접 커스텀 이름을 지은 경우가 아닐 때만 무기 이름으로 업데이트합니다.
                if (!WeaponSpec.PartName.IsEmpty() &&
                   (PendingAssemblyData.UnitName.IsEmpty() || 
                    PendingAssemblyData.UnitName == TEXT("New Unit") || 
                    PendingAssemblyData.UnitName.Contains(TEXT("Part_"))))
                {
                    PendingAssemblyData.UnitName = WeaponSpec.PartName;
                }
            }
        }

        UpdatePreviewActor();
        OnAssemblyDataChanged.Broadcast(SelectedSlotIndex, PendingAssemblyData.UnitName, PendingAssemblyData);
    }
}

void ANovaLobbyManager::ConfirmAssembly()
{
    if (!CurrentDeck.Units.IsValidIndex(SelectedSlotIndex)) return;
    
    FString ErrorMsg;
    // 조립 유효성 검사 (무게 제한 등)
    if (!IsCurrentAssemblyValid(ErrorMsg))
    {
        // 화면에 에러 로그 출력 및 진행 차단
        NOVA_SCREEN(Error, "조립 확정 불가: %s", *ErrorMsg);
        return;
    }

    // 1. 임시 편집 데이터(Pending)를 실제 덱 슬롯 데이터에 덮어씌워 확정(Commit)
    CurrentDeck.Units[SelectedSlotIndex] = PendingAssemblyData;

    // 2. 격납고 전시장(DeckManager)에 해당 슬롯 유닛의 외형 갱신 명령
    if (DeckManager)
    {
        DeckManager->UpdateSlotUnit(SelectedSlotIndex, PendingAssemblyData);
    }

    NOVA_SCREEN(Log, "Assembly Confirmed and Saved for Slot %d", SelectedSlotIndex);

    // 3. 변경된 덱 정보를 물리적 세이브 파일에 기록
    SaveCurrentDeck();
    
    // [이벤트 발송] 현재 이 델리게이트를 듣고 있는(AddDynamic) 모든 위젯들이 
    // 각자의 UpdateStatus(혹은 연결된 함수)를 동시에 실행하게 됩니다.
    OnAssemblyDataChanged.Broadcast(SelectedSlotIndex, PendingAssemblyData.UnitName, PendingAssemblyData);
}

void ANovaLobbyManager::ReleaseAssembly()
{
    if (!CurrentDeck.Units.IsValidIndex(SelectedSlotIndex)) return;

    // 1. 실제 덱 데이터에서 해당 슬롯 정보를 제거 (기본 구조체로 초기화)
    CurrentDeck.Units[SelectedSlotIndex] = FNovaUnitAssemblyData();

    // 2. 격납고 전시장 월드에서 해당 슬롯의 유닛 액터를 완전히 제거
    if (DeckManager)
    {
        DeckManager->ClearSlotUnit(SelectedSlotIndex);
    }

    // 3. 현재 편집 공간도 빈 상태(기본 세팅)로 되돌림
   // SelectDeckSlot(SelectedSlotIndex);

    NOVA_SCREEN(Log, "Assembly Released and Slot Cleared, Profile Setting is remained: %d", SelectedSlotIndex);
    
    // 파일 저장
    SaveCurrentDeck();
}

void ANovaLobbyManager::UpdatePreviewActor()
{
    if (PreviewUnit)
    {
        // 중앙 메인 프리뷰 유닛에 '임시 데이터'를 투영하여 실시간 조립 결과 시연
        PreviewUnit->ApplyAssemblyData(PendingAssemblyData);
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
    
    // 슬롯이 바뀌었을 때도 UI가 갱신되어야 하므로 신호를 보냅니다.
    OnAssemblyDataChanged.Broadcast(SelectedSlotIndex, PendingAssemblyData.UnitName, PendingAssemblyData);
}

