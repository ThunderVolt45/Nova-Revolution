// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/NovaPlayerController.h"

#include "AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NiagaraFunctionLibrary.h"
#include "NovaPawn.h"
#include "NovaRevolution.h"
#include "Blueprint/UserWidget.h"
#include "Core/NovaBase.h"
#include "Core/NovaInterfaces.h"
#include "Core/NovaLog.h"
#include "Core/NovaPlayerState.h"
#include "GAS/NovaGameplayTags.h"
#include "Input/NovaInputComponent.h"
#include "Core/NovaTypes.h"
#include "Core/AI/NovaAIController.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "UI/NovaHUD.h"

ANovaPlayerController::ANovaPlayerController()
{
	// bShowMouseCursor = true; // 장르가 RTS 이므로 마우스 항상 표시합니다.
	// DefaultMouseCursor = EMouseCursor::Default;
}

void ANovaPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	
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

	// 10개의 빈 부대 슬롯 미리 생성
	ControlGroups.SetNum(10);
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

	// 조합키 활성화
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Modifier_Shift)) bIsShiftDown = true;
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Modifier_Ctrl)) bIsCtrlDown = true;
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Modifier_Alt)) bIsAltDown = true;
	
	
	// 사령관 Skill 관련 로직 추가 : 스킬 타겟팅 모드인 경우 입력을 가로채서 GAS에만 전달하고 함수를 끝냅니다.
	if (PendingCommandType == ECommandType::Skill)
	{
		if (ANovaPlayerState* PS = GetPlayerState<ANovaPlayerState>())
		{
			UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
			if (ASC)
			{
				// 좌클릭 -> Confirm
				if (InputTag.MatchesTag(NovaGameplayTags::Input_Select))
				{
					NOVA_LOG(Log, "Input_Select detected during Skill mode. Calling LocalInputConfirm for PC: %s", *GetName());
					
					ASC->LocalInputConfirm();
					return; // 기존의 드래그/선택 로직을 실행하지 않고 나갑니다.
				}
				// 우클릭 -> Cancel
				else if (InputTag.MatchesTag(NovaGameplayTags::Input_Command))
				{
					NOVA_LOG(Warning, "Input_Command detected during Skill mode. Calling LocalInputCancel for PC: %s", *GetName());
					
					ASC->LocalInputCancel();
					return; // 기존의 이동 명령을 실행하지 않고 나갑니다.
				}
			}
		}
	}
	

	if (InputTag.MatchesTag(NovaGameplayTags::Input_Toggle_HealthBar))
	{
		ToggleHealthBar(InputTag);
		return;
	}
	
	// 명령 차단 로직 (아군 유닛이 아닐 때)
	if (InputTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Input"))))
	{
		// 선택 (Input.Select)이나 카메라 리셋(Input.Camera.Reset) 등 기본 조작은 제외
		if (!InputTag.MatchesTag(NovaGameplayTags::Input_Select) &&
			!InputTag.MatchesTag(NovaGameplayTags::Input_Camera_Reset))
		{
			int LocalTeamID = GetPlayerState<ANovaPlayerState>() ? GetPlayerState<ANovaPlayerState>()->GetTeamID() : -1;

			bool bContainsNonMyUnit = false;
			for (AActor* Unit : SelectedUnits)
			{
				INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(Unit);
				if (!TeamInterface || TeamInterface->GetTeamID() != LocalTeamID)
				{
					bContainsNonMyUnit = true;
					break;
				}
			}

			// 적군이 하나라도 선택되어 있다면 모든 명령 단축키 (A, S, H...) 무시
			if (bContainsNonMyUnit)
			{
				return;
			}
		}
	}

	// 슬롯(Slot 1~0) 공통 처리
	if (InputTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Input.Slot"))))
	{
		int32 SlotIndex = -1;
		// 태그별 슬롯 인덱스 매핑
		if (InputTag == NovaGameplayTags::Input_Slot_1) SlotIndex = 0;
		else if (InputTag == NovaGameplayTags::Input_Slot_2) SlotIndex = 1;
		else if (InputTag == NovaGameplayTags::Input_Slot_3) SlotIndex = 2;
		else if (InputTag == NovaGameplayTags::Input_Slot_4) SlotIndex = 3;
		else if (InputTag == NovaGameplayTags::Input_Slot_5) SlotIndex = 4;
		else if (InputTag == NovaGameplayTags::Input_Slot_6) SlotIndex = 5;
		else if (InputTag == NovaGameplayTags::Input_Slot_7) SlotIndex = 6;
		else if (InputTag == NovaGameplayTags::Input_Slot_8) SlotIndex = 7;
		else if (InputTag == NovaGameplayTags::Input_Slot_9) SlotIndex = 8;
		else if (InputTag == NovaGameplayTags::Input_Slot_0) SlotIndex = 9;

		if (SlotIndex != -1)
		{
			// 부대 지정 (Ctrl + Slot) 
			if (bIsCtrlDown)
			{
				// 현재 선택된 유닛들 중에서 내 팀 유닛이 아닌 것이 있는지 확인
				int32 LocalTeamID = GetPlayerState<ANovaPlayerState>()->GetTeamID();
				bool bContainsNonMyUnit = false;

				for (AActor* Unit : SelectedUnits)
				{
					INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(Unit);
					if (!TeamInterface || TeamInterface->GetTeamID() != LocalTeamID)
					{
						bContainsNonMyUnit = true;
						break;
					}
				}

				// 만약 내 유닛이 아닌 것이 섞여 있거나 선택된 유닛이 아예 없다면?
				if (bContainsNonMyUnit || SelectedUnits.Num() == 0)
				{
					// 기존 부대 지정을 날리지 않고 그냥 무시합니다.
					NOVA_SCREEN(Error, "Control Group %d Assignment Failed: Contains non-friendly units.",
					            SlotIndex + 1);
					return;
				}

				// 3. 순수하게 내 유닛들만 선택된 경우에만 부대 지정 수행
				ControlGroups[SlotIndex].Targets = SelectedUnits;

				// 유저가 직접 부대 지정 했다면 자동 편입 기능 false
				ControlGroups[SlotIndex].bIsAutoAssignActive = false;

				NOVA_SCREEN(Warning, "Control Group %d Assigned (%d units)", SlotIndex + 1, SelectedUnits.Num());

				return;
			}

			// 즉시 유닛 생산 (Shift + Slot)
			if (bIsShiftDown)
			{
				if (ANovaPlayerState* PS = GetPlayerState<ANovaPlayerState>())
				{
					if (ANovaBase* PlayerBase = PS->GetPlayerBase())
					{
						PlayerBase->ProduceUnit(SlotIndex);
						NOVA_SCREEN(Warning, "Request Production : Slot %d", SlotIndex + 1);
						return;
					}
					else { NOVA_SCREEN(Error, "Base is null!"); }
				}
				return;
			}

			// 즉시 스킬 시전 (Alt + Slot)
			if (bIsAltDown)
			{
				// TODO: 스킬 관련 Interface 추가 시 작업할 공간

				// 현재 구현된 스킬 인터페이스가 없음. 로그만 출력
				NOVA_SCREEN(Warning, "Request Commander Skill: Slot %d (Not Implemented Yet)", SlotIndex + 1);

				return;
			}

			// 부대 호출
			if (ControlGroups[SlotIndex].Targets.Num() > 0)
			{
				// 부대 지정된 유닛들을 Array에 포함 시킴 
				TArray<AActor*> TargetActors;
				for (const TObjectPtr<AActor>& ActorPtr : ControlGroups[SlotIndex].Targets)
				{
					if (ActorPtr) TargetActors.Add(ActorPtr.Get());
				}

				// 선택 및 카메라 포커스 로직 실행
				HandleFocusAndSelection(TargetActors, SlotIndex);
				NOVA_SCREEN(Warning, "Control Group %d Selected", SlotIndex + 1);
			}
			return;
		}
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
			FCommandData CmdData;
			CmdData.CommandType = ECommandType::Move;
			CmdData.TargetLocation = CursorHit.Location;

			AActor* HitActor = CursorHit.GetActor();
			if (HitActor)
			{
				if (HitActor->GetClass()->ImplementsInterface(UNovaSelectableInterface::StaticClass()))
				{
					CmdData.TargetActor = HitActor;
				}
				else
				{
					// 지형이나 일반 오브젝트인 경우 Actor는 무시하고 위치만 전달
					CmdData.TargetActor = nullptr;
				}
			}

			// 선택된 유닛들에게 명령 전달
			IssueCommandToSelectedUnits(CmdData);
		}
		return;
	}

	// 기지 선택(B)
	if (InputTag.MatchesTag(NovaGameplayTags::Input_SelectBase))
	{
		if (ANovaPlayerState* PS = GetPlayerState<ANovaPlayerState>())
		{
			if (ANovaBase* PlayerBase = PS->GetPlayerBase())
			{
				// Base의 ID는 10으로 지정
				HandleFocusAndSelection({PlayerBase}, 10);
				NOVA_SCREEN(Warning, "Command: Base Selected");
			}
			else
			{
				NOVA_SCREEN(Error, "Base is Null!");
			}
		}
		return;
	}

	// 카메라 Reset
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Camera_Reset))
	{
		if (ANovaPawn* NovaPawn = Cast<ANovaPawn>(GetPawn()))
		{
			NovaPawn->ResetCamera();
		}
	}

	// 누르는 즉시 실행되는 명령 : Stop(S), Hold(H), Halt(L)
	ECommandType ImmediateCmd = ECommandType::None;
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Stop)) ImmediateCmd = ECommandType::Stop;
	else if (InputTag.MatchesTag(NovaGameplayTags::Input_Hold)) ImmediateCmd = ECommandType::Hold;
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

	// 지정 대상/지점이 필요한 명령 : Attack(A), Patrol(P), Move(M), Spread(C)
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
	else if (InputTag.MatchesTag(NovaGameplayTags::Input_Spread))
	{
		PendingCommandType = ECommandType::Spread;
		// TODO: 나중에 UI 작업 (커서 모양 변경 등)
		NOVA_SCREEN(Warning, "Command: Spread(C). Click to Execute.");
	}
}

void ANovaPlayerController::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	// 드래그 박스 제거 우선 실행
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Select))
	{
		// 마우스를 떼는 순간 HUD의 드래그 박스 그리기를 중단
		if (ANovaHUD* NovaHUD = Cast<ANovaHUD>(GetHUD()))
		{
			NovaHUD->UpdateDragRect(FVector2D::ZeroVector, FVector2D::ZeroVector, false);
		}
	}

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

			// TargetActor가 실제로 유닛(Unit)이나 기지(Base)인지 검사
			AActor* HitActor = CursorHit.GetActor();
			if (HitActor)
			{
				// 인터페이스를 가지고 있거나 특정 클래스(Unit/Base)인 경우메만 TargetActor로 인정
				if (HitActor->GetClass()->ImplementsInterface(UNovaSelectableInterface::StaticClass()))
				{
					CmdData.TargetActor = HitActor;
				}
				else
				{
					CmdData.TargetActor = nullptr;
				}
			}

			IssueCommandToSelectedUnits(CmdData);

			// 명령 수행 후 초기화
			CancelPendingCommand();
			bIsDraggingBox = false; // 드래그 상태 초기화
			return;
		}
	}

	// 조합키 비활성화
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Modifier_Shift)) bIsShiftDown = false;
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Modifier_Ctrl)) bIsCtrlDown = false;
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Modifier_Alt)) bIsAltDown = false;

	// 마우스를 뗄 때 선택
	if (InputTag.MatchesTag(NovaGameplayTags::Input_Select))
	{
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
				AActor* HitActor = CursorHit.GetActor();
				int32 LocalTeamID = GetPlayerState<ANovaPlayerState>()->GetTeamID();

				// 유닛의 팀 확인
				INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(HitActor);
				bool bIsMyUnit = (TeamInterface && TeamInterface->GetTeamID() == LocalTeamID);

				// 현재 이미 선택된 유닛들 중 내 유닛이 아닌 것이 있는지 확인
				bool bAlreadyHasNonMyUnit = false;
				for (AActor* Unit : SelectedUnits)
				{
					INovaTeamInterface* SelectedTeam = Cast<INovaTeamInterface>(Unit);
					if (!SelectedTeam || SelectedTeam->GetTeamID() != LocalTeamID)
					{
						bAlreadyHasNonMyUnit = true;
						break;
					}
				}

				// 클릭한 대상이 내 것이 아니거나, 이미 내 것이 아닌 게 선택되어 있다면 -> 무조건 단일 선택
				if (!bIsMyUnit || bAlreadyHasNonMyUnit)
				{
					// 단, Shift 클릭으로 내 유닛을 추가하려는 경우에도
					// 기존에 적군이 있었다면 적군을 지우고 내 유닛 '하나'만 선택되도록 처리
					ClearSelection();
					NewSelectable->OnSelected();
					SelectedUnits.Add(HitActor);
				}
				else
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
						SelectedUnits.Add(HitActor);
					}
					}

					// 선택 변경 알림
					OnSelectionChanged.Broadcast(SelectedUnits);
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
	// [버그 수정] 대기 중인 명령이 있는 상태에서는 드래그 박스를 그리지 않음
	if (PendingCommandType != ECommandType::None) return;

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
	int32 LocalTeamID = GetPlayerState<ANovaPlayerState>() ? GetPlayerState<ANovaPlayerState>()->GetTeamID() : -1;

	// 1차 필터링: 사각형 안의 모든 선택 가능한 Actor 수집
	for (AActor* Actor : AllSelectableActors)
	{
		if (Actor)
		{
			// 유닛이 현재 선택 가능한 상태(안개 밖 등)인지 먼저 확인
			if (INovaSelectableInterface* Selectable = Cast<INovaSelectableInterface>(Actor))
			{
				if (!Selectable->IsSelectable()) continue;
			}

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

	// 2차 필터링: 스마트 선택 (아군 vs 적군, 유닛 vs 건물)
	if (ActorsInRect.Num() > 0)
	{
		TArray<AActor*> MyUnits; // 내 유닛들
		TArray<AActor*> MyBases; // 내 기지들
		TArray<AActor*> EnemyActors; // 적군/중립 액터들

		// 수집된 액터들을 종류별로 분류
		for (AActor* Actor : ActorsInRect)
		{
			INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(Actor);
			INovaSelectableInterface* Selectable = Cast<INovaSelectableInterface>(Actor);

			bool bIsMyTeam = (TeamInterface && TeamInterface->GetTeamID() == LocalTeamID);
			bool bIsUnit = (Selectable && Selectable->GetSelectableType() == ENovaSelectableType::Unit);

			if (bIsMyTeam)
			{
				if (bIsUnit) MyUnits.Add(Actor);
				else MyBases.Add(Actor);
			}
			else
			{
				EnemyActors.Add(Actor);
			}
		}

		// --- 최종 선택 로직 (우선순위 결정) ---

		// [우선순위 1] 내 유닛이 있다면 -> 내 유닛들만 전부 선택
		if (MyUnits.Num() > 0)
		{
			// 현재 선택된 유닛들 중 내 팀이 아닌 것이 있는지 미리 확인
			bool bAlreadyHasNonMyUnit = false;
			for (AActor* Unit : SelectedUnits)
			{
				INovaTeamInterface* SelectedTeam = Cast<INovaTeamInterface>(Unit);
				if (!SelectedTeam || SelectedTeam->GetTeamID() != LocalTeamID)
				{
					bAlreadyHasNonMyUnit = true;
					break;
				}
			}

			if (!bIsShiftDown || bAlreadyHasNonMyUnit) ClearSelection();
			for (AActor* Unit : MyUnits)
			{
				if (INovaSelectableInterface* Selectable = Cast<INovaSelectableInterface>(Unit))
				{
					Selectable->OnSelected();
					SelectedUnits.AddUnique(Unit);
				}
			}
		}
		// [우선순위 2] 내 유닛은 없고 내 기지만 있다면 -> 기지만 선택 (단일 또는 다중 선택 여부는 기획에 따라 다름)
		else if (MyBases.Num() > 0)
		{
			if (!bIsShiftDown) ClearSelection();
			// 보통 기지는 한 번에 여러 개 선택하지 않으므로 첫 번째 하나만 선택하거나, 모두 선택할 수 있습니다.
			// 여기서는 모두 선택하도록 구현합니다 (다중 기지 생산 대비)
			for (AActor* Base : MyBases)
			{
				if (INovaSelectableInterface* Selectable = Cast<INovaSelectableInterface>(Base))
				{
					Selectable->OnSelected();
					SelectedUnits.AddUnique(Base);
				}
			}
		}
		// [우선순위 3] 내 것은 아무것도 없고 적군만 있다면 -> **가장 첫 번째 하나만 선택 (중요!)**
		else if (EnemyActors.Num() > 0)
		{
			// 적군은 드래그하더라도 무조건 기존 선택 지우고 단일 선택으로 고정
			ClearSelection();
			AActor* TargetEnemy = EnemyActors[0];
			if (INovaSelectableInterface* Selectable = Cast<INovaSelectableInterface>(TargetEnemy))
			{
				Selectable->OnSelected();
				SelectedUnits.Add(TargetEnemy);
			}
		}

		// 선택 변경 알림
		OnSelectionChanged.Broadcast(SelectedUnits);
	}
}

// 유닛들에게 명령 전송
void ANovaPlayerController::IssueCommandToSelectedUnits(const FCommandData& CommandData)
{
	// 현재 로컬 플레이어의 팀 ID 가져오기
	int32 LocalTeamID = GetPlayerState<ANovaPlayerState>() ? GetPlayerState<ANovaPlayerState>()->GetTeamID() : -1;

	if (CommandData.CommandType == ECommandType::Move)
	{
		SpawnCommandVisualEffect(CommandData.TargetLocation, CommandData.CommandType, CommandData.TargetActor.Get());
	}
	for (AActor* Unit : SelectedUnits)
	{
		// 내 팀 유닛이 아니라면 명령 전송 무시
		INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(Unit);
		if (!TeamInterface || TeamInterface->GetTeamID() != LocalTeamID)
		{
			continue;
		}
		// 자기 자신을 공격하는 명령이라면 명령을 전달하지 않음
		if (CommandData.CommandType == ECommandType::Attack)
		{
			if (CommandData.TargetActor == Unit)
			{
				NOVA_SCREEN(Warning, "Self-attack command ignored for unit: %s", *Unit->GetName());
				continue;
			}
		}

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

// 단축키 입력 시 지정된 유닛 선택 및 카메라 이동 로직
void ANovaPlayerController::HandleFocusAndSelection(const TArray<AActor*>& TargetActors, int32 FocusID)
{
	float CurrentTime = GetWorld()->GetTimeSeconds();

	// 입력 일치 여부 && 연타 여부 확인
	bool bIsDoubleClick = (FocusID == LastFocusID) && (CurrentTime - LastFocusTime < DoubleClickThreshold);

	// 기존 선택 초기화
	ClearSelection();

	FVector AverageLocation = FVector::ZeroVector;
	int32 ValidCount = 0;

	// 대상 유닛들 선택 처리 및 중심점 계산
	for (AActor* Actor : TargetActors)
	{
		if (IsValid(Actor))
		{
			if (INovaSelectableInterface* Selectable = Cast<INovaSelectableInterface>(Actor))
			{
				Selectable->OnSelected();
				SelectedUnits.Add(Actor);

				AverageLocation += Actor->GetActorLocation();
				ValidCount++;
			}
		}
	}

	// 연타 시 카메라 이동
	if (bIsDoubleClick && ValidCount > 0)
	{
		AverageLocation /= (float)ValidCount; // 유닛들의 중심점
		if (APawn* ControlledPawn = GetPawn())
		{
			FVector NewCameraLocation = AverageLocation;
			NewCameraLocation.Z = ControlledPawn->GetActorLocation().Z; // Z값은 현재 카메라 높이를 유지
			ControlledPawn->SetActorLocation(NewCameraLocation);
			// NOVA_SCREEN(Warning, "Camera Move to Selected Targets");
		}
	}

	// 포커스 정보 갱신
	LastFocusID = FocusID;
	LastFocusTime = CurrentTime;

	// 선택 변경 알림
	OnSelectionChanged.Broadcast(SelectedUnits);
}

void ANovaPlayerController::ToggleHealthBar(FGameplayTag InputTag)
{
	// 상태 토글
	bShowHealthBars = !bShowHealthBars;

	// 체력바 Toggle방법을 델리게이트 BroadCast로 리팩토링
	if (OnShowHealthBarsChanged.IsBound())
	{
		OnShowHealthBarsChanged.Broadcast(bShowHealthBars);
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
	OnSelectionChanged.Broadcast(SelectedUnits);
}

// 생성된 유닛 자동 부대 편입
void ANovaPlayerController::OnUnitProduced(AActor* Unit, int32 SlotIndex)
{
	// 해당 유닛이 없거나 지정할 부대가 존재하지 않을 시 return
	if (!IsValid(Unit) || !ControlGroups.IsValidIndex(SlotIndex)) return;

	// 자동 편입 기능 사용 여부 확인
	if (ControlGroups[SlotIndex].bIsAutoAssignActive)
	{
		// 부대 지정 배열에 유닛 추가
		ControlGroups[SlotIndex].Targets.AddUnique(Unit);
		NOVA_SCREEN(Log, "Unit Auto-Assigned to Group %d", SlotIndex + 1);
	}
}

void ANovaPlayerController::NotifyTargetUnselectable(AActor* SelectedTargets)
{
	if (!SelectedTargets) return;

	// 현재 선택된 유닛 리스트에서 해당 유닛을 찾아 제거
	if (SelectedUnits.Contains(SelectedTargets))
	{
		if (INovaSelectableInterface* Selectable = Cast<INovaSelectableInterface>(SelectedTargets))
		{
			// 선택 해제 실행
			Selectable->OnDeselected();
		}
		SelectedUnits.Remove(SelectedTargets);

		// 선택 변경 알림
		OnSelectionChanged.Broadcast(SelectedUnits);
	}

	// 부대 지정(ControlGroups) 리스트에서도 제거
	for (int32 i = 0; i < ControlGroups.Num(); ++i)
	{
		if (ControlGroups[i].Targets.Contains(SelectedTargets))
		{
			ControlGroups[i].Targets.Remove(SelectedTargets);
		}
	}
}

void ANovaPlayerController::SpawnCommandVisualEffect(const FVector& Loc, ECommandType CommandType, AActor* TargetActor)
{
	UNiagaraSystem* EffectToSpawn = nullptr;

	// 명령 타입에 따라 이펙트 결정
	switch (CommandType)
	{
	case ECommandType::Move:
		if (!TargetActor)
		{
			EffectToSpawn = MoveCommandEffect;
		}
		break;
	case ECommandType::Attack:
		EffectToSpawn = AttackCommandEffect;
		break;
	default:
		EffectToSpawn = MoveCommandEffect;
		break;
	}
	if (EffectToSpawn)
	{
		// 지면에서 약간 띄워서 스폰 (Z축 오프셋)
		FVector SpawnLoc = Loc + FVector(0.f, 0.f, 10.f);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), EffectToSpawn, SpawnLoc);
	}
}
