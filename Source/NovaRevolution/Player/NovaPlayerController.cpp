// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/NovaPlayerController.h"

#include "Core/NovaInterfaces.h"
#include "GAS/NovaGameplayTags.h"
#include "Input/NovaInputComponent.h"
#include "Core/NovaTypes.h"	// FCommandData, ECommandType 사용을 위해 필요

ANovaPlayerController::ANovaPlayerController()
{
	bShowMouseCursor = true; // 장르가 RTS 이므로 마우스 항상 표시합니다.
	DefaultMouseCursor = EMouseCursor::Default;
}

void ANovaPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ANovaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 만들어둔 커스텀 입력 컴포넌트로 캐스팅 (성공 시 에만 진행)
	if (UNovaInputComponent* NovaInputComponent = CastChecked<UNovaInputComponent>(InputComponent))
	{
		// 데이터 에셋(InputConfig)에
		NovaInputComponent->BindAbilityActions(InputConfig, this,
		                                       &ThisClass::Input_AbilityInputTagPressed,
		                                       &ThisClass::Input_AbilityInputTagReleased,
		                                       &ThisClass::Input_AbilityInputTagHeld);
	}
}

void ANovaPlayerController::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	FHitResult CursorHit;
	GetCursorHitResult(CursorHit); // 마우스 아래의 정보를 CursorHit에 담습니다.

	// 입력된 InputTag를 보고 플레이어의 의도를 파악합니다.
	// 단순히 "키가 눌렸다"가 아니라 "선택을 하려고 눌렀는가(좌클릭), 명령을 내리려고 눌렀는가(우클릭)"를 구분합니다.
	// 좌클릭 (유닛 선택)
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Select))
	{
		// 마우스 아래에 선택 가능한 액터가 있는지 확인합니다.
		AActor* HitActor = CursorHit.GetActor();
		INovaSelectableInterface* NewSelectable = Cast<INovaSelectableInterface>(HitActor);

		// 새로운 선택 가능 액터를 클릭했을 때만 작동
		if (NewSelectable)
		{
			// Shift 키 등을 누르지 않았다면 기존 선택을 모두 해제
			// if (!bIsShiftDown) { ClearSelection(); }
			ClearSelection();	// 일단 단일 선택 모드로 구현
			
			NewSelectable->OnSelected();
			SelectedUnits.Add(HitActor);
		}
		// else : 빈 땅 클릭 시 아무것도 하지 않음.
	}

	// 우클릭 (스마트 명령)
	else if (InputTag.MatchesTag(NovaGameplayTags::Input_Command))
	{
		if (SelectedUnits.Num() > 0)
		{
			FCommandData Command;
			Command.CommandType = ECommandType::Move; // 기본은 이동
			Command.TargetLocation = CursorHit.Location;
			Command.TargetActor = CursorHit.GetActor();

			for (AActor* Unit : SelectedUnits)
			{
				if (INovaCommandInterface* CmdInterface = Cast<INovaCommandInterface>(Unit))
				{
					CmdInterface->IssueCommand(Command);
				}
			}
		}
	}
}

void ANovaPlayerController::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
}

void ANovaPlayerController::Input_AbilityInputTagHeld(FGameplayTag InputTag)
{
}

void ANovaPlayerController::GetCursorHitResult(FHitResult& OutHitResult)
{
	GetHitResultUnderCursor(ECC_Visibility, false, OutHitResult);
}

void ANovaPlayerController::ClearSelection()
{
	for (AActor* Unit : SelectedUnits)
	{
		if (INovaSelectableInterface* Selectable = Cast<INovaSelectableInterface>(Unit))
		{
			Selectable->OnDeselected();
		}
	}
	SelectedUnits.Empty();
}
