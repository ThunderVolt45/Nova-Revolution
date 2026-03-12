// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaUnit.h"
#include "Core/NovaPart.h"
#include "Core/NovaPartData.h"
#include "Core/NovaResourceComponent.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "NovaRevolution.h"
#include "NovaPlayerState.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/AI/NovaAIController.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/NovaAttributeSet.h"
#include "GAS/Abilities/NovaGameplayAbility.h"
#include "Player/NovaPlayerController.h"
#include "Engine/OverlapResult.h"
#include "Core/NovaObjectPoolSubsystem.h"
#include "BrainComponent.h"

#include "NavModifierComponent.h"
#include "NovaNavArea_Unit.h"
#include "AI/Navigation/NavigationDataResolution.h"
// #include "Commandlets/GatherTextFromAssetsCommandlet.h"
#include "NavAreas/NavArea_Default.h"
#include "UI/NovaHealthBarComponent.h"
#include "UI/NovaSelectionComponent.h"

ANovaUnit::ANovaUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	// 내비게이션 설정: 캡슐 기하구조가 직접 NavMesh를 파내지 않게 함 (이동 방해 방지)
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCanEverAffectNavigation(false);
	}

	// NavModifier 생성: 장애물 상태일 때만 특정 영역(고비용)을 생성하도록 설정
	NavModifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavModifier"));
	if (NavModifier)
	{
		NavModifier->SetAreaClass(UNovaNavArea_Unit::StaticClass());
		NavModifier->bAutoActivate = false; // 기본적으로는 비활성화 (이동 우선)
		NavModifier->NavMeshResolution = ENavigationDataResolution::High;
	}

	// AI 설정: 스폰 시 자동으로 전용 AI 컨트롤러 생성 및 빙의
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ANovaAIController::StaticClass();

	// 상속받은 기본 메시는 사용하지 않으므로 숨김 및 충돌 비활성화
	if (GetMesh())
	{
		GetMesh()->SetHiddenInGame(true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// ASC 및 AttributeSet 구성
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UNovaAttributeSet>(TEXT("AttributeSet"));

	// 기본 팀 ID 설정
	TeamID = NovaTeam::None;

	// 이동 설정: 캐릭터가 이동 방향으로 자동 회전
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
		GetCharacterMovement()->bConstrainToPlane = true;
		GetCharacterMovement()->bSnapToPlaneAtStart = true;
	}

	// 컨트롤러 회전 사용 안 함 (이동 방향 회전 우선)
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	// 선택 표시 위젯 컴포넌트
	SelectionComponent = CreateDefaultSubobject<UNovaSelectionComponent>(TEXT("SelectionComponent"));
	SelectionComponent->SetupAttachment(RootComponent);
	SelectionComponent->SetRelativeLocation(FVector::ZeroVector);

	// 체력바 컴포넌트 추가
	HealthBarComponent = CreateDefaultSubobject<UNovaHealthBarComponent>(TEXT("HealthBarComponent"));
	HealthBarComponent->SetupAttachment(RootComponent);
}

void ANovaUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead) return;

	// 1. 공중 유닛 고도 조절 (가상 평면 NavMesh 사용을 위해 주석 처리)
	/*
	if (MovementType == ENovaMovementType::Air)
	{
		FVector CurrentLocation = GetActorLocation();
		float TargetZ = DefaultAirZ;

		// 아래 방향으로 레이캐스트하여 지형 높이 확인
		FHitResult HitResult;
		FVector Start = CurrentLocation + FVector(0.f, 0.f, 100.f);
		FVector End = CurrentLocation - FVector(0.f, 0.f, 2000.f);
		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(this);

		// 유닛(Pawn) 등을 지면으로 인식하지 않도록 WorldStatic 채널만 체크합니다.
		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, TraceParams))
		{
			float FloorZ = HitResult.ImpactPoint.Z;
			float SafetyZ = FloorZ + MinSafetyHeight;

			// 지면이 높아서 안전 고도를 확보해야 하는 경우 목표 Z 상향 조정
			if (SafetyZ > DefaultAirZ)
			{
				TargetZ = SafetyZ;
			}
		}

		// 현재 Z값을 목표 Z값으로 부드럽게 보간
		if (!FMath::IsNearlyEqual(CurrentLocation.Z, TargetZ, 1.0f))
		{
			float NewZ = FMath::FInterpTo(CurrentLocation.Z, TargetZ, DeltaTime, HeightInterpSpeed);
			SetActorLocation(FVector(CurrentLocation.X, CurrentLocation.Y, NewZ));
		}
	}
	*/

	// 2. 다리(Legs) 부품 애니메이션 데이터 전달
	if (CurrentLegsPart)
	{
		// 이동 속도 전달
		float CurrentSpeed = GetVelocity().Size();
		CurrentLegsPart->SetMovementSpeed(CurrentSpeed);

		// 회전 속도 계산 및 전달
		float CurrentYaw = GetActorRotation().Yaw;
		float YawDelta = FMath::FindDeltaAngleDegrees(LastYaw, CurrentYaw);
		float RotationRate = YawDelta / DeltaTime; // 초당 회전 각도

		CurrentLegsPart->SetRotationRate(RotationRate);
		LastYaw = CurrentYaw;
	}

	// 3. 몸통(Body) 및 무기 회전 로직 실행
	UpdateBodyRotation(DeltaTime);
	UpdateWeaponAiming(DeltaTime);

	// 4. 완벽히 겹쳤을 때 이동 불가가 되는 현상 방지 (Anti-Overlap) 및 아군 길막힘 방지 밀어내기
	if (!bIsDead && GetCapsuleComponent())
	{
		float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
		TArray<FOverlapResult> Overlaps;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		// 아군 길막힘 방지를 위해 반경을 조금 더 넓게(1.1f) 잡습니다.
		if (GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn,
		                                      FCollisionShape::MakeSphere(Radius * 1.1f), Params))
		{
			for (const FOverlapResult& Overlap : Overlaps)
			{
				if (ANovaUnit* OtherUnit = Cast<ANovaUnit>(Overlap.GetActor()))
				{
					if (!OtherUnit->IsDead())
					{
						FVector MyLoc = GetActorLocation();
						FVector OtherLoc = OtherUnit->GetActorLocation();
						FVector Diff = MyLoc - OtherLoc;
						Diff.Z = 0.0f; // 수평 방향으로만 밀어냄

						float Dist = Diff.Size();

						// 거의 완전히 겹쳤을 경우 랜덤한 방향으로 튕겨냄 (기존 Anti-Overlap)
						if (Dist < 0.1f)
						{
							FVector RandomDir = FVector(FMath::RandRange(-1.f, 1.f), FMath::RandRange(-1.f, 1.f), 0.f).
								GetSafeNormal();
							AddActorWorldOffset(RandomDir * 2.0f, false);
							continue; // 이미 밀어냈으므로 다음 유닛 검사
						}

						// 충돌 반경의 80% 이내로 깊숙이 파고든 경우 밖으로 밀어냄 (기존 Anti-Overlap)
						if (Dist < Radius * 0.8f)
						{
							FVector PushDir = Diff / Dist;

							// 이동 중일 때 정면 충돌 데드락 방지 (우측 통행 유도)
							FVector Velocity = GetVelocity();
							Velocity.Z = 0.0f;

							if (Velocity.SizeSquared() > 100.0f) // 속도가 일정 이상일 때만 적용
							{
								FVector MyForward = Velocity.GetSafeNormal();
								FVector MyRight = FVector::CrossProduct(FVector::UpVector, MyForward);

								// 상대방이 내 기준으로 어느 쪽에 있는지 판별 (-PushDir은 상대를 향하는 방향)
								float DotRight = FVector::DotProduct(MyRight, -PushDir);

								FVector TangentDir;
								// 상대가 내 오른쪽에 있거나 정면에 가까울 때 -> 내 오른쪽으로 회피 (우측 통행)
								if (DotRight > -0.1f)
								{
									TangentDir = MyRight;
								}
								// 상대가 내 왼쪽에 확실히 치우쳐 있을 때 -> 내 왼쪽으로 회피
								else
								{
									TangentDir = -MyRight;
								}

								// 바깥으로 밀어내는 힘과 횡방향(옆으로 비껴가는) 힘을 혼합
								PushDir = (PushDir * 0.6f + TangentDir * 0.4f).GetSafeNormal();
							}

							AddActorWorldOffset(PushDir * 1.5f, false);
						}

						// --- 아군 길막힘 방지 밀어내기 로직 ---
						if (GetTeamID() == OtherUnit->GetTeamID())
						{
							ECommandType OtherCommand = ECommandType::None;
							if (ANovaAIController* OtherAI = Cast<ANovaAIController>(OtherUnit->GetController()))
							{
								OtherCommand = OtherAI->GetCurrentCommand();
							}

							// 상대 유닛이 Idle, Stop, Attack, Patrol 중일 때 상대를 밀어냅니다.
							if (OtherCommand == ECommandType::None ||
								OtherCommand == ECommandType::Stop ||
								OtherCommand == ECommandType::Attack ||
								OtherCommand == ECommandType::Patrol)
							{
								// 상대를 밀어낼 방향 (-Diff는 OtherUnit이 나에게서 멀어지는 방향)
								FVector PushDir = -Diff.GetSafeNormal();
								// 연쇄 밀어내기 호출
								OtherUnit->PushUnit(PushDir, 2.0f, 0);
							}
						}
					}
				}
			}
		}
	}

	if (HealthBarComponent && HealthBarComponent->IsVisible())
	{
		if (APlayerCameraManager* CamManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager)
		{
			// 1. 카메라의 상단 벡터(Up Vector)를 가져옵니다.
			// 화면상의 "아래"는 이 벡터의 반대 방향(-UpVector)입니다.
			FVector CameraUp = CamManager->GetCameraRotation().Quaternion().GetUpVector();
			FVector ScreenDownDir = -CameraUp;

			// 2. 캡슐의 바닥 지점을 찾습니다.
			float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			FVector CapsuleBottom = GetActorLocation() - FVector(0.f, 0.f, HalfHeight);

			// 3. 캡슐 반지름(Radius) + 추가 오프셋(예: 20.f)
			float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
			float TotalOffset = Radius + 20.0f; // 여기서 간격 조절 가능

			// 4. 최종 3D 위치: 캡슐 바닥에서 "화면 아래 방향"으로 오프셋만큼 이동
			FVector FinalAnchorLoc = CapsuleBottom + (ScreenDownDir * TotalOffset);

			HealthBarComponent->SetWorldLocation(FinalAnchorLoc);
		}
	}
}

void ANovaUnit::PushUnit(FVector PushDir, float PushAmount, int32 Depth)
{
	// 최대 연쇄 밀림 횟수 제한 (무한 루프 및 프레임 드랍 방지)
	if (Depth > 4 || bIsDead) return;

	FHitResult Hit;
	// 스윕(Sweep)을 켜서 다른 유닛/지형과 부딪히는지 확인하며 이동
	AddActorWorldOffset(PushDir * PushAmount, true, &Hit);

	// 다른 무언가에 부딪혀서 이동이 막혔다면 연쇄 밀어내기 시도
	if (Hit.bBlockingHit)
	{
		if (ANovaUnit* HitUnit = Cast<ANovaUnit>(Hit.GetActor()))
		{
			// 부딪힌 대상이 같은 팀 유닛인지 확인
			if (HitUnit->GetTeamID() == GetTeamID())
			{
				ECommandType HitCommand = ECommandType::None;
				if (ANovaAIController* HitAI = Cast<ANovaAIController>(HitUnit->GetController()))
				{
					HitCommand = HitAI->GetCurrentCommand();
				}

				// 대상이 비켜줄 수 있는 상태인지 확인
				if (HitCommand == ECommandType::None ||
					HitCommand == ECommandType::Stop ||
					HitCommand == ECommandType::Attack ||
					HitCommand == ECommandType::Patrol)
				{
					// 부딪힌 유닛도 같은 방향으로 밀어냄
					HitUnit->PushUnit(PushDir, PushAmount, Depth + 1);
				}
			}
		}
	}
}

void ANovaUnit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 1. 설정된 클래스 정보를 바탕으로 부품 생성 및 할당 (에디터 프리뷰용)
	ConstructUnitParts();

	// 2. 부품들 실제 소켓에 정렬 부착
	InitializePartAttachments();

	// 3. 에디터 프리뷰 환경에서만 미리 계산 반영 (런타임 이중 초기화 방지)
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		InitializeAttributesFromParts();
	}

	// 캡슐 반지름에 맞춰 위젯 크기 동적 설정
	if (GetCapsuleComponent() && SelectionComponent)
	{
		// 캡슐 반지름
		float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();

		// 캡슐 높이의 절반
		float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		// 공중 유닛 여부
		bool bIsAir = (MovementType == ENovaMovementType::Air);
		
		// SelectionComponent 초기화
		SelectionComponent->SetupSelection(Radius, HalfHeight, bIsAir);

		// 팀 색상 즉시 반영
		InitializeUIColors();
		SelectionComponent->SetTeamColor(CachedUIColor);
	}
}

void ANovaUnit::BeginPlay()
{
	Super::BeginPlay();

	// 런타임 시작 시 다시 한번 초기화 및 부착 보강
	ConstructUnitParts();
	InitializePartAttachments();

	// 3. 모든 부품이 부착된 후 스탯 합산 및 초기화
	InitializeAttributesFromParts();
	InitializeAbilitiesFromParts();

	if (AbilitySystemComponent)
	{
		// ASC 초기화 (ActorInfo 설정)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 속성 변경 콜백 등록
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
		                      .AddUObject(this, &ANovaUnit::OnHealthChanged);
	}

	// 초기 Yaw 설정
	LastYaw = GetActorRotation().Yaw;

	// UI에 적용할 색상 저장
	InitializeUIColors();

	// Selection 컴포넌트 초기 설정
	if (SelectionComponent && GetCapsuleComponent())
	{
		float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
		float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		bool bIsAir = (MovementType == ENovaMovementType::Air);

		SelectionComponent->SetupSelection(Radius, HalfHeight, bIsAir);
		SelectionComponent->SetTeamColor(CachedUIColor);
	}

	// 초기 체력바 상태 설정
	UpdateHealthBar();

	// --- 랠리 포인트 이동 로직 추가 ---
	// 초기 랠리 포인트가 설정되어 있다면 해당 위치로 이동 명령을 내립니다.
	if (!InitialRallyLocation.IsNearlyZero())
	{
		FCommandData MoveCmd;
		MoveCmd.CommandType = ECommandType::Move;
		MoveCmd.TargetLocation = InitialRallyLocation;

		IssueCommand(MoveCmd);
		NOVA_LOG(Log, "Unit %s is moving to initial rally point: %s", *GetName(), *InitialRallyLocation.ToString());
	}
}

void ANovaUnit::SetAssemblyData(const FNovaUnitAssemblyData& Data)
{
	UnitName = Data.UnitName;
	LegsPartClass = Data.LegsClass;
	BodyPartClass = Data.BodyClass;
	WeaponPartClass = Data.WeaponClass;

	NOVA_LOG(Log, "SetAssemblyData: %s (Legs: %s, Body: %s, Weapon: %s)",
	         *UnitName,
	         LegsPartClass ? *LegsPartClass->GetName() : TEXT("NULL"),
	         BodyPartClass ? *BodyPartClass->GetName() : TEXT("NULL"),
	         WeaponPartClass ? *WeaponPartClass->GetName() : TEXT("NULL"));
}


void ANovaUnit::UpdateBodyRotation(float DeltaTime)
{
	// 1. 몸통 파츠가 없으면 중단합니다.
	if (!CurrentBodyPart)
	{
		return;
	}

	// 현재 몸통의 세계 회전값
	FRotator CurrentRotation = CurrentBodyPart->GetActorRotation();
	FRotator TargetRotation;

	// 1. [최적화] 블랙보드 조회 대신 캐싱된 CurrentTarget만 확인합니다.
	if (IsValid(CurrentTarget))
	{
		// 타겟 방향 계산
		FVector Direction = CurrentTarget->GetActorLocation() - GetActorLocation();
		TargetRotation = Direction.Rotation();
	}
	else
	{
		// 2. 타겟이 없으면 유닛의 현재 정면(다리/뿌리 방향)으로 회전합니다.
		TargetRotation = GetActorRotation();
	}

	// 모델 보정 (Y축 정면 기준 -90도)
	TargetRotation.Yaw -= 90.0f;
	// 좌우(Yaw) 회전만 허용
	TargetRotation.Pitch = 0.0f;
	TargetRotation.Roll = 0.0f;

	// 3. 부드러운 회전 보간 (RInterpTo)
	FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		DeltaTime,
		BodyRotationInterpSpeed
	);

	// 계산된 회전값 적용
	CurrentBodyPart->SetActorRotation(NewRotation);
}

void ANovaUnit::UpdateWeaponAiming(float DeltaTime)
{
	// 1. 타겟이 유효하지 않으면 모든 무기를 정면(0도)으로 복귀시킵니다.
	if (!IsValid(CurrentTarget))
	{
		for (ANovaPart* WeaponPart : CurrentWeaponParts)
		{
			if (WeaponPart)
			{
				WeaponPart->SetTargetPitch(0.0f);
				WeaponPart->UpdateAiming(DeltaTime);
			}
		}
		return;
	}

	// 2. 각 무기 부품별로 타겟을 향한 각도를 계산합니다.
	for (ANovaPart* WeaponPart : CurrentWeaponParts)
	{
		if (WeaponPart)
		{
			// 무기의 월드 위치와 타겟 위치 사이의 방향 벡터 계산
			FVector WeaponLocation = WeaponPart->GetActorLocation();
			FVector TargetLocation = CurrentTarget->GetActorLocation();
			FVector Direction = TargetLocation - WeaponLocation;

			// 방향 벡터를 회전값으로 변환하여 Pitch 추출
			FRotator LookAtRot = Direction.Rotation();

			// 조준 각도 전달
			WeaponPart->SetTargetPitch(LookAtRot.Pitch);

			// 무기 내부의 업데이트 로직 실행
			WeaponPart->UpdateAiming(DeltaTime);
		}
	}
}

EBlackboardNotificationResult ANovaUnit::OnTargetActorChanged(const UBlackboardComponent& Blackboard,
                                                              FBlackboard::FKey KeyID)
{
	// 1. 블랙보드에서 새로운 타겟을 꺼내 멤버 변수에 저장합니다.
	// (이 함수는 타겟이 바뀔 때만 딱 한 번 실행되므로 매우 효율적입니다.)
	CurrentTarget = Cast<AActor>(Blackboard.GetValueAsObject(TEXT("TargetActor")));

	// 2. 계속해서 감시를 유지하겠다는 결과를 반환합니다.
	return EBlackboardNotificationResult::ContinueObserving;
}

void ANovaUnit::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (ANovaAIController* AIC = Cast<ANovaAIController>(NewController))
	{
		UBlackboardComponent* BB = AIC->GetBlackboardComponent();
		if (BB && BB->GetBlackboardAsset())
		{
			CachedBlackboard = BB;
			// 1. 키 ID를 구해서 멤버 변수에 저장합니다.
			TargetActorKeyID = BB->GetBlackboardAsset()->GetKeyID(TEXT("TargetActor"));

			if (TargetActorKeyID != FBlackboard::InvalidKey)
			{
				// 2. 저장한 ID를 사용하여 옵저버 등록
				TargetActorObserverHandle = BB->RegisterObserver(
					TargetActorKeyID,
					this,
					FOnBlackboardChangeNotification::CreateUObject(this, &ANovaUnit::OnTargetActorChanged)
				);
			}

			CurrentTarget = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));
		}
	}
}

void ANovaUnit::UnPossessed()
{
	// 중요: 저장해둔 TargetActorKeyID를 사용하여 해제합니다.
	if (CachedBlackboard && TargetActorObserverHandle.IsValid() && TargetActorKeyID != FBlackboard::InvalidKey)
	{
		// 사용자님께서 확인하신 2개의 인자(KeyID, Handle)를 정확히 전달합니다.
		CachedBlackboard->UnregisterObserver(TargetActorKeyID, TargetActorObserverHandle);
		TargetActorObserverHandle.Reset();
	}

	CachedBlackboard = nullptr;
	CurrentTarget = nullptr;
	TargetActorKeyID = FBlackboard::InvalidKey;

	Super::UnPossessed();
}

void ANovaUnit::ConstructUnitParts()
{
	UNovaObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UNovaObjectPoolSubsystem>();

	// 1. 다리(Legs) 할당
	if (LegsPartClass)
	{
		// 클래스가 바뀌었거나 아직 없는 경우
		if (!CurrentLegsPart || CurrentLegsPart->GetClass() != LegsPartClass)
		{
			if (CurrentLegsPart)
			{
				if (PoolSubsystem) PoolSubsystem->ReturnToPool(CurrentLegsPart);
				else CurrentLegsPart->Destroy();
			}
			
			if (PoolSubsystem)
			{
				CurrentLegsPart = Cast<ANovaPart>(PoolSubsystem->SpawnFromPool(LegsPartClass, GetActorTransform()));
			}
			else
			{
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				CurrentLegsPart = GetWorld()->SpawnActor<ANovaPart>(LegsPartClass, Params);
			}

			if (CurrentLegsPart)
			{
				CurrentLegsPart->SetOwner(this);
				CurrentLegsPart->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
				CurrentLegsPart->SetPartDataTable(PartDataTable);
			}
		}

		// 클래스 변경 여부와 상관없이 오프셋은 항상 업데이트 (에디터 실시간 반영용)
		if (CurrentLegsPart)
		{
			CurrentLegsPart->SetOwner(this);
			CurrentLegsPart->SetActorRelativeLocation(LegsOffset);
		}
	}
	else
	{
		// 클래스가 None인 경우 기존 부품 제거
		if (CurrentLegsPart)
		{
			if (PoolSubsystem) PoolSubsystem->ReturnToPool(CurrentLegsPart);
			else CurrentLegsPart->Destroy();
			CurrentLegsPart = nullptr;
		}
	}

	// 2. 몸통(Body) 할당
	if (BodyPartClass)
	{
		if (!CurrentBodyPart || CurrentBodyPart->GetClass() != BodyPartClass)
		{
			if (CurrentBodyPart)
			{
				if (PoolSubsystem) PoolSubsystem->ReturnToPool(CurrentBodyPart);
				else CurrentBodyPart->Destroy();
			}

			if (PoolSubsystem)
			{
				CurrentBodyPart = Cast<ANovaPart>(PoolSubsystem->SpawnFromPool(BodyPartClass, GetActorTransform()));
			}
			else
			{
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				CurrentBodyPart = GetWorld()->SpawnActor<ANovaPart>(BodyPartClass, Params);
			}

			if (CurrentBodyPart && CurrentLegsPart)
			{
				CurrentBodyPart->SetOwner(this);
				UPrimitiveComponent* LegsMesh = CurrentLegsPart->GetMainMesh();
				if (LegsMesh)
				{
					CurrentBodyPart->AttachToComponent(LegsMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, BodyTargetSocketName);
					CurrentBodyPart->SetPartDataTable(PartDataTable);
				}
			}
		}
	}
	else
	{
		if (CurrentBodyPart)
		{
			if (PoolSubsystem) PoolSubsystem->ReturnToPool(CurrentBodyPart);
			else CurrentBodyPart->Destroy();
			CurrentBodyPart = nullptr;
		}
	}

	// 3. 무기(Weapons) 할당
	// 기존 무기들 중 클래스가 다른 것들은 풀로 반환 또는 파괴
	for (int32 i = CurrentWeaponParts.Num() - 1; i >= 0; --i)
	{
		if (!CurrentWeaponParts[i] || CurrentWeaponParts[i]->GetClass() != WeaponPartClass)
		{
			if (CurrentWeaponParts[i])
			{
				if (PoolSubsystem) PoolSubsystem->ReturnToPool(CurrentWeaponParts[i]);
				else CurrentWeaponParts[i]->Destroy();
			}
			CurrentWeaponParts.RemoveAt(i);
		}
	}

	if (WeaponPartClass && CurrentBodyPart)
	{
		UPrimitiveComponent* BodyMesh = CurrentBodyPart->GetMainMesh();
		if (BodyMesh)
		{
			for (int32 i = 0; i < WeaponSocketNames.Num(); ++i)
			{
				const FName& SocketName = WeaponSocketNames[i];
				if (BodyMesh->DoesSocketExist(SocketName))
				{
					// 기존에 이 소켓에 붙어있는 무기가 있는지 확인 (인덱스 기반)
					if (!CurrentWeaponParts.IsValidIndex(i))
					{
						ANovaPart* NewWeapon = nullptr;
						if (PoolSubsystem)
						{
							NewWeapon = Cast<ANovaPart>(PoolSubsystem->SpawnFromPool(WeaponPartClass, GetActorTransform()));
						}
						else
						{
							FActorSpawnParameters Params;
							Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
							NewWeapon = GetWorld()->SpawnActor<ANovaPart>(WeaponPartClass, Params);
						}

						if (NewWeapon)
						{
							NewWeapon->SetOwner(this);
							NewWeapon->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
							NewWeapon->SetPartDataTable(PartDataTable);
							CurrentWeaponParts.Add(NewWeapon);
						}
					}
					else
					{
						// 이미 있다면 부착 상태 및 오너 확인
						CurrentWeaponParts[i]->SetOwner(this);
						CurrentWeaponParts[i]->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
					}
				}
			}
			
			// 남는 무기들 반환
			while (CurrentWeaponParts.Num() > WeaponSocketNames.Num())
			{
				ANovaPart* ExtraWeapon = CurrentWeaponParts.Pop();
				if (ExtraWeapon)
				{
					if (PoolSubsystem) PoolSubsystem->ReturnToPool(ExtraWeapon);
					else ExtraWeapon->Destroy();
				}
			}
		}
	}
	else if (!WeaponPartClass)
	{
		// 무기 클래스가 None이면 모든 무기 제거
		for (ANovaPart* WeaponPart : CurrentWeaponParts)
		{
			if (WeaponPart)
			{
				if (PoolSubsystem) PoolSubsystem->ReturnToPool(WeaponPart);
				else WeaponPart->Destroy();
			}
		}
		CurrentWeaponParts.Empty();
	}
}

void ANovaUnit::InitializePartAttachments()
{
	// ConstructUnitParts에서 이미 부착 로직을 수행하므로 여기서는 보강만 하거나 비워둡니다.
	if (CurrentLegsPart && CurrentBodyPart)
	{
		if (UPrimitiveComponent* LegsMesh = CurrentLegsPart->GetMainMesh())
		{
			CurrentBodyPart->AttachToComponent(LegsMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, BodyTargetSocketName);
		}
	}
}

void ANovaUnit::InitializeAttributesFromParts()
{
	if (!AttributeSet) return;

	float TotalWatt = 0.0f;
	float TotalHealth = 0.0f;
	float TotalAttack = 0.0f;
	float TotalDefense = 0.0f;
	float TotalSpeed = 0.0f;
	float TotalFireRate = 0.0f;
	float TotalSight = 0.0f;
	float TotalRange = 0.0f;
	float TotalMinRange = 0.0f;
	float TotalSplashRange = 0.0f;

	// 수집할 모든 부품 액터들을 리스트업
	TArray<ANovaPart*> PartActors;
	if (CurrentLegsPart) PartActors.Add(CurrentLegsPart);
	if (CurrentBodyPart) PartActors.Add(CurrentBodyPart);

	// 무기 부품 스펙 반영
	if (CurrentWeaponParts.Num() > 0)
	{
		PartActors.Add(CurrentWeaponParts[0]);
	}

	// 각 부품에서 스탯 수집
	for (ANovaPart* Part : PartActors)
	{
		if (Part)
		{
			Part->InitializePartSpec();
			const FNovaPartSpecRow& Spec = Part->GetPartSpec();

			TotalWatt += Spec.Watt;
			TotalHealth += Spec.Health;
			TotalAttack += Spec.Attack;
			TotalDefense += Spec.Defense;
			TotalSpeed += Spec.Speed;
			TotalFireRate += Spec.FireRate;
			TotalSight += Spec.Sight;
			TotalRange += Spec.Range;
			TotalMinRange += Spec.MinRange;
			TotalSplashRange += Spec.SplashRange;

			if (Spec.PartType == ENovaPartType::Legs)
			{
				MovementType = Spec.MovementType;
				if (Spec.CollisionRadius > 0.0f)
				{
					if (UCapsuleComponent* Capsule = GetCapsuleComponent())
					{
						Capsule->SetCapsuleRadius(Spec.CollisionRadius);
						if (NavModifier)
						{
							float ModifierRadius = Spec.CollisionRadius * 0.8f;
							float ModifierHeight = Capsule->GetUnscaledCapsuleHalfHeight();
							NavModifier->FailsafeExtent = FVector(ModifierRadius, ModifierRadius, ModifierHeight);
						}
						if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
						{
							MoveComp->NavAgentProps.AgentRadius = Spec.CollisionRadius;
							if (MovementType == ENovaMovementType::Air)
							{
								MoveComp->NavAgentProps.bCanWalk = false;
								MoveComp->NavAgentProps.bCanFly = true;
							}
							else
							{
								MoveComp->NavAgentProps.bCanWalk = true;
								MoveComp->NavAgentProps.bCanFly = false;
							}
							MoveComp->AvoidanceConsiderationRadius = Spec.CollisionRadius;
							MoveComp->bUseRVOAvoidance = false;
							MoveComp->UpdateNavAgent(*this);
						}
					}
				}
			}
			else if (Spec.PartType == ENovaPartType::Weapon)
			{
				TargetType = Spec.TargetType;
			}
		}
	}

	// AttributeSet 초기화
	AttributeSet->InitWatt(TotalWatt);
	AttributeSet->InitHealth(TotalHealth);
	AttributeSet->InitMaxHealth(TotalHealth); // MaxHealth도 동일하게 초기화
	AttributeSet->InitAttack(TotalAttack);
	AttributeSet->InitDefense(TotalDefense);
	AttributeSet->InitSpeed(TotalSpeed);
	AttributeSet->InitFireRate(TotalFireRate);
	AttributeSet->InitSight(TotalSight);
	AttributeSet->InitRange(TotalRange);
	AttributeSet->InitMinRange(TotalMinRange);
	AttributeSet->InitSplashRange(TotalSplashRange);

	// 디버깅: 속도가 0일 경우 테스트를 위해 최소 속도 부여
	// if (TotalSpeed <= 0.0f)
	// {
	// 	NOVA_SCREEN(Error, "Unit Speed is 0! Forcing speed to 300 for testing.");
	// 	TotalSpeed = 300.0f;
	// }

	// 최대 이동 속도 설정 반영
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = TotalSpeed;
		GetCharacterMovement()->MaxFlySpeed = TotalSpeed;

		// AI 이동 시 가속도를 무시하고 즉각적인 방향 전환과 최고 속도 도달을 허용 (빠릿빠릿한 조작감의 핵심)
		GetCharacterMovement()->bRequestedMoveUseAcceleration = false;

		// 공중 유닛 설정
		if (MovementType == ENovaMovementType::Air)
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Flying);
			GetCharacterMovement()->bCheatFlying = true; // 중력 영향 배제 보강

			// 비행 시 굼뜨게 움직이는 현상 방지: 마찰력을 걷기 수준으로 올림
			GetCharacterMovement()->MaxAcceleration = TotalSpeed * 10.0f; // 더 강력한 가속도
			GetCharacterMovement()->BrakingDecelerationFlying = TotalSpeed * 10.0f; // 즉각적인 제동
			GetCharacterMovement()->BrakingFrictionFactor = 2.0f;
			GetCharacterMovement()->FallingLateralFriction = 8.0f; // 공중 미끄러짐 방지

			// 가상 평면 NavMesh를 타기 위해 평면 제약을 켭니다.
			GetCharacterMovement()->bConstrainToPlane = true;
			GetCharacterMovement()->bSnapToPlaneAtStart = true;

			// 생성 위치를 하늘(Z = DefaultAirZ)로 고정
			FVector CurrentLoc = GetActorLocation();
			SetActorLocation(FVector(CurrentLoc.X, CurrentLoc.Y, DefaultAirZ));
		}
		else
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			GetCharacterMovement()->bConstrainToPlane = true;

			// 지상 유닛 가속도 보정
			GetCharacterMovement()->MaxAcceleration = TotalSpeed * 10.0f;
			GetCharacterMovement()->BrakingDecelerationWalking = TotalSpeed * 10.0f;
		}
	}

	// NOVA_SCREEN(Log, "Unit Stats Initialized: %s | HP(%.f), Speed(%.f), Type(%s)",
	//             *UnitName, TotalHealth, TotalSpeed,
	//             MovementType == ENovaMovementType::Air ? TEXT("Air") : TEXT("Ground"));
}


bool ANovaUnit::IsTargetInRange(const AActor* Target, float Range) const
{
	if (!IsValid(Target)) return false;

	FVector MyLoc = GetActorLocation();
	FVector TargetLoc = Target->GetActorLocation();

	// 1. 수평 거리 체크 (XY 평면상 거리)
	float DistSqXY = FVector::DistSquaredXY(MyLoc, TargetLoc);

	// 타겟의 충돌체 크기를 고려하여 판정 완화 (타겟의 루트 컴포넌트가 PrimitiveComponent인 경우)
	float TargetRadius = 0.0f;
	if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Target->GetRootComponent()))
	{
		TargetRadius = Capsule->GetScaledCapsuleRadius();
	}

	float AdjustedRange = Range + TargetRadius;
	if (DistSqXY > FMath::Square(AdjustedRange))
	{
		return false; // 수평 거리가 사거리를 벗어남
	}

	return true;
}

UAbilitySystemComponent* ANovaUnit::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ANovaUnit::OnSelected()
{
	bIsSelected = true;

	// 가시성 제어를 Component가 수행
	if (SelectionComponent)
	{
		SelectionComponent->SetSelectionVisible(true);
		SelectionComponent->SetTeamColor(CachedUIColor);
	}
	UE_LOG(LogTemp, Log, TEXT("Unit Selected: %s"), *GetName());
}

void ANovaUnit::OnDeselected()
{
	bIsSelected = false;

	// Widget을 비가시화
	if (SelectionComponent)
	{
		SelectionComponent->SetSelectionVisible(false);
	}
	UE_LOG(LogTemp, Log, TEXT("Unit Deselected: %s"), *GetName());
}

bool ANovaUnit::IsSelectable() const
{
	// 죽었거나 안개 속에 있으면 선택 불가
	if (bIsDead || !bIsVisibleByFog) return false;

	// 로컬 플레이어 팀 확인
	int32 LocalPlayerTeamID = -1;
	if (auto* PC = GetWorld()->GetFirstPlayerController())
	{
		if (auto* PS = PC->GetPlayerState<ANovaPlayerState>())
		{
			LocalPlayerTeamID = PS->GetTeamID();
		}
	}

	// 아군이면 항상 선택 가능, 적군이면 시야 내에 있을 때만 선택 가능
	if (TeamID == LocalPlayerTeamID) return true;

	return bIsVisibleByFog;
}

void ANovaUnit::IssueCommand(const FCommandData& CommandData)
{
	// 죽은 상태에서는 명령 수행 불가
	if (bIsDead) return;

	// 공격 명령 하달시 타겟 유효성(지상/공중) 필터링
	if (CommandData.CommandType == ECommandType::Attack && CommandData.TargetActor)
	{
		bool bCanAttackTarget = true;

		// 1. 공격 가능 대상 확인 (ASC 구현 여부)
		if (CommandData.TargetActor->GetInterfaceAddress(UAbilitySystemInterface::StaticClass()) == nullptr)
		{
			bCanAttackTarget = false;
		}
		else
		{
			ANovaUnit* TargetUnit = Cast<ANovaUnit>(CommandData.TargetActor);
			if (TargetUnit)
			{
				ENovaMovementType TargetMoveType = TargetUnit->GetMovementType();
				switch (TargetType)
				{
				case ENovaTargetType::GroundOnly:
					bCanAttackTarget = (TargetMoveType == ENovaMovementType::Ground);
					break;
				case ENovaTargetType::AirOnly:
					bCanAttackTarget = (TargetMoveType == ENovaMovementType::Air);
					break;
				case ENovaTargetType::All:
					bCanAttackTarget = true;
					break;
				case ENovaTargetType::None:
					bCanAttackTarget = false;
					break;
				}
			}
			else
			{
				// 유닛이 아니지만 ASC를 가진 대상(기지 등)은 기본적으로 지상 타겟으로 간주
				bCanAttackTarget = (TargetType != ENovaTargetType::AirOnly);
			}
		}

		if (!bCanAttackTarget)
		{
			NOVA_LOG(Warning, "Unit %s cannot attack %s due to TargetType mismatch or invalid target.", *GetName(),
			         *CommandData.TargetActor->GetName());
			return; // 공격 불가능한 대상이면 명령 무시
		}
	}

	// 1. 전용 AI 컨트롤러에게 명령 전달 (이동, 추적 등)
	if (INovaCommandInterface* CmdInterface = Cast<INovaCommandInterface>(GetController()))
	{
		CmdInterface->IssueCommand(CommandData);
	}

	NOVA_LOG(Log, "Unit Received Command: Type %d", (int32)CommandData.CommandType);

	// 2. 무기(Weapon) 애니메이션 재생: 공격 명령 시
	if (CommandData.CommandType == ECommandType::Attack)
	{
		for (ANovaPart* WeaponPart : CurrentWeaponParts)
		{
			if (WeaponPart)
			{
				WeaponPart->PlayAttackAnimation();
			}
		}
	}
}

void ANovaUnit::Die()
{
	if (bIsDead)
	{
		return;
	}

	// 유닛 사망 처리 로직
	NOVA_LOG(Warning, "Unit Died: %s", *GetName());
	bIsDead = true;

	// NovaPlayerController에 선택 해제 요청
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (ANovaPlayerController* NovaPC = Cast<ANovaPlayerController>(PC))
		{
			NovaPC->NotifyTargetUnselectable(this);
		}
	}

	// 자원 반납 (인구수 -1, 자신의 와트 비용만큼 차감)
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		float UnitWatt = ASC->GetNumericAttribute(UNovaAttributeSet::GetWattAttribute());

		UWorld* World = GetWorld();
		if (World)
		{
			for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++
			     Iterator)
			{
				if (APlayerController* PC = Iterator->Get())
				{
					if (ANovaPlayerState* PS = PC->GetPlayerState<ANovaPlayerState>())
					{
						// INovaTeamInterface를 통해 팀 ID를 확인합니다.
						INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(PS);
						if (TeamInterface && TeamInterface->GetTeamID() == TeamID)
						{
							if (UNovaResourceComponent* ResourceComp = PS->FindComponentByClass<
								UNovaResourceComponent>())
							{
								ResourceComp->UpdatePopulation(-1.0f, -UnitWatt);
								NOVA_LOG(Log, "Unit %s (Watt: %.f) died. Resources returned to team %d", *GetName(),
								         UnitWatt, TeamID);
								break;
							}
						}
					}
				}
			}
		}
	}

	// AI 동작 정지 요청
	if (ANovaAIController* AIC = Cast<ANovaAIController>(GetController()))
	{
		AIC->OnPawnDeath();
	}

	// 내비게이션 장애물 해제 (죽은 유닛은 길을 막지 않음)
	SetNavigationObstacle(false);

	// 충돌 비활성화 및 소멸 처리 (필요에 따라 래그돌 또는 파편화 연출 추가 가능)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();

	// 모든 부품에 사망 상태 전달
	TArray<ANovaPart*> PartActors;
	if (CurrentLegsPart) PartActors.Add(CurrentLegsPart);
	if (CurrentBodyPart) PartActors.Add(CurrentBodyPart);
	for (ANovaPart* WeaponPart : CurrentWeaponParts)
	{
		if (WeaponPart) PartActors.Add(WeaponPart);
	}

	for (ANovaPart* Part : PartActors)
	{
		if (Part)
		{
			Part->SetIsDead(true);
		}
	}

	// 체력바 컴포넌트 숨기기
	if (HealthBarComponent)
	{
		HealthBarComponent->SetVisibility(false);
		// 컴포넌트 비활성화
		HealthBarComponent->Deactivate();
	}

	// TODO: 팀원들과 상의하여 유닛 소멸 방식 결정 (오브젝트 풀링 적용?)
	// Destroy();
	// 2초 후 오브젝트 풀로 반환 (사망 애니메이션 대기)
	FTimerHandle ReturnToPoolTimer;
	GetWorld()->GetTimerManager().SetTimer(ReturnToPoolTimer, [this]()
	{
		if (UNovaObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UNovaObjectPoolSubsystem>())
		{
			PoolSubsystem->ReturnToPool(this);
		}
		else
		{
			Destroy();
		}
	}, 2.0f, false);
}

void ANovaUnit::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	// 체력이 0 이하가 되면 Die() 호출
	if (Data.NewValue <= 0.0f) Die();

	// 실시간 체력바 업데이트 호출
	UpdateHealthBar();
}

void ANovaUnit::InitializeAbilitiesFromParts()
{
	if (!AbilitySystemComponent) return;

	// 기존 부여된 모든 어빌리티 제거 (풀링 재사용 시 중복 방지)
	AbilitySystemComponent->ClearAllAbilities();

	// 중복 제거를 위한 Set 사용
	TSet<TSubclassOf<class UNovaGameplayAbility>> UniqueAbilities;

	// 수집할 모든 부품 액터 리스트업
	TArray<ANovaPart*> PartActors;
	if (CurrentLegsPart) PartActors.Add(CurrentLegsPart);
	if (CurrentBodyPart) PartActors.Add(CurrentBodyPart);

	// 무기는 여러 소켓에 붙어있을 수 있지만 모두 같은 무기만 붙으므로 무기 중 하나의 어빌리티만 등록한다.
	if (CurrentWeaponParts.Num() > 0)
	{
		PartActors.Add(CurrentWeaponParts[0]);
	}

	// 각 부품에서 어빌리티 클래스 추출
	for (ANovaPart* Part : PartActors)
	{
		if (Part)
		{
			const FNovaPartSpecRow& Spec = Part->GetPartSpec();
			for (auto AbilityClass : Spec.AbilityClasses)
			{
				if (AbilityClass)
				{
					UniqueAbilities.Add(AbilityClass);
				}
			}
		}
	}

	// 수집된 고유 어빌리티들을 ASC에 부여
	for (auto AbilityClass : UniqueAbilities)
	{
		if (AbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, -1, this));
		}
	}
}


void ANovaUnit::UpdateHealthBar()
{
	if (!HealthBarComponent || !AttributeSet) return;

	HealthBarComponent->UpdateHealthBar(AttributeSet->GetHealth(),
	                                    AttributeSet->GetMaxHealth(),
	                                    CachedUIColor);
}

void ANovaUnit::InitializeUIColors()
{
	int32 LocalPlayerTeamID = -1;
	if (auto* PC = GetWorld()->GetFirstPlayerController())
	{
		if (auto* PS = PC->GetPlayerState<ANovaPlayerState>())
		{
			LocalPlayerTeamID = PS->GetTeamID();
		}
	}

	// 색상 결정 및 캐싱
	CachedUIColor = FLinearColor::Red; // 적군
	if (TeamID == LocalPlayerTeamID) CachedUIColor = FLinearColor::Green; // 아군
	else if (TeamID == NovaTeam::None || LocalPlayerTeamID == -1) CachedUIColor = FLinearColor::Yellow; // 중립
}

void ANovaUnit::SetFogVisibility(bool bVisible)
{
	if (bIsVisibleByFog == bVisible) return;
	bIsVisibleByFog = bVisible;

	// 시각적 처리
	SetActorHiddenInGame(!bVisible);

	// 마우스 클릭(Visibility)만 선택적으로 무시
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, bVisible ? ECR_Block : ECR_Ignore);
	}

	// 체력바 위젯도 안개 가시성에 따라 숨김/표시
	if (HealthBarComponent)
	{
		HealthBarComponent->SetFogVisibility(bVisible);
	}

	// 선택된 상태에서 안개 속으로 사라졌을 때
	if (!bVisible && bIsSelected)
	{
		// 로컬 플레이어 컨트롤러를 찾아 알림을 보냄
		if (auto* NovaPC = Cast<ANovaPlayerController>(GetWorld()->GetFirstPlayerController()))
		{
			NovaPC->NotifyTargetUnselectable(this);
		}
	}

	// 선택 표시 위젯이 켜져 있었다면 강제로 끄기
	if (!bVisible && SelectionComponent)
	{
		SelectionComponent->SetSelectionVisible(false);
	}
}

void ANovaUnit::SetNavigationObstacle(bool bIsObstacle)
{
	// 죽은 유닛은 어떠한 경우에도 장애물이 될 수 없음
	if (bIsDead && bIsObstacle) bIsObstacle = false;

	// 상태 변화가 있을 때만 실행하여 불필요한 부하 방지
	if (bIsNavigationObstacle == bIsObstacle) return;
	bIsNavigationObstacle = bIsObstacle;

	if (NavModifier)
	{
		// 장애물 상태에 따라 영역 클래스 설정 (UnitArea: 고비용, Default: 일반)
		TSubclassOf<UNavArea> NewAreaClass = bIsObstacle
			                                     ? UNovaNavArea_Unit::StaticClass()
			                                     : UNavArea_Default::StaticClass();

		// 영역 클래스 설정 및 활성화/비활성화 처리
		NavModifier->SetAreaClass(NewAreaClass);

		if (bIsObstacle)
		{
			NavModifier->Activate(true);
		}
		else
		{
			NavModifier->Deactivate();
		}

		// 즉시 NavMesh 반영을 유도
		NavModifier->RefreshNavigationModifiers();

		NOVA_LOG(Log, "Unit %s NavModifier %hs", *GetName(), bIsObstacle ? "Activated" : "Deactivated");
	}
}

void ANovaUnit::OnSpawnFromPool_Implementation()
{
	bIsDead = false;

	// 1. [중요] 풀에서 부활한 경우, 새로 주입된 조립 데이터에 맞춰 부품 재조립 및 초기화 수행
	if (IsActorInitialized())
	{
		ConstructUnitParts();
		InitializePartAttachments();
		InitializeAttributesFromParts();

		// ASC 상태 초기화 (InitAbilityActorInfo를 통해 새로운 데이터에 맞게 갱신)
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->InitAbilityActorInfo(this, this);
			InitializeAbilitiesFromParts();
		}

		// UI 및 캐싱 데이터 초기화
		InitializeUIColors();
		UpdateSelectionCircleTransform();
		UpdateHealthBarTransform();
		UpdateHealthBarLength();
	}

	// 2. 체력 및 스탯 원상복구 (재조립 이후 최종 MaxHealth 기준으로 채움)
	if (AttributeSet)
	{
		AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
	}

	// 3. 물리/충돌/이동 복구
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// [수정] 이전 생의 이동 제약(Plane Constraint)을 완전히 초기화하여 고도 문제를 방지합니다.
		MoveComp->bConstrainToPlane = false;
		MoveComp->SetPlaneConstraintOrigin(FVector::ZeroVector);
		MoveComp->StopMovementImmediately();

		MoveComp->SetMovementMode(MovementType == ENovaMovementType::Air ? MOVE_Flying : MOVE_Walking);

		// 지상 유닛인 경우 즉시 바닥으로 스냅되도록 유도 (공중 유닛이었을 경우 대비)
		if (MovementType == ENovaMovementType::Ground)
		{
			MoveComp->bConstrainToPlane = true;
			// 현재 스폰 위치(Z)를 기준으로 평면 제약 설정
			MoveComp->SetPlaneConstraintOrigin(GetActorLocation());
		}
	}

	// 4. 모든 부품 상태 부활
	TArray<ANovaPart*> PartActors;
	if (CurrentLegsPart) PartActors.Add(CurrentLegsPart);
	if (CurrentBodyPart) PartActors.Add(CurrentBodyPart);
	for (ANovaPart* WeaponPart : CurrentWeaponParts)
	{
		if (WeaponPart) PartActors.Add(WeaponPart);
	}

	for (ANovaPart* Part : PartActors)
	{
		if (Part)
		{
			Part->SetIsDead(false);

			if (Part->GetPartSpec().PartType == ENovaPartType::Weapon)
			{
				Part->SetTargetPitch(0.0f);
			}
		}
	}

	// 5. AI 컨트롤러 및 비헤이비어 트리 재시작
	if (ANovaAIController* AIC = Cast<ANovaAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			BB->SetValueAsEnum(FName("CommandType"), (uint8)ECommandType::None);
			BB->ClearValue(FName("TargetActor"));
			BB->ClearValue(FName("TargetLocation"));
		}

		AIC->RestartLogic();
	}

	// 6. UI 상태 최종 초기화
	// [핵심] bIsVisibleByFog를 반대값으로 설정하여 SetFogVisibility(true)가 반드시 실행되도록 강제합니다.
	bIsVisibleByFog = false; 
	SetFogVisibility(true); 

	if (ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		SetHealthBarVisibilityOption(PC->GetShowHealthBars());
	}

	UpdateHealthBar();

	// 7. 랠리 포인트 이동
	if (!InitialRallyLocation.IsNearlyZero())
	{
		FCommandData MoveCmd;
		MoveCmd.CommandType = ECommandType::Move;
		MoveCmd.TargetLocation = InitialRallyLocation;
		IssueCommand(MoveCmd);
	}

	NOVA_LOG(Log, "Unit %s Re-initialized from Pool with new assembly.", *GetName());
}

void ANovaUnit::OnReturnToPool_Implementation()
{
	// 1. 선택 해제
	if (bIsSelected)
	{
		OnDeselected();
	}

	// 2. 이동 및 AI 정지
	if (ANovaAIController* AIC = Cast<ANovaAIController>(GetController()))
	{
		AIC->StopMovementOptimized();
		if (UBrainComponent* Brain = AIC->GetBrainComponent())
		{
			Brain->StopLogic("ReturnToPool");
		}
	}

	// 3. 물리 및 이동 제약 초기화 (풀에 들어갈 때 상태 깨끗하게 비움)
	SetNavigationObstacle(false);
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->bConstrainToPlane = false;
		MoveComp->SetPlaneConstraintOrigin(FVector::ZeroVector);
	}

	// 4. UI 숨김 및 상태 초기화
	if (HealthBarWidget)
	{
		HealthBarWidget->SetVisibility(false);
	}
	
	// 다음 사용 시 SetFogVisibility가 정상 작동하도록 초기값(true)으로 리셋
	bIsVisibleByFog = true;

	// 5. 조립 데이터 및 파츠 액터 초기화 (풀로 반환)
	UNovaObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UNovaObjectPoolSubsystem>();
	if (PoolSubsystem)
	{
		if (CurrentLegsPart)
		{
			PoolSubsystem->ReturnToPool(CurrentLegsPart);
			CurrentLegsPart = nullptr;
		}
		if (CurrentBodyPart)
		{
			PoolSubsystem->ReturnToPool(CurrentBodyPart);
			CurrentBodyPart = nullptr;
		}
		for (ANovaPart* WeaponPart : CurrentWeaponParts)
		{
			if (WeaponPart) PoolSubsystem->ReturnToPool(WeaponPart);
		}
		CurrentWeaponParts.Empty();
	}

	LegsPartClass = nullptr;
	BodyPartClass = nullptr;
	WeaponPartClass = nullptr;

	NOVA_LOG(Log, "Unit %s Returned to Pool.", *GetName());
}
