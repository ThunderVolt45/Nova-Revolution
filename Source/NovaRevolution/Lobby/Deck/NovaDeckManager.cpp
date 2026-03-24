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
	}

	// 3. 유닛 설정 적용 및 캡처 타겟 지정
	if (UnitToUpdate)
	{
		// 슬롯 번호 주입 (클릭 이벤트 처리를 위함)
		UnitToUpdate->SetSlotIndex(SlotIndex);

		// 유닛 배율 설정 (전시용이므로 적절한 크기로 설정)
		UnitToUpdate->SetUnitScale(2.5f);

		// ---------------------------------------------------------
		// 중요: 여기서 유닛의 다리/몸통/무기 파츠를 실제로 스폰하고 조립합니다.
		UnitToUpdate->ApplyAssemblyData(UnitData);
		// ---------------------------------------------------------

		// 관리 맵에 저장 (새로 생성된 경우를 위함)
		SpawnedUnits.Add(SlotIndex, UnitToUpdate);

		// [핵심 변경 사항]
		// 유닛 조립(ApplyAssemblyData)이 끝났으므로,
		// 이제 이 슬롯의 카메라에게 방금 조립된 유닛(UnitToUpdate)을 계속 찍으라고 지시합니다.
		// 타이머나 딜레이 없이 여기서 바로 호출하면 매 프레임 실시간으로 캡처를 시작합니다.
		TargetSlot->SetCaptureTarget(UnitToUpdate);
	}
}

void ANovaDeckManager::ClearSlotUnit(int32 SlotIndex)
{
	// 1. 기존 스폰된 프리뷰 유닛 제거
	// 해당 슬롯 인덱스에 매핑된 유닛이 있다면 월드에서 제거하고 맵(Map)에서도 삭제합니다.
	if (TObjectPtr<APreviewUnit>* FoundUnit = SpawnedUnits.Find(SlotIndex))
	{
		if (FoundUnit && *FoundUnit)
		{
			(*FoundUnit)->Destroy();
		}
		SpawnedUnits.Remove(SlotIndex);
	}

	// 2. [수정] 유닛 존재 여부와 상관없이 해당 슬롯 액터를 찾아 카메라(캡처)를 초기화
	// 월드에 배치된 DeckSlot 액터들 중 인덱스가 일치하는 것을 찾습니다.
	for (ANovaDeckSlot* Slot : DeckSlots)
	{
		if (Slot && Slot->GetSlotIndex() == SlotIndex)
		{
			// 타겟을 nullptr로 전달하여 호출합니다.
			// 내부적으로 SceneCapture가 작동하면서 RenderTarget을 투명한 배경으로 밉니다.
			Slot->SetCaptureTarget(nullptr);
			break;
		}
	}
}

