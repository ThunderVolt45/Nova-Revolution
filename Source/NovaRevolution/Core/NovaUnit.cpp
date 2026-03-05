// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaUnit.h"
#include "Core/NovaPart.h"
#include "Core/NovaPartData.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "NovaRevolution.h"
#include "GAS/NovaAttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Core/AI/NovaAIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

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
}

void ANovaUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead) return;

	if (LegsPartComponent)
	{
		if (ANovaPart* LegsActor = Cast<ANovaPart>(LegsPartComponent->GetChildActor()))
		{
			// 1. 이동 속도 전달
			float CurrentSpeed = GetVelocity().Size();
			LegsActor->SetMovementSpeed(CurrentSpeed);

			// 2. 회전 속도 계산 및 전달
			float CurrentYaw = GetActorRotation().Yaw;
			float YawDelta = FMath::FindDeltaAngleDegrees(LastYaw, CurrentYaw);
			float RotationRate = YawDelta / DeltaTime; // 초당 회전 각도
			
			LegsActor->SetRotationRate(RotationRate);
			LastYaw = CurrentYaw;
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
}

void ANovaUnit::BeginPlay()
{
	Super::BeginPlay();

	// 런타임 시작 시 다시 한번 초기화 및 부착 보강
	ConstructUnitParts();
	InitializePartAttachments();

	// 3. 모든 부품이 부착된 후 스탯 합산 및 초기화
	InitializeAttributesFromParts();

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

void ANovaUnit::ConstructUnitParts()
{
	// 다리와 몸통 부품 할당
	if (LegsPartClass) LegsPartComponent->SetChildActorClass(LegsPartClass);
	if (BodyPartClass) BodyPartComponent->SetChildActorClass(BodyPartClass);

	// 기존에 동적으로 생성된 무기 컴포넌트들 제거 (중복 생성 방지)
	for (UChildActorComponent* Comp : WeaponPartComponents)
	{
		if (Comp) Comp->DestroyComponent();
	}
	WeaponPartComponents.Empty();

	// 무기(Weapons) 할당 및 컴포넌트 생성
	for (int32 i = 0; i < WeaponSlotConfigs.Num(); ++i)
	{
		const FNovaWeaponPartSlot& Config = WeaponSlotConfigs[i];
		if (Config.WeaponPartClass)
		{
			// 무기 컴포넌트 동적 생성 및 등록
			FName CompName = FName(*FString::Printf(TEXT("WeaponPartComponent_%d"), i));

			UChildActorComponent* WeaponComp = NewObject<UChildActorComponent>(this, CompName);
			if (WeaponComp)
			{
				WeaponComp->CreationMethod = EComponentCreationMethod::UserConstructionScript;
				WeaponComp->RegisterComponent();
				WeaponComp->SetChildActorClass(Config.WeaponPartClass);
				WeaponComp->AttachToComponent(BodyPartComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);

				WeaponPartComponents.Add(WeaponComp);
			}
		}
	}
}

void ANovaUnit::InitializePartAttachments()
{
	// 1. 다리(Legs) 내에서 몸통(Body)이 붙을 소켓을 찾음
	if (ANovaPart* LegsActor = Cast<ANovaPart>(LegsPartComponent->GetChildActor()))
	{
		// 데이터 주입
		LegsActor->SetPartID(LegsPartID);
		LegsActor->SetPartDataTable(PartDataTable);

		if (UPrimitiveComponent* LegsMesh = LegsActor->GetMainMesh())
		{
			// 몸통을 지정된 소켓에 부착
			BodyPartComponent->AttachToComponent(LegsMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, BodyTargetSocketName);
		}
	}

	// 2. 몸통(Body) 내에서 무기(Weapons)들이 붙을 소켓들을 찾음
	if (ANovaPart* BodyActor = Cast<ANovaPart>(BodyPartComponent->GetChildActor()))
	{
		// 데이터 주입
		BodyActor->SetPartID(BodyPartID);
		BodyActor->SetPartDataTable(PartDataTable);

		if (UPrimitiveComponent* BodyMesh = BodyActor->GetMainMesh())
		{
			for (int32 i = 0; i < WeaponPartComponents.Num(); ++i)
			{
				if (i < WeaponSlotConfigs.Num())
				{
					// 무기 데이터 주입
					if (ANovaPart* WeaponActor = Cast<ANovaPart>(WeaponPartComponents[i]->GetChildActor()))
					{
						WeaponActor->SetPartID(WeaponSlotConfigs[i].PartID);
						WeaponActor->SetPartDataTable(PartDataTable);
					}

					// 각 무기를 슬롯 설정에 지정된 전용 소켓에 부착
					WeaponPartComponents[i]->AttachToComponent(
						BodyMesh, 
						FAttachmentTransformRules::SnapToTargetIncludingScale, 
						WeaponSlotConfigs[i].TargetSocketName);
				}
			}
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
	TArray<AActor*> PartActors;
	PartActors.Add(LegsPartComponent->GetChildActor());
	PartActors.Add(BodyPartComponent->GetChildActor());
	for (auto WeaponComp : WeaponPartComponents) PartActors.Add(WeaponComp->GetChildActor());

	// 각 부품에서 스탯 수집
	for (AActor* Actor : PartActors)
	{
		if (ANovaPart* Part = Cast<ANovaPart>(Actor))
		{
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
	}

	NOVA_SCREEN(Log, "Unit Stats Initialized: HP(%.f), Speed(%.f), Watt(%.f)", TotalHealth, TotalSpeed, TotalWatt);
}


UAbilitySystemComponent* ANovaUnit::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ANovaUnit::OnSelected()
{
	bIsSelected = true;
	// TODO: 팀원 B가 구현할 하이라이트/데칼 로직이 들어갈 자리
	UE_LOG(LogTemp, Log, TEXT("Unit Selected: %s"), *GetName());
}

void ANovaUnit::OnDeselected()
{
	bIsSelected = false;
	// TODO: 팀원 B가 구현할 하이라이트 해제 로직
	UE_LOG(LogTemp, Log, TEXT("Unit Deselected: %s"), *GetName());
}

bool ANovaUnit::IsSelectable() const
{
	// 플레이어 팀만 선택 가능하게 하려면 여기서 필터링 가능 (기본값 true)
	return true;
}

void ANovaUnit::IssueCommand(const FCommandData& CommandData)
{
	// 죽은 상태에서는 명령 수행 불가
	if (bIsDead) return;
	
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

bool ANovaUnit::MoveToLocation(const FVector& TargetLocation, float AcceptanceRadius)
{
	if (bIsDead) return false;

	ANovaAIController* AIC = Cast<ANovaAIController>(GetController());
	if (!AIC) return false;

	FVector FinalGoal = TargetLocation;

	// 1. 수동으로 목표 지점을 네비메쉬 위로 투영 (클릭 지점이 도달 불가능한 곳일 때를 대비)
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		FNavLocation ProjectedLocation;
		// 넉넉한 범위(1000) 내에서 가장 가까운 네비메쉬 지점 찾기
		if (NavSys->ProjectPointToNavigation(TargetLocation, ProjectedLocation, FVector(1000.f, 1000.f, 1000.f)))
		{
			FinalGoal = ProjectedLocation.Location;
		}
	}

	// 2. 이동 요청 설정
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(FinalGoal);
	MoveRequest.SetAcceptanceRadius(AcceptanceRadius);
	MoveRequest.SetAllowPartialPath(true);       // 경로가 끊겨도 최대한 가까이 이동
	MoveRequest.SetProjectGoalLocation(true);
	MoveRequest.SetRequireNavigableEndLocation(false); // 도달 불가능한 지점이라도 실패 처리하지 않음

	FPathFollowingRequestResult Result = AIC->MoveTo(MoveRequest);
	
	if (Result.Code == EPathFollowingRequestResult::Failed)
	{
		NOVA_LOG(Warning, "MoveToLocation Failed! Unit: %s, Goal: %s", *GetName(), *FinalGoal.ToString());
	}
	else
	{
		NOVA_LOG(Log, "MoveToLocation Started. Unit: %s, ResultCode: %d", *GetName(), (int32)Result.Code);
	}

	return Result.Code == EPathFollowingRequestResult::RequestSuccessful || Result.Code == EPathFollowingRequestResult::AlreadyAtGoal;
}

void ANovaUnit::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	// 체력이 0 이하가 되면 Die() 호출
	if (Data.NewValue <= 0.0f) Die();
}
