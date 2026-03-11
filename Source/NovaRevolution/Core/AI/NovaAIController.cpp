// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaAIController.h"
#include "VisualLogger/VisualLogger.h"
#include "Navigation/PathFollowingComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "NovaRevolution.h"
#include "Core/NovaUnit.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "NovaNavigationFilter_Move.h"

const FName ANovaAIController::TargetLocationKey(TEXT("TargetLocation"));
const FName ANovaAIController::TargetActorKey(TEXT("TargetActor"));
const FName ANovaAIController::CommandTypeKey(TEXT("CommandType"));

ANovaAIController::ANovaAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	bSetControlRotationFromPawnOrientation = true;

	// 컴포넌트 초기화
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));

	// 기본 내비게이션 필터 설정: 모든 경로 탐색에서 유닛 장애물 비용 무시
	DefaultNavigationFilterClass = UNovaNavigationFilter_Move::StaticClass();
}

void ANovaAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsManualMoving)
	{
		UpdateManualMovement(DeltaTime);
	}

	// 이동 중일 때만 Stuck 감지 로직 실행
	if (IsMoveInProgress())
	{
		UpdateStuckDetection(DeltaTime);
	}
	else
	{
		StuckTimer = 0.0f;
	}
}

void ANovaAIController::OnPossess(APawn* InPawn)
{
	// 비헤이비어 트리 실행
	if (BehaviorTreeAsset)
	{
		bool bSuccess = RunBehaviorTree(BehaviorTreeAsset);
		if (bSuccess)
		{
			NOVA_LOG(Log, "AIController: Successfully started Behavior Tree [%s] for Pawn [%s]", *BehaviorTreeAsset->GetName(), *InPawn->GetName());
		}
		else
		{
			NOVA_LOG(Error, "AIController: Failed to run Behavior Tree [%s] for Pawn [%s]", *BehaviorTreeAsset->GetName(), *InPawn->GetName());
		}
	}
	else
	{
		NOVA_LOG(Warning, "AIController: BehaviorTreeAsset is NOT assigned in %s! (Pawn: %s)", *GetName(), InPawn ? *InPawn->GetName() : TEXT("NULL"));
	}
	
	Super::OnPossess(InPawn);

	// 블랙보드 옵저버 등록: CommandType 변화 감지
	if (BlackboardComponent && BlackboardComponent->GetBlackboardAsset())
	{
		FBlackboard::FKey CommandTypeKeyID = BlackboardComponent->GetBlackboardAsset()->GetKeyID(CommandTypeKey);
		if (CommandTypeKeyID != FBlackboard::InvalidKey)
		{
			BlackboardComponent->RegisterObserver(CommandTypeKeyID, this, FOnBlackboardChangeNotification::CreateUObject(this, &ANovaAIController::OnCommandTypeChanged));
		}
	}

	// 초기 위치 저장
	if (InPawn)
	{
		LastStuckCheckLocation = InPawn->GetActorLocation();
	}
}

void ANovaAIController::OnUnPossess()
{
	// 블랙보드 옵저버 해제 (KeyID를 캐싱하지 않았으므로 모든 옵저버 해제 또는 시스템에 맡김)
	if (BlackboardComponent)
	{
		BlackboardComponent->UnregisterObserversFrom(this);
	}

	Super::OnUnPossess();
}

void ANovaAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	// 이동이 완료되었을 때 (성공, 중단 등 상관없이 멈춘 상태) 장애물로 설정
	// 이를 통해 공격이나 순찰 중 사거리 내에 도달하여 멈췄을 때도 장애물이 됩니다.
	if (ANovaUnit* MyUnit = Cast<ANovaUnit>(GetPawn()))
	{
		MyUnit->SetNavigationObstacle(true);
	}
}

EBlackboardNotificationResult ANovaAIController::OnCommandTypeChanged(const UBlackboardComponent& InBlackboard, FBlackboard::FKey KeyID)
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn) return EBlackboardNotificationResult::RemoveObserver;

	ANovaUnit* MyUnit = Cast<ANovaUnit>(MyPawn);
	if (!MyUnit) return EBlackboardNotificationResult::ContinueObserving;

	ECommandType CurrentCommand = static_cast<ECommandType>(InBlackboard.GetValueAsEnum(CommandTypeKey));

	// 모든 상태 변화(플레이어 명령 + 행동 트리 자율 변경)에 대해 장애물 상태를 자동 전환
	switch (CurrentCommand)
	{
	case ECommandType::Move:
	case ECommandType::Patrol:
	case ECommandType::Attack:
	case ECommandType::Spread:
		// 이동이 필요한 상태면 장애물 해제
		MyUnit->SetNavigationObstacle(false);
		break;

	case ECommandType::None:
	case ECommandType::Stop:
	case ECommandType::Hold:
	case ECommandType::Halt:
		// 정지해야 하는 상태면 장애물 활성화
		MyUnit->SetNavigationObstacle(true);
		break;

	default:
		break;
	}

	return EBlackboardNotificationResult::ContinueObserving;
}

void ANovaAIController::IssueCommand(const FCommandData& CommandData)
{
	if (BlackboardComponent && BlackboardComponent->GetBlackboardAsset())
	{
		APawn* MyPawn = GetPawn();
		if (!MyPawn) return;

		// 새로운 명령 시 Stuck 상태 초기화
		StuckTimer = 0.0f;
		LastStuckCheckLocation = MyPawn->GetActorLocation();

		// 1. 자기 자신을 대상으로 하는 공격 명령 차단
		if (CommandData.CommandType == ECommandType::Attack && CommandData.TargetActor == MyPawn)
		{
			NOVA_LOG(Warning, "AIController: Self-attack command ignored.");
			return;
		}

		// 2. 명령 수신 시 이전 수동 이동 상태 초기화 (이동 명령 계열이 아닌 경우)
		if (CommandData.CommandType != ECommandType::Attack && CommandData.CommandType != ECommandType::Move && CommandData.CommandType != ECommandType::Patrol)
		{
			StopMovementOptimized();
		}

		// 3. 블랙보드 데이터 업데이트
		// 이 호출 직후 OnCommandTypeChanged 옵저버가 실행되어 내비게이션 장애물 상태가 자동으로 변경됩니다.
		BlackboardComponent->SetValueAsEnum(CommandTypeKey, (uint8)CommandData.CommandType);
		
		switch (CommandData.CommandType)
		{
		case ECommandType::Move:
		case ECommandType::Patrol:
			// 이미 해당 위치(최소 사거리) 근처에 있다면 불필요한 이동 명령 방지
			if (FVector::DistSquared(MyPawn->GetActorLocation(), CommandData.TargetLocation) < FMath::Square(MinMoveDistance))
			{
				BlackboardComponent->SetValueAsEnum(CommandTypeKey, (uint8)ECommandType::None);
				return;
			}
			BlackboardComponent->SetValueAsVector(TargetLocationKey, CommandData.TargetLocation);
			BlackboardComponent->ClearValue(TargetActorKey);
			break;

		case ECommandType::Attack:
			if (CommandData.TargetActor)
			{
				BlackboardComponent->SetValueAsObject(TargetActorKey, CommandData.TargetActor);
				BlackboardComponent->SetValueAsVector(TargetLocationKey, CommandData.TargetActor->GetActorLocation());
			}
			else
			{
				BlackboardComponent->SetValueAsVector(TargetLocationKey, CommandData.TargetLocation);
				BlackboardComponent->ClearValue(TargetActorKey);
			}
			break;

		case ECommandType::Stop:
		case ECommandType::Hold:
		case ECommandType::Halt:
			BlackboardComponent->ClearValue(TargetLocationKey);
			BlackboardComponent->ClearValue(TargetActorKey);
			StopMovementOptimized();
			break;

		case ECommandType::Spread:
			BlackboardComponent->SetValueAsVector(TargetLocationKey, CommandData.TargetLocation);
			BlackboardComponent->ClearValue(TargetActorKey);
			break;

		default:
			break;
		}

		// 비헤이비어 트리 강제 재시작 (즉각적인 반응성 확보)
		if (BehaviorTreeComponent)
		{
			BehaviorTreeComponent->RestartLogic();
		}
	}
}

void ANovaAIController::MoveToLocationOptimized(const FVector& Dest, float AcceptanceRadius)
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;

	ANovaUnit* MyUnit = Cast<ANovaUnit>(MyPawn);
	ENovaMovementType MoveType = MyUnit ? MyUnit->GetMovementType() : ENovaMovementType::Ground;

	// 이동 시작 시 장애물 상태 해제 (경로 탐색 방해 방지)
	if (MyUnit)
	{
		MyUnit->SetNavigationObstacle(false);
	}

	// 이동 시작 시 타이머 초기화
	StuckTimer = 0.0f;
	LastStuckCheckLocation = MyPawn->GetActorLocation();

	// 1. 공중 유닛: 수동 이동 설정
	if (MoveType == ENovaMovementType::Air)
	{
		// 이전에 엔진 경로 추종이 활성화되어 있었다면 한 번만 중단
		if (GetMoveStatus() != EPathFollowingStatus::Idle)
		{
			StopMovement();
		}

		bIsManualMoving = true;
		ManualMoveGoal = Dest;
		ManualMoveTargetActor = nullptr;
		ManualAcceptanceRadius = AcceptanceRadius;
		return;
	}

	// 2. 지상 유닛: 이동용 필터를 사용하여 유닛 간 뭉침 상황에서도 즉시 경로 생성
	bIsManualMoving = false;
	MoveToLocation(Dest, AcceptanceRadius, true, true, true, true, UNovaNavigationFilter_Move::StaticClass(), true);
}

void ANovaAIController::MoveToActorOptimized(AActor* TargetActor, float AcceptanceRadius)
{
	if (!TargetActor) return;

	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;

	ANovaUnit* MyUnit = Cast<ANovaUnit>(MyPawn);
	ENovaMovementType MyMoveType = MyUnit ? MyUnit->GetMovementType() : ENovaMovementType::Ground;

	// 이동 시작 시 장애물 상태 해제
	if (MyUnit)
	{
		MyUnit->SetNavigationObstacle(false);
	}

	// 이동 시작 시 타이머 초기화
	StuckTimer = 0.0f;
	LastStuckCheckLocation = MyPawn->GetActorLocation();

	// 타겟의 정보 확인
	ANovaUnit* TargetUnit = Cast<ANovaUnit>(TargetActor);
	bool bIsTargetAir = TargetUnit && (TargetUnit->GetMovementType() == ENovaMovementType::Air);

	// 1. 공중 유닛: 수동 추적 설정
	if (MyMoveType == ENovaMovementType::Air)
	{
		if (GetMoveStatus() != EPathFollowingStatus::Idle)
		{
			StopMovement();
		}

		bIsManualMoving = true;
		ManualMoveTargetActor = TargetActor;
		ManualAcceptanceRadius = AcceptanceRadius;
		return;
	}

	// 2. 지상 유닛 처리
	bIsManualMoving = false;

	// 지상 유닛이 공중 타겟을 쫓는 경우: 타겟의 수평 위치 기반 NavMesh 지점으로 이동
	if (bIsTargetAir)
	{
		FVector TargetLoc = TargetActor->GetActorLocation();
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if (NavSys)
		{
			FNavLocation ProjectedLocation;
			if (NavSys->ProjectPointToNavigation(TargetLoc, ProjectedLocation, FVector(500.f, 500.f, 2000.f)))
			{
				MoveToLocation(ProjectedLocation.Location, AcceptanceRadius, true, true, true, true, UNovaNavigationFilter_Move::StaticClass(), true);
				return;
			}
		}
	}

	// 일반적인 지상 타겟 추적 시에도 필터 적용 및 목적지 투영 활성화
	MoveToActor(TargetActor, AcceptanceRadius, true, true, true, UNovaNavigationFilter_Move::StaticClass(), true);
}

bool ANovaAIController::IsMoveInProgress() const
{
	if (bIsManualMoving) return true;
	
	if (UPathFollowingComponent* PFollow = GetPathFollowingComponent())
	{
		return PFollow->GetStatus() != EPathFollowingStatus::Idle;
	}
	
	return false;
}

void ANovaAIController::StopMovementOptimized()
{
	bIsManualMoving = false;
	ManualMoveTargetActor = nullptr;
	
	if (APawn* MyPawn = GetPawn())
	{
		ManualMoveGoal = MyPawn->GetActorLocation();

		if (ANovaUnit* MyUnit = Cast<ANovaUnit>(MyPawn))
		{
			MyUnit->SetNavigationObstacle(true);
		}
	}
	
	StopMovement();
	
	// Stuck 감지 상태 해제
	StuckTimer = 0.0f;
}

void ANovaAIController::UpdateManualMovement(float DeltaSeconds)
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		bIsManualMoving = false;
		return;
	}

	// 1. 타겟 액터 추적 중인 경우 유효성 및 사망 여부 검사
	if (ManualMoveTargetActor.IsValid())
	{
		bool bTargetInvalid = false;
		
		// 유닛인 경우 사망 상태 확인
		if (ANovaUnit* TargetUnit = Cast<ANovaUnit>(ManualMoveTargetActor.Get()))
		{
			if (TargetUnit->IsDead()) bTargetInvalid = true;
		}

		if (bTargetInvalid || ManualMoveTargetActor->IsPendingKillPending())
		{
			StopMovementOptimized();
			return;
		}
		
		ManualMoveGoal = ManualMoveTargetActor->GetActorLocation();
	}

	FVector CurrentLocation = MyPawn->GetActorLocation();
	FVector Direction = (ManualMoveGoal - CurrentLocation);
	Direction.Z = 0.0f;

	float Distance = Direction.Size();

	if (Distance <= ManualAcceptanceRadius)
	{
		bIsManualMoving = false;

		// 목적지 도착 시 장애물 상태 활성화
		if (ANovaUnit* MyUnit = Cast<ANovaUnit>(MyPawn))
		{
			MyUnit->SetNavigationObstacle(true);
		}
		return;
	}

	Direction.Normalize();
	MyPawn->AddMovementInput(Direction, 1.0f);

	// 회전 보간
	FRotator CurrentRotation = MyPawn->GetActorRotation();
	FRotator TargetRotation = Direction.Rotation();
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, 5.0f);
	MyPawn->SetActorRotation(NewRotation);
}

void ANovaAIController::UpdateStuckDetection(float DeltaTime)
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;

	FVector CurrentLocation = MyPawn->GetActorLocation();
	float DistanceMoved = FVector::Dist(CurrentLocation, LastStuckCheckLocation);

	// 일정 거리보다 적게 움직였는지 확인 (수평 거리 위주)
	if (DistanceMoved < StuckDistanceThreshold)
	{
		StuckTimer += DeltaTime;
		
		if (StuckTimer >= StuckTimeThreshold)
		{
			NOVA_LOG(Warning, "AIController: Stuck Detected for Pawn [%s]! Handling...", *MyPawn->GetName());
			HandleStuckStatus();
			StuckTimer = 0.0f; // 한 번 처리 후 타이머 초기화 (반복 방지)
		}
	}
	else
	{
		// 유의미한 움직임이 있었다면 타이머 및 체크 위치 갱신
		StuckTimer = 0.0f;
		LastStuckCheckLocation = CurrentLocation;
	}
}

void ANovaAIController::HandleStuckStatus()
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn || !BlackboardComponent) return;

	FVector CurrentLocation = MyPawn->GetActorLocation();
	FVector GoalLocation = BlackboardComponent->GetValueAsVector(TargetLocationKey);
	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(TargetActorKey));
	ECommandType CurrentCommand = (ECommandType)BlackboardComponent->GetValueAsEnum(CommandTypeKey);

	// --- 명령 성격 구분 ---
	// 목표 지점이 점유되었을 때 중단되어도 괜찮은 명령들 (단발성 이동)
	bool bIsDisposableCommand = (CurrentCommand == ECommandType::Move || CurrentCommand == ECommandType::Spread);

	// 1. 목표 지점 점유 체크
	// 목표 지점 근처(예: 500유닛 이내)에서만 점유 판정 수행 (멀리서 미리 멈추는 것 방지)
	float DistToGoalSq = FVector::DistSquared(CurrentLocation, GoalLocation);
	bool bIsNearGoal = DistToGoalSq < FMath::Square(500.0f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(MyPawn);
	if (TargetActor) QueryParams.AddIgnoredActor(TargetActor);

	float CheckRadius = 50.0f;
	if (ANovaUnit* MyUnit = Cast<ANovaUnit>(MyPawn))
	{
		if (UCapsuleComponent* Capsule = MyUnit->GetCapsuleComponent())
		{
			CheckRadius = Capsule->GetScaledCapsuleRadius();
		}
	}

	FHitResult Hit;
	bool bIsGoalOccupied = bIsNearGoal && GetWorld()->SweepSingleByChannel(
		Hit, GoalLocation, GoalLocation + FVector(0,0,10), 
		FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(CheckRadius), QueryParams);

	// 2. 대응 로직 분기
	if (bIsGoalOccupied)
	{
		if (bIsDisposableCommand)
		{
			// 이동/산개 명령은 목표 지점이 꽉 차있으면 중단 (명령 완료로 간주)
			NOVA_LOG(Log, "AIController: Goal is occupied. Terminating disposable command [%d] for [%s]", (uint8)CurrentCommand, *MyPawn->GetName());
			BlackboardComponent->SetValueAsEnum(CommandTypeKey, (uint8)ECommandType::None);
			StopMovementOptimized();
		}
		else
		{
			// 순찰/공격 등은 멈추지 않고 주변에서 대기하며 기회를 봄
			NOVA_LOG(Log, "AIController: Goal occupied but persistent command [%d] remains. Waiting for [%s]", (uint8)CurrentCommand, *MyPawn->GetName());
			StopMovementOptimized(); 
		}
		return;
	}

	// 3. 목표 지점은 비어있는데 끼어있는 경우 -> 옆으로 우회 시도
	NOVA_LOG(Log, "AIController: Goal clear but stuck. Attempting bypass for [%s]", *MyPawn->GetName());

	FVector MoveDirection = (GoalLocation - CurrentLocation).GetSafeNormal();
	FVector BypassDirection = FVector::CrossProduct(MoveDirection, FVector::UpVector);
	
	if (FMath::RandBool()) BypassDirection *= -1.0f;

	FVector DetourPoint = CurrentLocation + BypassDirection * 200.0f;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		FNavLocation ProjectedLocation;
		if (NavSys->ProjectPointToNavigation(DetourPoint, ProjectedLocation, FVector(300.f, 300.f, 300.f)))
		{
			// 일시적으로 우회 지점으로 이동
			MoveToLocationOptimized(ProjectedLocation.Location, 50.0f);
		}
		else
		{
			// 우회 지점조차 없다면 중단 가능한 명령만 종료
			if (bIsDisposableCommand)
			{
				BlackboardComponent->SetValueAsEnum(CommandTypeKey, (uint8)ECommandType::None);
			}
			StopMovementOptimized();
		}
	}
}

void ANovaAIController::ActivateAbilityByTag(const FGameplayTag& AbilityTag, AActor* Target)
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn || !Target || !AbilityTag.IsValid()) return;

	FGameplayEventData Payload;
	Payload.Instigator = MyPawn;
	Payload.Target = Target;
	Payload.EventTag = AbilityTag;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MyPawn, AbilityTag, Payload);
}

void ANovaAIController::OnPawnDeath()
{
	if (BehaviorTreeComponent)
	{
		BehaviorTreeComponent->StopTree(EBTStopMode::Safe);
	}

	StopMovementOptimized();
}
