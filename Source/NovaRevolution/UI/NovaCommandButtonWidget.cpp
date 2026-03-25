// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/NovaCommandButtonWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Core/NovaPlayerState.h"
#include "Player/NovaPlayerController.h"

void UNovaCommandButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 에디터 디자인 시점에서도 아이콘이 보이도록 설정
	if (CommandImage && CommandIcon)
	{
		CommandImage->SetBrushFromTexture(CommandIcon);
	}
}

void UNovaCommandButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CommandButton) CommandButton->OnClicked.AddDynamic(this, &UNovaCommandButtonWidget::OnButtonClicked);

	if (ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetOwningPlayer()))
	{
		PC->OnSelectionChanged.AddDynamic(this, &UNovaCommandButtonWidget::HandleSelectionChanged);
		HandleSelectionChanged(PC->GetSelectedUnits());
	}
}

void UNovaCommandButtonWidget::OnButtonClicked()
{
	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetOwningPlayer());
	if (!PC) return;

	if (CommandType == ECommandType::Stop || CommandType == ECommandType::Hold || CommandType == ECommandType::Halt)
	{
		FCommandData CmdData;
		CmdData.CommandType = CommandType;
		PC->IssueCommandToSelectedUnits(CmdData);
		PC->CancelPendingCommand();
	}
	else
	{
		PC->SetPendingCommandType(CommandType);
	}
}

void UNovaCommandButtonWidget::HandleSelectionChanged(const TArray<AActor*>& SelectedUnits)
{
	if (HasControllableUnit(SelectedUnits))
	{
		SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UNovaCommandButtonWidget::HasControllableUnit(const TArray<AActor*>& SelectedUnits) const
{
	// 1. 선택된 대상이 없으면 즉시 false
	if (SelectedUnits.Num() == 0) return false;

	ANovaPlayerState* PS = GetOwningPlayerState<ANovaPlayerState>();
	if (!PS) return false;

	int32 MyTeamID = PS->GetTeamID();

	// 2. 이미 배타적 선택이 보장되어 있으므로, 첫 번째 유닛만 확인합니다.
	AActor* FirstActor = SelectedUnits[0];
	if (IsValid(FirstActor))
	{
		INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(FirstActor);
		// 내 팀인지 확인
		if (TeamInterface && TeamInterface->GetTeamID() == MyTeamID)
		{
			// 명령을 받을 수 있는 인터페이스(유닛 등)를 가졌는지 확인
			if (FirstActor->GetClass()->ImplementsInterface(UNovaCommandInterface::StaticClass()))
			{
				return true;
			}
		}
	}

	// 첫 번째 유닛이 내 것이 아니거나 명령 불가 대상이면
	return false;
}
