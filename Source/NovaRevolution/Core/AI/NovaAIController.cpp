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
#include "Engine/OverlapResult.h"

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

	bool bIsMoving = IsMoveInProgress();

	// 이동 중일 때만 Stuck 감지 로직 실행
	if (bIsMoving)
	{
		UpdateStuckDetection(DeltaTime);
	}
	else
	{
		StuckTimer = 0.0f;
	}

	// [핵심] 유닛이 엔진 내비게이션을 통해 이동 중이 아닐 때만 장애물로 작동하게 함.
	// 이를 통해 순찰 대기 중이거나 공격 사거리 내에서 제자리에 서서 공격할 때 자동으로 장애물이 됩니다.
	if (ANovaUnit* MyUnit = Cast<ANovaUnit>(GetPawn()))
	{
		MyUnit->SetNavigationObstacle(!bIsMoving);
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

		// [추가] 공중 유닛의 목표 위치 Z값을 공중(가상 평면) 고도로 강제 고정
		FCommandData AdjustedCommandData = CommandData;
		if (ANovaUnit* MyUnit = Cast<ANovaUnit>(MyPawn))
		{
			if (MyUnit->GetMovementType() == ENovaMovementType::Air && 
			   (AdjustedCommandData.CommandType == ECommandType::Move || 
			    AdjustedCommandData.CommandType == ECommandType::Patrol || 
			    AdjustedCommandData.CommandType == ECommandType::Spread))
			{
				AdjustedCommandData.TargetLocation.Z = MyUnit->GetDefaultAirZ();
			}
		}

		// 3. 블랙보드 데이터 업데이트
		// 이 호출 직후 OnCommandTypeChanged 옵저버가 실행되어 내비게이션 장애물 상태가 자동으로 변경됩니다.
		BlackboardComponent->SetValueAsEnum(CommandTypeKey, (uint8)AdjustedCommandData.CommandType);
		
		switch (AdjustedCommandData.CommandType)
		{
		case ECommandType::Move:
		case ECommandType::Patrol:
			// 이미 해당 위치(최소 사거리) 근처에 있다면 불필요한 이동 명령 방지
			if (FVector::DistSquared(MyPawn->GetActorLocation(), AdjustedCommandData.TargetLocation) < FMath::Square(MinMoveDistance))
			{
				BlackboardComponent->SetValueAsEnum(CommandTypeKey, (uint8)ECommandType::None);
				return;
			}
			BlackboardComponent->SetValueAsVector(TargetLocationKey, AdjustedCommandData.TargetLocation);
			BlackboardComponent->ClearValue(TargetActorKey);
			break;

		case ECommandType::Attack:
			if (AdjustedCommandData.TargetActor)
			{
				BlackboardComponent->SetValueAsObject(TargetActorKey, AdjustedCommandData.TargetActor);
				BlackboardComponent->SetValueAsVector(TargetLocationKey, AdjustedCommandData.TargetActor->GetActorLocation());
			}
			else
			{
				BlackboardComponent->SetValueAsVector(TargetLocationKey, AdjustedCommandData.TargetLocation);
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
			BlackboardComponent->SetValueAsVector(TargetLocationKey, AdjustedCommandData.TargetLocation);
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

	// 이동 시작 시 장애물 상태 해제 (경로 탐색 방해 방지)
	if (MyUnit)
	{
		MyUnit->SetNavigationObstacle(false);
	}

	// 이동 시작 시 타이머 초기화
	StuckTimer = 0.0f;
	LastStuckCheckLocation = MyPawn->GetActorLocation();

	// 수동 이동 플래그 해제
	bIsManualMoving = false;
	
	// [추가] 공중 유닛인 경우 목적지의 Z값을 가상 평면(하늘 NavMesh) 높이로 강제 보정합니다.
	FVector FinalDest = Dest;
	if (MyUnit && MyUnit->GetMovementType() == ENovaMovementType::Air)
	{
		FinalDest.Z = MyUnit->GetDefaultAirZ();
	}

	// 이동용 필터를 사용하여 유닛 간 뭉침 상황에서도 즉시 경로 생성 (공중/지상 모두 적용)
	MoveToLocation(FinalDest, AcceptanceRadius, true, true, true, true, UNovaNavigationFilter_Move::StaticClass(), true);
}

void ANovaAIController::MoveToActorOptimized(AActor* TargetActor, float AcceptanceRadius)
{
	if (!TargetActor) return;

	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;

	ANovaUnit* MyUnit = Cast<ANovaUnit>(MyPawn);

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

	// 수동 이동 해제
	bIsManualMoving = false;

	// 지상 유닛이 공중 타겟을 쫓는 경우: 타겟의 수평 위치 기반 NavMesh 지점으로 이동
	if (MyUnit && MyUnit->GetMovementType() == ENovaMovementType::Ground && bIsTargetAir)
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

	// 일반적인 타겟 추적 시 필터 적용 (공중 유닛도 엔진 이동 사용)
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

	StuckTimer += DeltaTime;

	// 지정된 시간(StuckTimeThreshold)마다 한 번씩 이동 거리를 검사합니다.
	if (StuckTimer >= StuckTimeThreshold)
	{
		FVector CurrentLocation = MyPawn->GetActorLocation();
		// Z축은 제외하고 평면상의 이동 거리만 확인
		float DistanceMovedSq = FVector::DistSquaredXY(CurrentLocation, LastStuckCheckLocation);

		// 정해진 시간 동안 임계값(StuckDistanceThreshold)만큼도 이동하지 못했다면 Stuck으로 간주
		if (DistanceMovedSq < FMath::Square(StuckDistanceThreshold))
		{
			NOVA_LOG(Warning, "AIController: Stuck Detected for Pawn [%s]! Handling...", *MyPawn->GetName());
			HandleStuckStatus();
		}

		// 다음 주기를 위해 타이머와 위치를 초기화
		StuckTimer = 0.0f;
		LastStuckCheckLocation = CurrentLocation;
	}
}

ECommandType ANovaAIController::GetCurrentCommand() const
{
	if (BlackboardComponent)
	{
		return static_cast<ECommandType>(BlackboardComponent->GetValueAsEnum(CommandTypeKey));
	}
	return ECommandType::None;
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
	// 목표 지점 근처(예: 600유닛 이내)에서만 점유 판정 수행 (멀리서 미리 멈추는 것 방지)
	float DistToGoalSq = FVector::DistSquared(CurrentLocation, GoalLocation);
	bool bIsNearGoal = DistToGoalSq < FMath::Square(EarlyArrivalDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(MyPawn);
	if (TargetActor) QueryParams.AddIgnoredActor(TargetActor);

	float CheckRadius = 50.0f;
	ANovaUnit* MyUnit = Cast<ANovaUnit>(MyPawn);
	if (MyUnit)
	{
		if (UCapsuleComponent* Capsule = MyUnit->GetCapsuleComponent())
		{
			CheckRadius = Capsule->GetScaledCapsuleRadius();
		}
	}

	bool bIsGoalOccupied = false;

	if (bIsNearGoal)
	{
		FHitResult Hit;
		bIsGoalOccupied = GetWorld()->SweepSingleByChannel(
			Hit, GoalLocation, GoalLocation + FVector(0,0,10), 
			FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(CheckRadius), QueryParams);

		// [개선] 목표 지점 자체가 막히지 않았더라도, 내가 아군에 가로막혀 목표에 다가가지 못하는 상태라면 조기 도착(Early Arrival)으로 간주
		if (!bIsGoalOccupied && MyUnit)
		{
			TArray<FOverlapResult> Overlaps;
			GetWorld()->OverlapMultiByChannel(Overlaps, CurrentLocation, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(CheckRadius * 1.5f), QueryParams);
			for (const FOverlapResult& Overlap : Overlaps)
			{
				if (ANovaUnit* OtherUnit = Cast<ANovaUnit>(Overlap.GetActor()))
				{
					if (OtherUnit->GetTeamID() == MyUnit->GetTeamID())
					{
						bIsGoalOccupied = true;
						NOVA_LOG(Log, "AIController: Blocked by ally near goal. Considering goal occupied for [%s]", *MyPawn->GetName());
						break;
					}
				}
			}
		}
	}

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
			// 순찰/공격 등은 멈추지 않고 주변에서 대기하며 기회를 봄 (순찰의 경우 BTTask에서 즉시 목적지 반전됨)
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

	FVector DetourPoint = CurrentLocation + BypassDirection * BypassDistance;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		FNavLocation ProjectedLocation;
		// 투영 범위도 우회 거리에 비례하여 설정
		if (NavSys->ProjectPointToNavigation(DetourPoint, ProjectedLocation, FVector(BypassDistance, BypassDistance, 300.f)))
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

void ANovaAIController::RestartLogic()
{
	if (BehaviorTreeComponent)
	{
		BehaviorTreeComponent->RestartLogic();
	}
}
