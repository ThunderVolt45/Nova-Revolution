// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/NovaBaseButtonWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Core/NovaAssemblyTypes.h"
#include "Core/NovaBase.h"
#include "Core/NovaGameMode.h"
#include "Core/NovaPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/NovaPlayerController.h"

void UNovaBaseButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (ButtonImage && ButtonIcon)
	{
		ButtonImage->SetBrushFromTexture(ButtonIcon);
	}
}

void UNovaBaseButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ActionButton)
	{
		ActionButton->OnClicked.AddDynamic(this, &UNovaBaseButtonWidget::OnButtonClicked);
	}

	// 1. 데이터 캐싱 시도 (게임 시작 시 한 번)
	TryCacheDeckData();

	// 2. 선택 변경 델리게이트 구독
	if (ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetOwningPlayer()))
	{
		PC->OnSelectionChanged.AddDynamic(this, &UNovaBaseButtonWidget::HandleSelectionChanged);
		HandleSelectionChanged(PC->GetSelectedUnits());
	}
}

void UNovaBaseButtonWidget::OnButtonClicked()
{
	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetOwningPlayer());
	if (!PC) return;

	const TArray<AActor*>& SelectedUnits = PC->GetSelectedUnits();
	if (SelectedUnits.Num() == 0) return;

	ANovaBase* SelectedBase = Cast<ANovaBase>(SelectedUnits[0]);
	if (!SelectedBase) return;

	if (ActionType == EBaseActionType::ProduceUnit)
	{
		// 캐싱된 데이터를 직접 쓰지 않고, 기지에 인덱스만 넘겨 실제 생산 로직을 실행합니다.
		SelectedBase->ProduceUnit(ProductionSlotIndex);
	}
	else if (ActionType == EBaseActionType::SetRallyPoint)
	{
		PC->SetPendingCommandType(ECommandType::Move);
	}
}

void UNovaBaseButtonWidget::HandleSelectionChanged(const TArray<AActor*>& SelectedUnits)
{
	// 가시성 제어
	if (IsBaseSelected(SelectedUnits))
	{
		SetVisibility(ESlateVisibility::Visible);

		// 혹시 NativeConstruct 시점에 캐싱에 실패했다면 여기서 다시 시도
		if (!bIsDataCached) TryCacheDeckData();
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNovaBaseButtonWidget::TryCacheDeckData()
{
	// 이미 캐싱되었거나 유닛 생산 버튼이 아니면 패스
	if (bIsDataCached || ActionType != EBaseActionType::ProduceUnit) return;

	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetOwningPlayer());
	if (!PC) return;

	ANovaPlayerState* PS = PC->GetPlayerState<ANovaPlayerState>();
	ANovaGameMode* GM = Cast<ANovaGameMode>(UGameplayStatics::GetGameMode(this));

	// PS와 GM이 유효할 때 덱 정보를 가져옴 (팀 ID 기반)
	if (PS && GM)
	{
		FNovaDeckInfo DeckInfo = GM->GetPlayerDeck(PS->GetTeamID());

		if (DeckInfo.Units.IsValidIndex(ProductionSlotIndex))
		{
			CachedUnitData = DeckInfo.Units[ProductionSlotIndex];
			bIsDataCached = true;

			// 캐싱된 데이터를 즉시 UI에 반영
			UpdateUIFromCache();
		}
	}
}

void UNovaBaseButtonWidget::UpdateUIFromCache()
{
	if (!bIsDataCached || !ButtonImage) return;
	
	// 현재는 유닛 이름이 있으면 유효한 것으로 보고, 필요한 연출을 수행합니다.
	if (!CachedUnitData.UnitName.IsEmpty())
	{
		// 아이콘 변경 로직 (아이콘 데이터가 추가되면 적용)
		// ButtonImage->SetBrushFromTexture(CachedUnitData.UnitIcon);
	}
}

bool UNovaBaseButtonWidget::IsBaseSelected(const TArray<AActor*>& SelectedUnits) const
{
	if (SelectedUnits.Num() == 0) return false;

	ANovaPlayerState* PS = GetOwningPlayerState<ANovaPlayerState>();
	if (!PS) return false;

	AActor* FirstActor = SelectedUnits[0];
	if (IsValid(FirstActor))
	{
		INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(FirstActor);
		INovaSelectableInterface* SelectableInterface = Cast<INovaSelectableInterface>(FirstActor);

		// 내 팀의 'Base'인지 확인
		return (TeamInterface && TeamInterface->GetTeamID() == PS->GetTeamID() &&
				SelectableInterface && SelectableInterface->GetSelectableType() == ENovaSelectableType::Base);
	}

	return false;
}
