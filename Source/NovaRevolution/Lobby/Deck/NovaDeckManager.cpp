// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/Deck/NovaDeckManager.h"

#include "Lobby/Deck/NovaDeckSlot.h"
#include "Lobby/Preview/PreviewUnit.h"

ANovaDeckManager::ANovaDeckManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ANovaDeckManager::UpdateSlotUnit(int32 SlotIndex, const FNovaUnitAssemblyData& UnitData)
{
	// 1. 해당 인덱스를 가진 슬롯 액터 탐색
	ANovaDeckSlot* TargetSlot = nullptr;
	for (ANovaDeckSlot* Slot : DeckSlots)
	{
		if (Slot && Slot->GetSlotIndex() == SlotIndex)
		{
			TargetSlot = Slot;
			break;
		}
	}

	// 슬롯이나 스폰할 클래스 정보가 없으면 중단
	if (!TargetSlot || !PreviewUnitClass) return;

	// 2. 해당 슬롯에 이미 스폰된 유닛이 있는지 확인
	TObjectPtr<APreviewUnit>* ExistingUnit = SpawnedUnits.Find(SlotIndex);
	APreviewUnit* UnitToUpdate = nullptr;

	if (ExistingUnit && *ExistingUnit)
	{
		// 이미 존재한다면 해당 유닛의 참조 유지
		UnitToUpdate = *ExistingUnit;
	}
	else
	{
		// 유닛이 없으면 슬롯의 스폰 포인트 트랜스폼을 기준으로 새로 생성
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		UnitToUpdate = GetWorld()->SpawnActor<APreviewUnit>(
			PreviewUnitClass, 
			TargetSlot->GetUnitSpawnTransform(), 
			SpawnParams
		);

		// 생성된 유닛을 맵에 등록하여 관리
		if (UnitToUpdate)
		{
			SpawnedUnits.Add(SlotIndex, UnitToUpdate);
		}
	}

	// 3. 조립 데이터 적용 (기존 PreviewUnit의 실시간 조립 로직 호출)
	if (UnitToUpdate)
	{
		UnitToUpdate->ApplyAssemblyData(UnitData);
	}
}

void ANovaDeckManager::ClearSlotUnit(int32 SlotIndex)
{
	// 슬롯 인덱스로 등록된 유닛이 있는지 확인 후 제거
	if (TObjectPtr<APreviewUnit>* FoundUnit = SpawnedUnits.Find(SlotIndex))
	{
		if (*FoundUnit)
		{
			// 월드에서 액터 제거 (부품들은 내부 풀링 시스템에 의해 처리됨)
			(*FoundUnit)->Destroy();
		}
        
		// 관리 맵에서 해당 인덱스 삭제
		SpawnedUnits.Remove(SlotIndex);
	}
}

