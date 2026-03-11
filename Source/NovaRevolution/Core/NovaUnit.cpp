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
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/NovaAttributeSet.h"
#include "GAS/Abilities/NovaGameplayAbility.h"
#include "Player/NovaPlayerController.h"

ANovaUnit::ANovaUnit()
{
	PrimaryActorTick.bCanEverTick = true;

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

	// --- 차일드 액터 컴포넌트 초기화 (런타임 생성용) ---
	// Legs(다리)는 캡슐(Root)에 직접 부착
	LegsPartComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("LegsPartComponent"));
	LegsPartComponent->SetupAttachment(GetRootComponent());

	// Body(몸통)는 다리의 소켓에 부착될 준비 (BeginPlay에서 처리)
	BodyPartComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("BodyPartComponent"));
	BodyPartComponent->SetupAttachment(LegsPartComponent);

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
	SelectionWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("SelectionWidget"));
	SelectionWidget->SetupAttachment(RootComponent);

	// 부모 캡슐의 스케일과 회전이 위젯을 조절하지 못하게 차단
	SelectionWidget->SetUsingAbsoluteScale(true);
	SelectionWidget->SetUsingAbsoluteRotation(true);

	SelectionWidget->SetWidgetSpace(EWidgetSpace::World); // 월드 공간에 배치
	SelectionWidget->SetRelativeRotation(FRotator(90.f, 0.f, 0.f)); // 바닥에 눕힘
	SelectionWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 충돌 끄기
	SelectionWidget->SetVisibility(false); // 초기에는 숨김

	// 바닥 전용 위젯 생성
	GroundSelectionWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("GroundSelectionWidget"));
	GroundSelectionWidget->SetupAttachment(RootComponent);
	GroundSelectionWidget->SetUsingAbsoluteScale(true);
	GroundSelectionWidget->SetUsingAbsoluteRotation(true);
	GroundSelectionWidget->SetWorldRotation(FRotator(90.f, 0.f, 0.f));
	GroundSelectionWidget->SetWidgetSpace(EWidgetSpace::World);
	GroundSelectionWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundSelectionWidget->SetVisibility(false);

	// 수직 고도 안내선
	HeightIndicatorLine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeightIndicatorLine"));
	HeightIndicatorLine->SetupAttachment(RootComponent);
	// 엔진 기본 큐브 메쉬 로드
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded())
	{
		HeightIndicatorLine->SetStaticMesh(CubeMesh.Object);
	}

	// 항상 수직을 유지하도록 설정
	HeightIndicatorLine->SetUsingAbsoluteRotation(true);
	HeightIndicatorLine->SetWorldRotation(FRotator::ZeroRotator);;
	HeightIndicatorLine->SetCastShadow(false); // 선은 그림자를 만들지 않음
	HeightIndicatorLine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeightIndicatorLine->SetVisibility(false);
}

void ANovaUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead) return;

	// 1. 공중 유닛 고도 조절
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

	// 2. 다리(Legs) 부품 애니메이션 데이터 전달
	if (LegsPartComponent)
	{
		if (ANovaPart* LegsActor = Cast<ANovaPart>(LegsPartComponent->GetChildActor()))
		{
			// 이동 속도 전달
			float CurrentSpeed = GetVelocity().Size();
			LegsActor->SetMovementSpeed(CurrentSpeed);

			// 회전 속도 계산 및 전달
			float CurrentYaw = GetActorRotation().Yaw;
			float YawDelta = FMath::FindDeltaAngleDegrees(LastYaw, CurrentYaw);
			float RotationRate = YawDelta / DeltaTime; // 초당 회전 각도

			LegsActor->SetRotationRate(RotationRate);
			LastYaw = CurrentYaw;

			// --- 실시간 속도 확인용 ---
			// NOVA_SCREEN(Log,"Unit: %s | Current Speed: %.2f", *GetName(), CurrentSpeed);
		}
	}

	// 3. 몸통(Body) 및 무기 회전 로직 실행
	UpdateBodyRotation(DeltaTime);
	UpdateWeaponAiming(DeltaTime);

	// 선택된 상태일 때만 실시간으로 바닥 위치 추적 (선택됨, 죽지않음, widget존재)
	if (bIsSelected && !bIsDead)
	{
		UpdateGroundCircleAndLine();
	}
}

void ANovaUnit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 1. 설정된 클래스 정보를 바탕으로 부품 생성 및 할당 (에디터 프리뷰용)
	ConstructUnitParts();

	// 2. 부품들 실제 소켓에 정렬 부착
	InitializePartAttachments();

	// 위젯 크기 설정 함수 호출
	UpdateSelectionCircleTransform();

	// 3. 에디터 프리뷰 환경에서만 미리 계산 반영 (런타임 이중 초기화 방지)
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		InitializeAttributesFromParts();
	}

	// 캡슐 반지름에 맞춰 위젯 크기 동적 설정
	if (GetCapsuleComponent() && SelectionWidget)
	{
		// 캡슐 반지름 가져오기 (스케일 포함)
		float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();

		// 원의 지름 계산 (캡슐보다 약간 더 크게 1.2배 여유)
		float Diameter = Radius * 2.0f * 1.2f;

		// DrawSize를 설정(지름 * 지름)
		SelectionWidget->SetDrawSize(FVector2D(Diameter, Diameter));

		// DrawAtDesiredSize는 수동 설정 시 false
		SelectionWidget->SetDrawAtDesiredSize(false);
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

	// 모든 조립과 캡슐 크기 변경이 끝난 후 위젯 사이즈 갱신 함수 호출
	UpdateSelectionCircleTransform();

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
	// 1. 몸통 컴포넌트가 없으면 중단합니다.
	if (!BodyPartComponent)
	{
		NOVA_LOG(Log, "UpdateBodyRotation: BodyPartComponent is nullptr, UpdateBodyRotation Is Canceled");
		return;
	}

	// 현재 몸통의 세계 회전값
	FRotator CurrentRotation = BodyPartComponent->GetComponentRotation();
	FRotator TargetRotation;

	// 1. [최적화] 블랙보드 조회 대신 캐싱된 CurrentTarget만 확인합니다.
	// IsValid를 통해 타겟이 죽거나 사라졌는지도 동시에 체크합니다.
	if (IsValid(CurrentTarget))
	{
		// 타겟 방향 계산
		FVector Direction = CurrentTarget->GetActorLocation() - GetActorLocation();
		TargetRotation = Direction.Rotation();
	}
	else
	{
		// 2. [기능 개선] 타겟이 없으면 유닛의 현재 정면(다리/뿌리 방향)으로 회전합니다.
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
	BodyPartComponent->SetWorldRotation(NewRotation);
}

void ANovaUnit::UpdateWeaponAiming(float DeltaTime)
{
	// 1. 타겟이 유효하지 않으면 모든 무기를 정면(0도)으로 복귀시킵니다.
	if (!IsValid(CurrentTarget))
	{
		for (auto WeaponComp : WeaponPartComponents)
		{
			if (ANovaPart* WeaponPart = Cast<ANovaPart>(WeaponComp->GetChildActor()))
			{
				WeaponPart->SetTargetPitch(0.0f);
				WeaponPart->UpdateAiming(DeltaTime);
			}
		}
		return;
	}

	// 2. 각 무기 부품별로 타겟을 향한 각도를 계산합니다.
	for (auto WeaponComp : WeaponPartComponents)
	{
		if (ANovaPart* WeaponPart = Cast<ANovaPart>(WeaponComp->GetChildActor()))
		{
			// 무기의 월드 위치와 타겟 위치 사이의 방향 벡터 계산
			FVector WeaponLocation = WeaponComp->GetComponentLocation();
			FVector TargetLocation = CurrentTarget->GetActorLocation();
			FVector Direction = TargetLocation - WeaponLocation;

			// 방향 벡터를 회전값으로 변환하여 Pitch 추출
			FRotator LookAtRot = Direction.Rotation();

			// 조준 각도 전달 (필요 시 -45 ~ 45도 등으로 제한 가능)
			WeaponPart->SetTargetPitch(LookAtRot.Pitch);

			// 무기 내부의 업데이트 로직 실행 (ABP 값 전달 포함)
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
	// 다리와 몸통 부품 할당
	if (LegsPartClass)
	{
		LegsPartComponent->SetChildActorClass(LegsPartClass);
		if (LegsPartComponent->GetChildActor() == nullptr)
		{
			LegsPartComponent->CreateChildActor();
		}
	}

	if (BodyPartClass)
	{
		BodyPartComponent->SetChildActorClass(BodyPartClass);
		if (BodyPartComponent->GetChildActor() == nullptr)
		{
			BodyPartComponent->CreateChildActor();
		}
	}

	// 기존에 동적으로 생성된 무기 컴포넌트들 제거 (중복 생성 방지)
	for (UChildActorComponent* Comp : WeaponPartComponents)
	{
		if (Comp) Comp->DestroyComponent();
	}
	WeaponPartComponents.Empty();

	// 무기(Weapons) 할당 및 컴포넌트 생성
	if (WeaponPartClass && BodyPartComponent)
	{
		if (ANovaPart* BodyActor = Cast<ANovaPart>(BodyPartComponent->GetChildActor()))
		{
			if (UPrimitiveComponent* BodyMesh = BodyActor->GetMainMesh())
			{
				for (int32 i = 0; i < WeaponSocketNames.Num(); ++i)
				{
					const FName& SocketName = WeaponSocketNames[i];
					if (BodyMesh->DoesSocketExist(SocketName))
					{
						FName CompName = FName(*FString::Printf(TEXT("WeaponPartComponent_%d"), i));
						UChildActorComponent* WeaponComp = NewObject<UChildActorComponent>(this, CompName);
						if (WeaponComp)
						{
							WeaponComp->CreationMethod = EComponentCreationMethod::Instance;
							WeaponComp->RegisterComponent();
							WeaponComp->SetChildActorClass(WeaponPartClass);
							WeaponComp->CreateChildActor(); // 명시적으로 자식 액터 생성 호출
							WeaponComp->AttachToComponent(
								BodyMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);

							WeaponPartComponents.Add(WeaponComp);
						}
					}
					else
					{
						NOVA_LOG(Warning, "Weapon Socket '%s' does not exist on Body Mesh of %s",
						         *SocketName.ToString(), *GetName());
					}
				}
			}
		}
		else
		{
			NOVA_LOG(Warning, "ConstructUnitParts: BodyActor is NULL even after CreateChildActor!");
		}
	}
}

void ANovaUnit::InitializePartAttachments()
{
	// 1. 다리(Legs) 내에서 몸통(Body)이 붙을 소켓을 찾음
	if (ANovaPart* LegsActor = Cast<ANovaPart>(LegsPartComponent->GetChildActor()))
	{
		if (UPrimitiveComponent* LegsMesh = LegsActor->GetMainMesh())
		{
			// 몸통을 지정된 소켓에 부착
			BodyPartComponent->AttachToComponent(LegsMesh, FAttachmentTransformRules::SnapToTargetIncludingScale,
			                                     BodyTargetSocketName);
		}
	}

	// 2. 무기 부품들은 ConstructUnitParts에서 이미 몸통의 소켓에 부착되었으므로 추가 로직 불필요
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
	TArray<AActor*> PartActors;
	PartActors.Add(LegsPartComponent->GetChildActor());
	PartActors.Add(BodyPartComponent->GetChildActor());

	// 무기 부품은 여러 개가 붙을 수 있지만 모두 같은 종류를 붙일 예정이므로 스펙은 첫 번째 것 하나만 반영함
	for (auto WeaponComp : WeaponPartComponents)
	{
		if (AActor* WeaponActor = WeaponComp->GetChildActor())
		{
			PartActors.Add(WeaponActor);
			break; // 첫 번째 무기만 추가하고 중단
		}
	}

	// 각 부품에서 스탯 수집
	for (AActor* Actor : PartActors)
	{
		if (ANovaPart* Part = Cast<ANovaPart>(Actor))
		{
			// 중요: 데이터 테이블로부터 스펙이 아직 로드되지 않았을 수 있으므로 강제 초기화 시도
			// (OnConstruction이나 BeginPlay의 실행 순서 문제를 방지하기 위함)
			Part->InitializePartSpec();

			// 데이터 테이블 방식 (PartSpec) 참조
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

			// --- 신규 타입 정보 추출 ---
			if (Spec.PartType == ENovaPartType::Legs)
			{
				MovementType = Spec.MovementType;

				// --- 캡슐 콜라이더 반경 설정 (Legs 전용) ---
				if (Spec.CollisionRadius > 0.0f)
				{
					if (UCapsuleComponent* Capsule = GetCapsuleComponent())
					{
						Capsule->SetCapsuleRadius(Spec.CollisionRadius);

						// --- 네비게이션 및 이동 컴포넌트 데이터 동기화 ---
						if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
						{
							// 네비게이션 시스템이 참조하는 에이전트 반경 업데이트
							MoveComp->NavAgentProps.AgentRadius = Spec.CollisionRadius;

							// 유닛 간 회피(RVO/Crowd) 시 고려할 반경 업데이트
							MoveComp->AvoidanceConsiderationRadius = Spec.CollisionRadius;

							// 변경된 에이전트 설정을 네비게이션 시스템에 알림
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
	if (TotalSpeed <= 0.0f)
	{
		NOVA_SCREEN(Error, "Unit Speed is 0! Forcing speed to 300 for testing.");
		TotalSpeed = 300.0f;
	}

	// 최대 이동 속도 설정 반영
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = TotalSpeed;
		GetCharacterMovement()->MaxFlySpeed = TotalSpeed;

		// 공중 유닛 설정
		if (MovementType == ENovaMovementType::Air)
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Flying);
			GetCharacterMovement()->bCheatFlying = true; // 중력 영향 배제 보강

			// 공중 유닛은 평면 제약 해제 (고도 조절을 위함)
			GetCharacterMovement()->bConstrainToPlane = false;

			// if (AAIController* AIC = Cast<AAIController>(GetController()))
			// {
			// 	if (UPathFollowingComponent* PPathFollowing = AIC->GetPathFollowingComponent())
			// 	{
			// 		// 공중 유닛은 장애물 회피를 위해 NavMesh를 타지 않도록 설정
			// 		// (프로젝트 상황에 따라 하단의 코드가 필요할 수 있음)
			// 	}
			// }
		}
		else
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			GetCharacterMovement()->bConstrainToPlane = true;
		}
	}

	NOVA_SCREEN(Log, "Unit Stats Initialized: %s | HP(%.f), Speed(%.f), Type(%s)",
	            *UnitName, TotalHealth, TotalSpeed,
	            MovementType == ENovaMovementType::Air ? TEXT("Air") : TEXT("Ground"));
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

	// 2. 수직 거리 체크 (Z축 차이)
	// 공중 유닛과 지상 유닛 간의 교전을 위해 수직 허용 오차를 넉넉히 둡니다 (예: 1500.f)
	// float VerticalDist = FMath::Abs(MyLoc.Z - TargetLoc.Z);
	// if (VerticalDist > 1500.0f)
	// {
	// 	return false; // 고도 차이가 너무 커서 사격 불능
	// }

	return true;
}

UAbilitySystemComponent* ANovaUnit::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ANovaUnit::OnSelected()
{
	bIsSelected = true;
	// TODO: 팀원 B가 구현할 하이라이트/데칼 로직이 들어갈 자리
	if (SelectionWidget)
	{
		UpdateSelectionCircleColor();
		SelectionWidget->SetVisibility(true);

		// 바닥 위젯 켜기
		if (GroundSelectionWidget)
		{
			GroundSelectionWidget->SetVisibility(true);
			// 선택 즉시 위치를 한 번 잡아줌
			UpdateGroundCircleAndLine();
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Unit Selected: %s"), *GetName());
}

void ANovaUnit::OnDeselected()
{
	bIsSelected = false;
	// Widget을 비가시화
	if (SelectionWidget) SelectionWidget->SetVisibility(false);
	if (GroundSelectionWidget) GroundSelectionWidget->SetVisibility(false);
	if (HeightIndicatorLine) HeightIndicatorLine->SetVisibility(false);
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
		for (auto WeaponComp : WeaponPartComponents)
		{
			if (ANovaPart* WeaponActor = Cast<ANovaPart>(WeaponComp->GetChildActor()))
			{
				WeaponActor->PlayAttackAnimation();
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
			for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
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

	// 충돌 비활성화 및 소멸 처리 (필요에 따라 래그돌 또는 파편화 연출 추가 가능)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();

	// 모든 부품에 사망 상태 전달
	TArray<AActor*> PartActors;
	PartActors.Add(LegsPartComponent->GetChildActor());
	PartActors.Add(BodyPartComponent->GetChildActor());
	for (auto WeaponComp : WeaponPartComponents) PartActors.Add(WeaponComp->GetChildActor());

	for (AActor* Actor : PartActors)
	{
		if (ANovaPart* Part = Cast<ANovaPart>(Actor))
		{
			Part->SetIsDead(true);
		}
	}

	// TODO: 팀원들과 상의하여 유닛 소멸 방식 결정 (오브젝트 풀링 적용?)
	// Destroy();
}

void ANovaUnit::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	// 체력이 0 이하가 되면 Die() 호출
	if (Data.NewValue <= 0.0f) Die();
}

void ANovaUnit::InitializeAbilitiesFromParts()
{
	if (!AbilitySystemComponent) return;

	// 중복 제거를 위한 Set 사용
	TSet<TSubclassOf<class UNovaGameplayAbility>> UniqueAbilities;

	// 수집할 모든 부품 액터 리스트업
	TArray<AActor*> PartActors;
	if (LegsPartComponent) PartActors.Add(LegsPartComponent->GetChildActor());
	if (BodyPartComponent) PartActors.Add(BodyPartComponent->GetChildActor());

	// 무기는 여러 소켓에 붙어있을 수 있지만 모두 같은 무기만 붙으므로 무기 중 하나의 어빌리티만 등록한다.
	for (auto WeaponComp : WeaponPartComponents)
	{
		if (WeaponComp)
		{
			PartActors.Add(WeaponComp->GetChildActor());
			break;
		}
	}

	// 각 부품에서 어빌리티 클래스 추출
	for (AActor* Actor : PartActors)
	{
		if (ANovaPart* Part = Cast<ANovaPart>(Actor))
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

void ANovaUnit::UpdateSelectionCircleColor()
{
	if (!SelectionWidget) return;

	// 로컬 플레이어 팀 확인
	int32 LocalPlayerTeamID = -1;
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->GetPlayerState<ANovaPlayerState>())
	{
		LocalPlayerTeamID = PC->GetPlayerState<ANovaPlayerState>()->GetTeamID();
	}

	FLinearColor TargetColor = FLinearColor::Red; // 기본 Red (적)
	if (TeamID == LocalPlayerTeamID) TargetColor = FLinearColor::Green; // 내 유닛 Green
	else if (TeamID == NovaTeam::None || LocalPlayerTeamID == -1) TargetColor = FLinearColor::Yellow; // 중립, 아군 Yellow

	// 위젯 인스턴스에 접근하여 색상 전달
	// 1. 본체 위젯 색상 변경
	if (SelectionWidget && SelectionWidget->GetUserWidgetObject())
	{
		SelectionWidget->GetUserWidgetObject()->SetColorAndOpacity(TargetColor);
	}

	// 2. 바닥 투영 원 색상 변경 (추가)
	if (GroundSelectionWidget && GroundSelectionWidget->GetUserWidgetObject())
	{
		GroundSelectionWidget->GetUserWidgetObject()->SetColorAndOpacity(TargetColor);
	}

	// 3. 수직 안내선 색상 변경 (추가)
	if (HeightIndicatorLine)
	{
		// 런타임에 색상을 바꾸기 위해 Dynamic Material Instance를 사용합니다.
		UMaterialInstanceDynamic* DynMat = HeightIndicatorLine->CreateDynamicMaterialInstance(0);
		if (DynMat)
		{
			// 재질에서 만든 파라미터 이름(예: TeamColor)에 맞춰 색상을 전달합니다.
			DynMat->SetVectorParameterValue(TEXT("TeamColor"), TargetColor);
		}
	}
}

void ANovaUnit::UpdateSelectionCircleTransform()
{
	if (GetCapsuleComponent() && SelectionWidget)
	{
		// 스케일이 적용된 최종 반지름 값을 가져옴
		float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();

		// 지름에 여유있게 1.2배
		float Diameter = Radius * 2.0f * 1.2f;

		// 위젯의 해상도(DrawSize)를 지름 크기로 설정
		// SetUsingAbsoluteScale(true) 덕분에 이 수치가 곧 월드 크기가 됩니다.
		SelectionWidget->SetDrawSize(FVector2D(Diameter, Diameter));

		// 위치(높이) 업데이트
		float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		// 공중 유닛은 위젯을 더 아래로 내려줌
		if (MovementType == ENovaMovementType::Air)
		{
			HalfHeight += AirWidgetHeightOffset;
		}
		// 캡슐의 바닥에서 2.f정도 위로 올려줌 (위젯이 지면에 묻히는 걸 방지)
		SelectionWidget->SetRelativeLocation(FVector(0.f, 0.f, -HalfHeight + 2.f));

		// 상속을 껏으므로 위젯 자체 스케일은 1, 1, 1로 고정
		SelectionWidget->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));
	}
}

void ANovaUnit::UpdateGroundCircleAndLine()
{
	if (!GroundSelectionWidget || !HeightIndicatorLine) return;

	// 1. 시작점 설정: 캡슐의 정중앙에서 '절반 높이'만큼 내려서 바닥(Bottom) 지점을 찾습니다.
	float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector CapsuleBottom = GetActorLocation() - FVector(0.f, 0.f, HalfHeight);

	// 유닛 위치에서 아래로 충분히 긴 레이를 쏩니다.
	FVector Start = GetActorLocation() + FVector(0.f, 0.f, 10.f);;
	FVector End = Start - FVector(0.f, 0.f, 5000.f);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// ECollisionChannel GroundChannel = ECC_GameTraceChannel1;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic/*GroundChannel*/, Params))
	{
		FVector GroundLoc = Hit.Location;
		float Height = CapsuleBottom.Z - GroundLoc.Z;

		// 바닥 원 크기 설정(고정 크기)
		GroundSelectionWidget->SetVisibility(bIsSelected); // 선택 시 가시성 유지
		GroundSelectionWidget->SetWorldLocation(GroundLoc + FVector(0.f, 0.f, 10.f));
		GroundSelectionWidget->SetDrawSize(FVector2D(GroundSelectionCircleSize, GroundSelectionCircleSize));

		// 수직 안내선 업데이트 (공중 유닛일 때만 의미 있음)
		if (MovementType == ENovaMovementType::Air && Height > 20.f)
		{
			HeightIndicatorLine->SetVisibility(bIsSelected); // 선택됐을 때만 보임

			// 선의 위치는 유닛과 바닥의 정중앙에 배치
			FVector MidPoint = (CapsuleBottom + GroundLoc) * 0.5f;
			HeightIndicatorLine->SetWorldLocation(MidPoint);

			// Z 스케일을 높이에 맞게 조절 (기본 큐브 100단위 기준)
			// X, Y는 아주 얇게 (0.01f ~ 0.05f) 조정하세요.
			HeightIndicatorLine->SetWorldScale3D(FVector(0.02f, 0.02f, Height / 100.f));
		}
		else
		{
			HeightIndicatorLine->SetVisibility(false);
		}
	}
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
	if (!bVisible && SelectionWidget)
	{
		SelectionWidget->SetVisibility(false);
	}
}
