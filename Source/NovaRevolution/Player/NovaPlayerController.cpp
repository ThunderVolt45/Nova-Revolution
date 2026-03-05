// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/NovaPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "NovaPawn.h"
#include "NovaRevolution.h"
#include "Blueprint/UserWidget.h"
#include "Core/NovaInterfaces.h"
#include "Core/NovaLog.h"
#include "GAS/NovaGameplayTags.h"
#include "Input/NovaInputComponent.h"
#include "Core/NovaTypes.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "UI/NovaHUD.h"

ANovaPlayerController::ANovaPlayerController()
{
	bShowMouseCursor = true; // 장르가 RTS 이므로 마우스 항상 표시합니다.
	DefaultMouseCursor = EMouseCursor::Default;
}

void ANovaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 입력 모드 설정 (Game : 게임 월드 입력(클릭, 드래그) 지원 | UI : UI 마우스 오버 등 지원)
	FInputModeGameAndUI InputMode;

	// 마우스 가두기 옵션 설정 (LcokAlways 또는 LockInFullScreen)
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);

	// 커서 표시 설정
	InputMode.SetHideCursorDuringCapture(false);;

	// 컨트롤러에 적용
	SetInputMode(InputMode);

	// 1. 로컬 플레이어(내 화면)인지 확인 (UI 생성 및 입력 설정의 필수 조건)
	if (IsLocalPlayerController())
	{
		// 기존 코드 내부로 이동
		// Local Player Subsystem 가져오기
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(
			GetLocalPlayer()))
		{
			// IMC가 에디터에서 할당되었는지 확인 후 추가
			if (IMC)
			{
				// 우선순위 0으로 추가
				Subsystem->AddMappingContext(IMC, 0);
			}
		}

		// --- 새로운 로직: 메인 HUD 생성 및 표시 ---
		// 에디터(BP_NovaPlayerController)에서 MainHUDClass가 할당되었는지 확인
		if (MainHUDClass)
		{
			MainHUDInstance = CreateWidget<UUserWidget>(this, MainHUDClass);
			if (MainHUDInstance)
			{
				MainHUDInstance->AddToViewport();
			}
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
		                               &ThisClass::Input_MoveCamera);
	}
	if (ZoomAction)
	{
		NovaInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this,
		                               &ThisClass::Input_Zoom);
	}
}

void ANovaPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// 마우스 화면 스크롤링 기능
	if (bEnableEdgeScrolling)
	{
		float MouseX, MouseY;
		// 마우스 좌표 얻어오기(APlayerController 함수)
		if (GetMousePosition(MouseX, MouseY))
		{
			int32 ViewportSizeX, ViewportSizeY;
			// Viewport 사이즈 얻어오기(APlayerController 함수)
			GetViewportSize(ViewportSizeX, ViewportSizeY);

			float ForwardInput = 0.f;
			float RightInput = 0.f;

			// 상단/하단 경계 체크 (Y축은 위쪽이 0)
			if (MouseY <= EdgeScrollMargin)
			{
				ForwardInput = 1.f;
			}
			else if (MouseY >= ViewportSizeY - EdgeScrollMargin)
			{
				ForwardInput = -1.f;
			}

			// 좌측/우측 경계 체크
			if (MouseX <= EdgeScrollMargin)
			{
				RightInput = -1.f;
			}
			else if (MouseX >= ViewportSizeX - EdgeScrollMargin)
			{
				RightInput = 1.f;
			}

			// 마우스가 경계이 있다면 카메라 이동 적용
			if (ForwardInput != 0.f || RightInput != 0.f)
			{
				ApplyCameraMovement(ForwardInput, RightInput);
			}
		}
	}
}

void ANovaPlayerController::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	FHitResult CursorHit;
	GetCursorHitResult(CursorHit); // 마우스 아래의 정보를 CursorHit에 담습니다.

	// [추가] Shift 키를 누르는 순간 다중 선택 모드 활성화!
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Modifier_Select))
	{
		bIsShiftDown = true;
	}

	// 마우스 좌클릭 (선택)
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Select))
	{
		// 1. 현재 마우스 좌표를 시작점으로 저장
		GetMousePosition(DragStartPos.X, DragStartPos.Y);
		bIsDraggingBox = false; // 드래그 상태 초기화
	}

	// 마우스 우클릭 (스마트 명령)
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Command))
	{
		// 선택된 명령이 있다면 우클릭 시 취소
		if (PendingCommandType != ECommandType::None)
		{
			CancelPendingCommand();
			NOVA_SCREEN(Warning, "Pending Command Canceled.");
		}
		// 그렇지 않다면 스마트 명령 (이동 또는 공격)
		else if (SelectedUnits.Num() > 0)
		{
			// 지정 대상에 대한 정보 전달 (Location 또는 Actor)
			FCommandData Command;
			Command.CommandType = ECommandType::Move; // 기본은 이동
			Command.TargetLocation = CursorHit.Location;
			Command.TargetActor = CursorHit.GetActor();

			// 선택된 유닛들에게 명령 전달
			IssueCommandToSelectedUnits(Command);
		}
		return;
	}

	// 누르는 즉시 실행되는 명령 : Stop(S), Hold(H), Spread(C), Halt(L)
	ECommandType ImmediateCmd = ECommandType::None;
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Stop)) ImmediateCmd = ECommandType::Stop;
	else if (InputTag.MatchesTag(NovaGameplayTags::Input_Hold)) ImmediateCmd = ECommandType::Hold;
	else if (InputTag.MatchesTag(NovaGameplayTags::Input_Spread)) ImmediateCmd = ECommandType::Spread;
	else if (InputTag.MatchesTag(NovaGameplayTags::Input_Halt)) ImmediateCmd = ECommandType::Halt;
	if (ImmediateCmd != ECommandType::None && SelectedUnits.Num() > 0)
	{
		FCommandData CmdData;
		CmdData.CommandType = ImmediateCmd;
		IssueCommandToSelectedUnits(CmdData);

		// TODO: 나중에 UI 작업 (커서 모양 변경 등)
		NOVA_SCREEN(Warning, "Command Executed: %d", (int32)ImmediateCmd);
		// 다른 대기 중인 명령이 있었다면 취소
		CancelPendingCommand();
		// 명령 실행 후 즉시 return
		return;
	}

	// 지정 대상/지점이 필요한 명령 : Attack(A), Patrol(P), Move(M) 
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Attack))
	{
		PendingCommandType = ECommandType::Attack;
		// TODO: 나중에 UI 작업 (커서 모양 변경 등)
		NOVA_SCREEN(Warning, "Command: Attack(A). Click to Execute.");
	}
	else if (InputTag.MatchesTag(NovaGameplayTags::Input_Patrol))
	{
		PendingCommandType = ECommandType::Patrol;
		// TODO: 나중에 UI 작업 (커서 모양 변경 등)
		NOVA_SCREEN(Warning, "Command: Patrol(P). Click to Execute.");
	}
	else if (InputTag.MatchesTag(NovaGameplayTags::Input_Move))
	{
		PendingCommandType = ECommandType::Move;
		// TODO: 나중에 UI 작업 (커서 모양 변경 등)
		NOVA_SCREEN(Warning, "Command: Move(M). Click to Execute.");
	}
}

void ANovaPlayerController::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	// 명령 수행
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Select))
	{
		// 대기 중인 명령이 있다면 수행
		if (PendingCommandType != ECommandType::None && SelectedUnits.Num() > 0)
		{
			FHitResult CursorHit;
			GetCursorHitResult(CursorHit);

			FCommandData CmdData;
			CmdData.CommandType = PendingCommandType;
			CmdData.TargetLocation = CursorHit.Location;
			CmdData.TargetActor = CursorHit.GetActor();

			IssueCommandToSelectedUnits(CmdData);

			// 명령 수행 후 초기화
			CancelPendingCommand();
			return;
		}
	}

	// Shift 키를 떼는 순간 다중 선택 모드 해제!
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Modifier_Select))
	{
		bIsShiftDown = false;
	}

	// 마우스를 뗄 때 선택
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Select))
	{
		// 마우스를 떼는 순간 HUD의 드래그 박스 그리기를 중단
		if (ANovaHUD* NovaHUD = Cast<ANovaHUD>(GetHUD()))
		{
			NovaHUD->UpdateDragRect(FVector2D::ZeroVector, FVector2D::ZeroVector, false);
		}

		if (bIsDraggingBox)
		{
			// 드래그 선택 수행
			PerformBoxSelection();
			bIsDraggingBox = false;
		}
		else
		{
			FHitResult CursorHit;
			GetCursorHitResult(CursorHit);
			INovaSelectableInterface* NewSelectable = Cast<INovaSelectableInterface>(CursorHit.GetActor());
			// 단일 선택 수행 (단순 클릭)
			if (NewSelectable)
			{
				// Shift가 눌려있다면
				if (bIsShiftDown)
				{
					// 이미 선택 되어 있다면
					if (SelectedUnits.Contains(CursorHit.GetActor()))
					{
						// 선택 해제
						NewSelectable->OnDeselected();
						SelectedUnits.Remove(CursorHit.GetActor());
					}
					else
					{
						// 추가
						NewSelectable->OnSelected();
						SelectedUnits.Add(CursorHit.GetActor());
					}
				}
				// Shift가 눌려있지 않다면
				else
				{
					ClearSelection();
					NewSelectable->OnSelected();
					SelectedUnits.Add(CursorHit.GetActor());
				}
			}
		}
		// TODO: 나중에 여기에 HUD->DragSelectUpdate(..., false) 호출을 추가합니다.
	}

	// [확인용 로그] 어떤 태그가 들어오는지, 현재 몇 마리가 선택되어 있는지 출력
	UE_LOG(LogTemp, Warning, TEXT("Tag Pressed: %s | 현재 선택된 유닛 수: %d"),
	       *InputTag.ToString(), SelectedUnits.Num());
}

void ANovaPlayerController::Input_AbilityInputTagHeld(FGameplayTag InputTag)
{
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Select))
	{
		FVector2D CurrentPosition;
		GetMousePosition(CurrentPosition.X, CurrentPosition.Y);

		// 시작점과 현재점의 거리가 일정 이상(예: 5픽셀)이면 드래그로 간주
		if (FVector2D::Distance(DragStartPos, CurrentPosition) > 5.f)
		{
			bIsDraggingBox = true;
			// TODO: 나중에 여기에 HUD->DragSelectUpdate(...) 호출을 추가합니다.
			if (ANovaHUD* NovaHUD = Cast<ANovaHUD>(GetHUD()))
			{
				NovaHUD->UpdateDragRect(DragStartPos, CurrentPosition, true);
			}
		}
	}
}

void ANovaPlayerController::Input_MoveCamera(const FInputActionValue& Value)
{
	// 입력 값 가져오기
	FVector2D InputVector = Value.Get<FVector2D>();

	// 공통 함수로 입력을 넘김. (Y는 Forward, X는 Right)
	ApplyCameraMovement(InputVector.Y, InputVector.X);
}

void ANovaPlayerController::ApplyCameraMovement(float ForwardInput, float RightInput)
{
	APawn* ControlledPawn = GetPawn();

	if (ControlledPawn)
	{
		// 카메라의 현재 회전 방향을 기준으로 앞/옆 방향 계산
		const FRotator Rotation = GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		// 앞/뒤 이동 벡터 계산
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		// 좌/우 이동 벡터 계산
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Pawn에게 이동 명령 전달 (속도는 Pawn의 MovementComponent에서 조절)
		ControlledPawn->AddMovementInput(ForwardDirection, ForwardInput);
		ControlledPawn->AddMovementInput(RightDirection, RightInput);
	}
}

void ANovaPlayerController::Input_Zoom(const FInputActionValue& Value)
{
	float ZoomValue = Value.Get<float>(); // 휠을 위로 +1, 아래로 -1

	// 조종 중인 Pawn을 가져와서 UpdateZoom 호출
	if (ANovaPawn* NovaPawn = Cast<ANovaPawn>(GetPawn()))
	{
		NovaPawn->UpdateZoom(ZoomValue);
	}
}

void ANovaPlayerController::PerformBoxSelection()
{
	FVector2D EndPosition;
	GetMousePosition(EndPosition.X, EndPosition.Y);

	// 드래그 사각형의 최소/최대 좌표 계산 (산술 비교용)
	float MinX = FMath::Min(DragStartPos.X, EndPosition.X);
	float MaxX = FMath::Max(DragStartPos.X, EndPosition.X);
	float MinY = FMath::Min(DragStartPos.Y, EndPosition.Y);
	float MaxY = FMath::Max(DragStartPos.Y, EndPosition.Y);

	// 선택 가능한 모든 액터 가져오기 (인터페이스 기반)
	TArray<AActor*> AllSelectableActors;
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UNovaSelectableInterface::StaticClass(),
	                                            AllSelectableActors);

	TArray<AActor*> ActorsInRect;

	for (AActor* Actor : AllSelectableActors)
	{
		if (Actor)
		{
			// 유닛의 3D 위치를 화면의 2D 좌표로 투영
			FVector2D ScreenPos;
			if (ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenPos))
			{
				// 사각형 영역 안에 들어와 있는지 체크
				if (ScreenPos.X >= MinX && ScreenPos.X <= MaxX &&
					ScreenPos.Y >= MinY && ScreenPos.Y <= MaxY)
				{
					ActorsInRect.Add(Actor);
				}
			}
		}
	}

	// 5. 최종 선택 처리 (Shift 상태 반영 가능)
	if (ActorsInRect.Num() > 0)
	{
		if (!bIsShiftDown) { ClearSelection(); }

		for (AActor* Unit : ActorsInRect)
		{
			if (INovaSelectableInterface* Selectable = Cast<INovaSelectableInterface>(Unit))
			{
				Selectable->OnSelected();
				SelectedUnits.AddUnique(Unit); // 중복 방지
			}
		}
	}
}

// 유닛들에게 명령 전송
void ANovaPlayerController::IssueCommandToSelectedUnits(const FCommandData& CommandData)
{
	for (AActor* Unit : SelectedUnits)
	{
		if (INovaCommandInterface* CmdInterface = Cast<INovaCommandInterface>(Unit))
		{
			CmdInterface->IssueCommand(CommandData);
		}
	}
}

void ANovaPlayerController::CancelPendingCommand()
{
	PendingCommandType = ECommandType::None;
	// TODO: 나중에 커서 모양 복구
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
