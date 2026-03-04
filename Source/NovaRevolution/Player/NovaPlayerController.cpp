// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/NovaPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Core/NovaInterfaces.h"
#include "GAS/NovaGameplayTags.h"
#include "Input/NovaInputComponent.h"
#include "Core/NovaTypes.h"

ANovaPlayerController::ANovaPlayerController()
{
	bShowMouseCursor = true; // 장르가 RTS 이므로 마우스 항상 표시합니다.
	DefaultMouseCursor = EMouseCursor::Default;
}

void ANovaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Local Player Subsystem 가져오기
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
		GetLocalPlayer()))
	{
		// IMC가 에디터에서 할당되었는지 확인 후 추가
		if (IMC)
		{
			// 우선순위 0으로 추가
			Subsystem->AddMappingContext(IMC, 0);
		}
	}
}

void ANovaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 만들어둔 커스텀 입력 컴포넌트로 캐스팅 (성공 시 에만 진행)
	UNovaInputComponent* NovaInputComponent = CastChecked<UNovaInputComponent>(InputComponent);
	// 태그 기반 바인딩 (유닛 조작용)
	NovaInputComponent->BindAbilityActions(InputConfig, this,
	                                       &ThisClass::Input_AbilityInputTagPressed,
	                                       &ThisClass::Input_AbilityInputTagReleased,
	                                       &ThisClass::Input_AbilityInputTagHeld);

	// 일반 액션 바인딩 (카메라 이동용)
	if (MoveCameraAction)
	{
		NovaInputComponent->BindAction(MoveCameraAction, ETriggerEvent::Triggered, this,
		                               &ANovaPlayerController::MoveCamera);
	}
}

void ANovaPlayerController::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	// [확인용 로그] 어떤 태그가 들어오는지, 현재 몇 마리가 선택되어 있는지 출력
	UE_LOG(LogTemp, Warning, TEXT("Tag Pressed: %s | 현재 선택된 유닛 수: %d"),
	       *InputTag.ToString(), SelectedUnits.Num());

	FHitResult CursorHit;
	GetCursorHitResult(CursorHit); // 마우스 아래의 정보를 CursorHit에 담습니다.

	// 입력된 InputTag를 보고 플레이어의 의도를 파악합니다.
	// 좌클릭 (유닛 선택) - 새로운 대상이 있을 때만 교체
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Select))
	{
		// 새로운 선택 가능 액터를 클릭했을 때만 작동
		if (INovaSelectableInterface* NewSelectable = Cast<INovaSelectableInterface>(CursorHit.GetActor()))
		{
			// Shift 키 등을 누르지 않았다면 기존 선택을 모두 해제
			// if (!bIsShiftDown) { ClearSelection(); }
			ClearSelection(); // 일단 단일 선택 모드로 구현

			NewSelectable->OnSelected();
			SelectedUnits.Add(CursorHit.GetActor());
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

void ANovaPlayerController::MoveCamera(const FInputActionValue& Value)
{
	// 입력 값 가져오기
	FVector2D InputVector = Value.Get<FVector2D>();

	// 조종 중인 Pawn 가져오기
	APawn* ControlledPawn = GetPawn();

	if (ControlledPawn && (InputVector.SizeSquared() > 0.f))
	{
		// 카메라의 현재 회전 방향을 기준으로 앞/옆 방향 계산
		const FRotator Rotation = GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		// 앞/뒤 이동 벡터 계산
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		// 좌/우 이동 벡터 계산
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Pawn에게 이동 명령 전달 (속도는 Pawn의 MovementComponent에서 조절)
		ControlledPawn->AddMovementInput(ForwardDirection, InputVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, InputVector.X);
	}
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
