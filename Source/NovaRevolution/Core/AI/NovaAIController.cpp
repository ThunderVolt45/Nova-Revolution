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
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"

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
}

void ANovaAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsManualMoving)
	{
		UpdateManualMovement(DeltaTime);
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
}

void ANovaAIController::IssueCommand(const FCommandData& CommandData)
{
	if (BlackboardComponent && BlackboardComponent->GetBlackboardAsset())
	{
		APawn* MyPawn = GetPawn();
		if (!MyPawn) return;

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

	// 2. 지상 유닛: 기존 엔진 이동
	bIsManualMoving = false;
	MoveToLocation(Dest, AcceptanceRadius, true, true, false, true, nullptr, true);
}

void ANovaAIController::MoveToActorOptimized(AActor* TargetActor, float AcceptanceRadius)
{
	if (!TargetActor) return;

	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;

	ANovaUnit* MyUnit = Cast<ANovaUnit>(MyPawn);
	ENovaMovementType MyMoveType = MyUnit ? MyUnit->GetMovementType() : ENovaMovementType::Ground;

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
				MoveToLocation(ProjectedLocation.Location, AcceptanceRadius, true, true, false, true, nullptr, true);
				return;
			}
		}
	}

	// 일반적인 지상 타겟 추적 (MoveToActor는 엔진 내부에서 최적화되므로 중복 체크 제거)
	MoveToActor(TargetActor, AcceptanceRadius, true, true, false, nullptr, true);
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
	}
	
	StopMovement();
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
