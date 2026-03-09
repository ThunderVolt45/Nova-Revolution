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
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/NovaLog.h"
#include "DSP/AudioDebuggingUtilities.h"
#include "GAS/Abilities/NovaGameplayAbility.h"

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
			
			// --- 실시간 속도 확인용 ---
			// NOVA_SCREEN(Log,"Unit: %s | Current Speed: %.2f", *GetName(), CurrentSpeed);
		}
	}
	// 3. [추가] 몸통(Body) 회전 로직을 매 프레임 실행합니다.
	UpdateBodyRotation(DeltaTime);
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

//
// AActor* ANovaUnit::GetTargetFromBlackboard() const
// {
// 	// 1. 유닛의 AI 컨트롤러를 가져옵니다.
// 	if (ANovaAIController* AICon = Cast<ANovaAIController>(GetController()))
// 	{
// 		// 2. 컨트롤러가 가진 블랙보드 컴포넌트를 가져옵니다.
// 		if (UBlackboardComponent* BB = AICon->GetBlackboardComponent())
// 		{
// 			// 3. "TargetActor" 키에 저장된 값을 AActor 타입으로 반환합니다.
// 			// (이미 ANovaAIController에 정의된 TargetActorKey 이름을 사용하는 것이 정확합니다.)
// 			return Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));
// 		}
// 	}
// 	return nullptr;
// }

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
							WeaponComp->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);

							WeaponPartComponents.Add(WeaponComp);
						}
					}
					else
					{
						NOVA_LOG(Warning, "Weapon Socket '%s' does not exist on Body Mesh of %s", *SocketName.ToString(), *GetName());
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
			BodyPartComponent->AttachToComponent(LegsMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, BodyTargetSocketName);
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
