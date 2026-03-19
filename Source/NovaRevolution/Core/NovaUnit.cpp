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
#include "Core/AI/NovaAIPlayerController.h"
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
#include "GameFramework/GameStateBase.h"
// #include "Commandlets/GatherTextFromAssetsCommandlet.h"

#include "NavigationSystem.h"
#include "NovaMapManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/GameplayStatics.h"
#include "NavAreas/NavArea_Default.h"
#include "UI/NovaHealthBarComponent.h"
#include "UI/NovaSelectionComponent.h"

#pragma region Constructor
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

	// 1. 캡처 컴포넌트 생성
	PortraitCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortraitCapture"));
	PortraitCapture->SetupAttachment(RootComponent);

	// 2. 중요: 자기 자신만 찍도록 설정 (배경 제거 효과)
	PortraitCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	// 소스 변경 (SceneColorSceneDepth -> FinalColorLDR)
	// LDR 모드는 알파 채널에 Opacity(불투명도)를 저장합니다.
	PortraitCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

	// 포스트 프로세스 끄기
	PortraitCapture->ShowFlags.SetPostProcessing(false); // 포스트 프로세스 끄기
	PortraitCapture->ShowFlags.SetAtmosphere(false); // 대기 효과 끄기
	PortraitCapture->ShowFlags.SetFog(false); // 안개 끄기

	// 3. 성능을 위해 기본적으로 꺼둠
	PortraitCapture->bCaptureEveryFrame = false;
	PortraitCapture->bCaptureOnMovement = false;

	// 4. 위치 조정 (기본적으로 유닛 앞쪽 위)
	PortraitCapture->SetRelativeLocation(PortraitCaptureLocation);
	PortraitCapture->SetRelativeRotation(PortraitCaptureRotation);
	PortraitCapture->FOVAngle = PortraitCaptureFOVAngle;

	// 라이팅 끄기 (Unlit 모드와 동일 효과)
	// PortraitCapture->ShowFlags.SetLighting(false);
	PortraitCapture->ShowFlags.SetLighting(true);

	// 선택 표시 위젯 컴포넌트
	SelectionComponent = CreateDefaultSubobject<UNovaSelectionComponent>(TEXT("SelectionComponent"));
	SelectionComponent->SetupAttachment(RootComponent);
	SelectionComponent->SetRelativeLocation(FVector::ZeroVector);

	// 체력바 컴포넌트 추가
	HealthBarComponent = CreateDefaultSubobject<UNovaHealthBarComponent>(TEXT("HealthBarComponent"));
	HealthBarComponent->SetupAttachment(RootComponent);
}
#pragma endregion

#pragma region Engine Overrides
void ANovaUnit::BeginPlay()
{
	Super::BeginPlay();

	// 하이라이트용 동적 머티리얼 인스턴스 미리 생성
	if (HighlightMasterMaterial)
	{
		HighlightDynamicMaterial = UMaterialInstanceDynamic::Create(HighlightMasterMaterial, this);
	}

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

		// 속도 변경 콜백 등록 : 저주프리즈 스킬 관련 로직
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetSpeedAttribute())
		                      .AddUObject(this, &ANovaUnit::OnSpeedChanged);
	}

	// 초기 Yaw 설정
	LastYaw = GetActorRotation().Yaw;

	// 렌더 타겟 연결
	if (PortraitRenderTarget && PortraitCapture)
	{
		PortraitCapture->TextureTarget = PortraitRenderTarget;
		PortraitCapture->SetRelativeLocationAndRotation(PortraitCaptureLocation, PortraitCaptureRotation); // 카메라 조정
		// ShowOnlyList에 자기 자신 추가
		PortraitCapture->ShowOnlyActors.Add(this);
	}

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

	// MapManager에 등록
	if (ANovaMapManager* MapManager = Cast<ANovaMapManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ANovaMapManager::StaticClass())))
	{
		MapManager->RegisterActor(this);
	}

	// --- 랠리 포인트 이동 로직 추가 ---
	// 초기 랠리 포인트가 설정되어 있다면 해당 위치로 이동 명령을 내립니다.
	if (!InitialRallyLocation.IsNearlyZero())
	{
		FCommandData MoveCmd;
		MoveCmd.CommandType = ECommandType::Move;
		MoveCmd.TargetLocation = InitialRallyLocation;

		IssueCommand(MoveCmd);
		// NOVA_LOG(Log, "Unit %s is moving to initial rally point: %s", *GetName(), *InitialRallyLocation.ToString());
	}
}

void ANovaUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 사망 시에는 머티리얼 애니메이션(1초)만 처리한다
	if (bIsDead)
	{
		if (DeathTimeElapsed < CharredDuration)
		{
			DeathTimeElapsed += DeltaTime;
			UpdateCharredEffect(FMath::Min(DeathTimeElapsed / CharredDuration, 1.0f));
		}
		return;
	}

	// 1. 다리(Legs) 부품 애니메이션 데이터 전달
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

	// 2. 몸통(Body) 및 무기 회전 로직 실행
	UpdateBodyRotation(DeltaTime);
	UpdateWeaponAiming(DeltaTime);

	// 3. 완벽히 겹쳤을 때 이동 불가가 되는 현상 방지 (Anti-Overlap) 및 아군 길막힘 방지 밀어내기
	HandleUnitOverlaps(DeltaTime);

	// 4. 체력바 위젯 위치 업데이트
	UpdateUIComponents();
}

void ANovaUnit::HandleUnitOverlaps(float DeltaTime)
{
	if (bIsDead || !GetCapsuleComponent()) return;

	float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// 아군 길막힘 방지를 위해 반경을 조금 더 넓게(1.1f) 잡습니다.
	bool bHasOverlap = GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn,
	                                                     FCollisionShape::MakeSphere(Radius * 1.1f), Params);
	if (!bHasOverlap) return;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		ANovaUnit* OtherUnit = Cast<ANovaUnit>(Overlap.GetActor());
		if (!OtherUnit || OtherUnit->IsDead()) continue;

		FVector MyLoc = GetActorLocation();
		FVector OtherLoc = OtherUnit->GetActorLocation();
		FVector Diff = MyLoc - OtherLoc;
		Diff.Z = 0.0f; // 수평 방향으로만 밀어냄

		float Dist = Diff.Size();

		// 거의 완전히 겹쳤을 경우 랜덤한 방향으로 튕겨냄 (기존 Anti-Overlap)
		if (Dist < 0.1f)
		{
			FVector RandomDir = FVector(FMath::RandRange(-1.f, 1.f), FMath::RandRange(-1.f, 1.f), 0.f).GetSafeNormal();
			FVector Offset = RandomDir * (120.0f * DeltaTime);

			// [개선] NavMesh 레이캐스트 및 고도 보정
			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			if (NavSys)
			{
				const ANavigationData* NavData = NavSys->GetNavDataForProps(GetNavAgentPropertiesRef());
				if (NavData)
				{
					FVector HitLocation;
					FVector TargetLoc = MyLoc + Offset;
					// [수정] NavData의 Raycast를 직접 호출
					if (NavData->Raycast(MyLoc, TargetLoc, HitLocation, NavData->GetDefaultQueryFilter()))
					{
						TargetLoc = HitLocation;
					}

					// 내비메시 표면에 Z 고도 고정 (지상/공중 통합 처리)
					FNavLocation ProjectedLoc;
					if (NavSys->ProjectPointToNavigation(TargetLoc, ProjectedLoc, FVector(10.f, 10.f, 200.f), NavData))
					{
						TargetLoc = ProjectedLoc.Location;
						TargetLoc.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
					}

					Offset = TargetLoc - MyLoc;
				}
			}

			// 보정된 오프셋만큼 밀어냄
			AddActorWorldOffset(Offset, false);
			continue;
		}

		// 충돌 반경의 80% 이내로 깊숙이 파고든 경우 밖으로 밀어냄 (기존 Anti-Overlap)
		if (Dist < Radius * 0.8f)
		{
			FVector PushDir = Diff / Dist;

			// 이동 중일 때 정면 충돌 데드락 방지 (우측 통행 유도)
			FVector Velocity = GetVelocity();
			Velocity.Z = 0.0f;

			if (Velocity.SizeSquared() > 100.0f)
			{
				FVector MyForward = Velocity.GetSafeNormal();
				FVector MyRight = FVector::CrossProduct(FVector::UpVector, MyForward);
				float DotRight = FVector::DotProduct(MyRight, -PushDir);

				FVector TangentDir = (DotRight > -0.1f) ? MyRight : -MyRight;
				PushDir = (PushDir * 0.6f + TangentDir * 0.4f).GetSafeNormal();
			}

			FVector Offset = PushDir * (90.0f * DeltaTime);

			// [개선] NavMesh 레이캐스트 및 고도 보정
			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			if (NavSys)
			{
				const ANavigationData* NavData = NavSys->GetNavDataForProps(GetNavAgentPropertiesRef());
				if (NavData)
				{
					FVector HitLocation;
					FVector TargetLoc = MyLoc + Offset;
					// [수정] NavData의 Raycast를 직접 호출
					if (NavData->Raycast(MyLoc, TargetLoc, HitLocation, NavData->GetDefaultQueryFilter()))
					{
						TargetLoc = HitLocation;
					}

					// 내비메시 표면에 Z 고도 고정 (지상/공중 통합 처리)
					FNavLocation ProjectedLoc;
					if (NavSys->ProjectPointToNavigation(TargetLoc, ProjectedLoc, FVector(10.f, 10.f, 200.f), NavData))
					{
						TargetLoc = ProjectedLoc.Location;
						TargetLoc.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
					}

					Offset = TargetLoc - MyLoc;
				}
			}

			// 보정된 오프셋만큼 밀어냄
			AddActorWorldOffset(Offset, false);
		}

		// --- 아군 길막힘 방지 밀어내기 로직 ---
		if (GetTeamID() != OtherUnit->GetTeamID()) continue;

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
			// 초당 120.0f 속도로 밀어냄
			OtherUnit->PushUnit(-Diff.GetSafeNormal(), 120.0f * DeltaTime, 0);
		}
	}
}

void ANovaUnit::UpdateUIComponents()
{
	if (!HealthBarComponent || !HealthBarComponent->IsVisible()) return;

	APlayerCameraManager* CamManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
	if (!CamManager) return;

	// 1. 카메라의 상단 벡터(Up Vector)를 가져옵니다.
	FVector CameraUp = CamManager->GetCameraRotation().Quaternion().GetUpVector();
	FVector ScreenDownDir = -CameraUp;

	// 2. 캡슐의 바닥 지점을 찾습니다.
	float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector CapsuleBottom = GetActorLocation() - FVector(0.f, 0.f, HalfHeight);

	// 3. 캡슐 반지름(Radius) + 추가 오프셋
	float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	float TotalOffset = Radius + 20.0f;

	// 4. 최종 3D 위치: 캡슐 바닥에서 "화면 아래 방향"으로 오프셋만큼 이동
	FVector FinalAnchorLoc = CapsuleBottom + (ScreenDownDir * TotalOffset);

	HealthBarComponent->SetWorldLocation(FinalAnchorLoc);
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
#pragma endregion

#pragma region Unit Assembly & Initialization
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

void ANovaUnit::SetTeamID(int32 InTeamID)
{
	TeamID = InTeamID;

	// 소속 팀의 AI 사령관이 있다면 현재 웨이브(방어/소집)에 자신을 편입시킵니다.
	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		if (ANovaAIPlayerController* AIC = Cast<ANovaAIPlayerController>(Iterator->Get()))
		{
			if (AIC->GetTeamID() == TeamID)
			{
				AIC->AddUnitToCurrentWave(this);
				break;
			}
		}
	}
}

void ANovaUnit::ConstructUnitParts()
{
	ConstructLegs();
	ConstructBody();
	ConstructWeapons();
}

ANovaPart* ANovaUnit::SpawnPart(TSubclassOf<ANovaPart> PartClass)
{
	if (!PartClass) return nullptr;

	UNovaObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UNovaObjectPoolSubsystem>();
	ANovaPart* NewPart;

	if (PoolSubsystem)
	{
		NewPart = Cast<ANovaPart>(PoolSubsystem->SpawnFromPool(PartClass, GetActorTransform()));
	}
	else
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		NewPart = GetWorld()->SpawnActor<ANovaPart>(PartClass, Params);
	}

	if (NewPart)
	{
		NewPart->SetOwner(this);
		NewPart->SetPartDataTable(PartDataTable);

		// 캡처 카메라가 이 부품을 볼 수 있도록 리스트에 추가
		if (PortraitCapture)
		{
			PortraitCapture->ShowOnlyActors.AddUnique(NewPart);
		}
	}

	return NewPart;
}

void ANovaUnit::ConstructLegs()
{
	UNovaObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UNovaObjectPoolSubsystem>();

	// 클래스가 없으면 기존 부품 제거 후 종료
	if (!LegsPartClass)
	{
		if (CurrentLegsPart)
		{
			if (PoolSubsystem) PoolSubsystem->ReturnToPool(CurrentLegsPart);
			else CurrentLegsPart->Destroy();
			CurrentLegsPart = nullptr;
		}
		return;
	}

	// 클래스가 바뀌었거나 아직 없는 경우 새로 생성
	if (!CurrentLegsPart || CurrentLegsPart->GetClass() != LegsPartClass)
	{
		if (CurrentLegsPart)
		{
			if (PoolSubsystem) PoolSubsystem->ReturnToPool(CurrentLegsPart);
			else CurrentLegsPart->Destroy();
		}

		CurrentLegsPart = SpawnPart(LegsPartClass);
		if (CurrentLegsPart)
		{
			CurrentLegsPart->AttachToComponent(GetRootComponent(),
			                                   FAttachmentTransformRules::SnapToTargetIncludingScale);
			// 다리 부품 정면 보정 (Z: -90)
			CurrentLegsPart->SetActorRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		}
	}

	// 오프셋은 항상 업데이트 (에디터 실시간 반영용)
	if (CurrentLegsPart)
	{
		CurrentLegsPart->SetActorRelativeLocation(LegsOffset);
		CurrentLegsPart->SetActorRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}
}

void ANovaUnit::ConstructBody()
{
	UNovaObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UNovaObjectPoolSubsystem>();

	if (!BodyPartClass)
	{
		if (CurrentBodyPart)
		{
			if (PoolSubsystem) PoolSubsystem->ReturnToPool(CurrentBodyPart);
			else CurrentBodyPart->Destroy();
			CurrentBodyPart = nullptr;
		}
		return;
	}

	if (!CurrentBodyPart || CurrentBodyPart->GetClass() != BodyPartClass)
	{
		if (CurrentBodyPart)
		{
			if (PoolSubsystem) PoolSubsystem->ReturnToPool(CurrentBodyPart);
			else CurrentBodyPart->Destroy();
		}

		CurrentBodyPart = SpawnPart(BodyPartClass);
		InitializePartAttachments();
	}
}

void ANovaUnit::ConstructWeapons()
{
	UNovaObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UNovaObjectPoolSubsystem>();

	// 1. 기존 무기 중 클래스가 다른 것들 제거
	for (int32 i = CurrentWeaponParts.Num() - 1; i >= 0; --i)
	{
		if (CurrentWeaponParts[i] && CurrentWeaponParts[i]->GetClass() == WeaponPartClass) continue;

		if (CurrentWeaponParts[i])
		{
			if (PoolSubsystem) PoolSubsystem->ReturnToPool(CurrentWeaponParts[i]);
			else CurrentWeaponParts[i]->Destroy();
		}
		CurrentWeaponParts.RemoveAt(i);
	}

	// 2. 무기 클래스나 몸통이 없으면 종료
	if (!WeaponPartClass || !CurrentBodyPart)
	{
		for (ANovaPart* Weapon : CurrentWeaponParts)
		{
			if (!Weapon) continue;
			if (PoolSubsystem) PoolSubsystem->ReturnToPool(Weapon);
			else Weapon->Destroy();
		}
		CurrentWeaponParts.Empty();
		return;
	}

	// 3. 소켓 수에 맞춰 무기 생성 및 부착
	UPrimitiveComponent* BodyMesh = CurrentBodyPart->GetMainMesh();
	if (!BodyMesh) return;

	for (int32 i = 0; i < WeaponSocketNames.Num(); ++i)
	{
		const FName& SocketName = WeaponSocketNames[i];
		if (!BodyMesh->DoesSocketExist(SocketName)) continue;

		if (!CurrentWeaponParts.IsValidIndex(i))
		{
			ANovaPart* NewWeapon = SpawnPart(WeaponPartClass);
			if (NewWeapon)
			{
				NewWeapon->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetIncludingScale,
				                             SocketName);
				CurrentWeaponParts.Add(NewWeapon);
			}
		}
		else
		{
			CurrentWeaponParts[i]->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetIncludingScale,
			                                         SocketName);
		}
	}

	// 4. 남는 무기들 반환
	while (CurrentWeaponParts.Num() > WeaponSocketNames.Num())
	{
		ANovaPart* ExtraWeapon = CurrentWeaponParts.Pop();
		if (!ExtraWeapon) continue;
		if (PoolSubsystem) PoolSubsystem->ReturnToPool(ExtraWeapon);
		else ExtraWeapon->Destroy();
	}
}

void ANovaUnit::InitializePartAttachments()
{
	// ConstructUnitParts에서 이미 부착 로직을 수행하므로 여기서는 보강만 하거나 비워둡니다.
	if (CurrentLegsPart && CurrentBodyPart)
	{
		if (UPrimitiveComponent* LegsMesh = CurrentLegsPart->GetMainMesh())
		{
			// 소켓 위치가 정확한지 확인하기 위해 트랜스폼 업데이트 강제
			LegsMesh->UpdateComponentToWorld();

			CurrentBodyPart->AttachToComponent(LegsMesh, FAttachmentTransformRules::SnapToTargetIncludingScale,
			                                   BodyTargetSocketName);

			// 부착 후 몸통의 상대 위치를 한번 더 리셋하여 정확히 소켓에 물리도록 함
			if (UPrimitiveComponent* BodyMesh = CurrentBodyPart->GetMainMesh())
			{
				BodyMesh->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
			}
		}
	}
}

void ANovaUnit::InitializeAttributesFromParts()
{
	if (!AttributeSet) return;

	float TotalWatt = 0.0f, TotalHealth = 0.0f, TotalAttack = 0.0f, TotalDefense = 0.0f, TotalSpeed = 0.0f;
	float TotalFireRate = 0.0f, TotalSight = 0.0f, TotalRange = 0.0f, TotalMinRange = 0.0f, TotalSplashRange = 0.0f;

	// 수집할 모든 부품 액터 리스트업
	TArray<ANovaPart*> PartActors;
	if (CurrentLegsPart) PartActors.Add(CurrentLegsPart);
	if (CurrentBodyPart) PartActors.Add(CurrentBodyPart);
	if (CurrentWeaponParts.Num() > 0) PartActors.Add(CurrentWeaponParts[0]);

	for (ANovaPart* Part : PartActors)
	{
		if (!Part) continue;

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

		if (Spec.PartType == ENovaPartType::Legs) ApplyLegsSpec(Spec);
		else if (Spec.PartType == ENovaPartType::Weapon) TargetType = Spec.TargetType;
	}

	// 주요 스탯들의 최소치 이하로 내려가지 않도록 보정
	if (TotalHealth < 1.f) TotalHealth = 1.0f;
	if (TotalAttack < 0.f) TotalAttack = 0.0f;
	if (TotalDefense < 0.f) TotalDefense = 0.0f;
	if (TotalSpeed < 0.f) TotalSpeed = 0.0f;
	if (TotalFireRate < 25.f) TotalFireRate = 25.0f;

	// AttributeSet 초기화
	AttributeSet->InitWatt(TotalWatt);
	AttributeSet->InitHealth(TotalHealth);
	AttributeSet->InitMaxHealth(TotalHealth);
	AttributeSet->InitAttack(TotalAttack);
	AttributeSet->InitDefense(TotalDefense);
	AttributeSet->InitSpeed(TotalSpeed);
	AttributeSet->InitFireRate(TotalFireRate);
	AttributeSet->InitSight(TotalSight);
	AttributeSet->InitRange(TotalRange);
	AttributeSet->InitMinRange(TotalMinRange);
	AttributeSet->InitSplashRange(TotalSplashRange);

	// ★ 버그 수정: 오브젝트 풀링 시 기존 시스템에 남아있는 Aggregator 값까지 완전히 갱신 처리
	// ASC가 유효하다면 SetNumericAttributeBase를 호출하여 정상적으로 값이 동기화되게 합니다.
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetNumericAttributeBase(UNovaAttributeSet::GetWattAttribute(), TotalWatt);
		AbilitySystemComponent->SetNumericAttributeBase(UNovaAttributeSet::GetMaxHealthAttribute(), TotalHealth);
		AbilitySystemComponent->SetNumericAttributeBase(UNovaAttributeSet::GetHealthAttribute(), TotalHealth);
		AbilitySystemComponent->SetNumericAttributeBase(UNovaAttributeSet::GetAttackAttribute(), TotalAttack);
		AbilitySystemComponent->SetNumericAttributeBase(UNovaAttributeSet::GetDefenseAttribute(), TotalDefense);
		AbilitySystemComponent->SetNumericAttributeBase(UNovaAttributeSet::GetSpeedAttribute(), TotalSpeed);
		AbilitySystemComponent->SetNumericAttributeBase(UNovaAttributeSet::GetFireRateAttribute(), TotalFireRate);
		AbilitySystemComponent->SetNumericAttributeBase(UNovaAttributeSet::GetSightAttribute(), TotalSight);
		AbilitySystemComponent->SetNumericAttributeBase(UNovaAttributeSet::GetRangeAttribute(), TotalRange);
		AbilitySystemComponent->SetNumericAttributeBase(UNovaAttributeSet::GetMinRangeAttribute(), TotalMinRange);
		AbilitySystemComponent->SetNumericAttributeBase(UNovaAttributeSet::GetSplashRangeAttribute(), TotalSplashRange);
	}

	// 이동 속도 및 AI 설정 반영
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	MoveComp->MaxWalkSpeed = TotalSpeed;
	MoveComp->MaxFlySpeed = TotalSpeed;
	MoveComp->bRequestedMoveUseAcceleration = false;

	if (MovementType == ENovaMovementType::Air)
	{
		MoveComp->SetMovementMode(MOVE_Flying);
		MoveComp->bCheatFlying = true;
		MoveComp->MaxAcceleration = TotalSpeed * 10.0f;
		MoveComp->BrakingDecelerationFlying = TotalSpeed * 10.0f;
		MoveComp->BrakingFrictionFactor = 2.0f;
		MoveComp->FallingLateralFriction = 8.0f;

		// Z축 이동을 완벽히 차단하기 위한 평면 제한 설정
		MoveComp->bConstrainToPlane = true;
		MoveComp->bSnapToPlaneAtStart = true;
		MoveComp->SetPlaneConstraintNormal(FVector(0.0f, 0.0f, 1.0f));
		MoveComp->SetPlaneConstraintOrigin(FVector(0.0f, 0.0f, DefaultAirZ));

		// 현재 위치에서 Z값만 공중 고도로 강제 조정 (텔레포트 판정으로 물리 엔진에 의한 밀림 방지)
		FVector CurrentLoc = GetActorLocation();
		SetActorLocation(FVector(CurrentLoc.X, CurrentLoc.Y, DefaultAirZ), false, nullptr,
		                 ETeleportType::TeleportPhysics);
	}
	else
	{
		MoveComp->SetMovementMode(MOVE_Walking);
		MoveComp->bConstrainToPlane = true;
		MoveComp->MaxAcceleration = TotalSpeed * 10.0f;
		MoveComp->BrakingDecelerationWalking = TotalSpeed * 10.0f;
	}
}

void ANovaUnit::ApplyLegsSpec(const FNovaPartSpecRow& Spec)
{
	MovementType = Spec.MovementType;
	if (Spec.CollisionRadius <= 0.0f) return;

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule) return;

	Capsule->SetCapsuleRadius(Spec.CollisionRadius);

	if (NavModifier)
	{
		float ModifierRadius = Spec.CollisionRadius * 0.8f;
		float ModifierHeight = Capsule->GetUnscaledCapsuleHalfHeight();
		NavModifier->FailsafeExtent = FVector(ModifierRadius, ModifierRadius, ModifierHeight);
	}

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	MoveComp->NavAgentProps.AgentRadius = Spec.CollisionRadius;
	MoveComp->NavAgentProps.bCanWalk = (MovementType != ENovaMovementType::Air);
	MoveComp->NavAgentProps.bCanFly = (MovementType == ENovaMovementType::Air);
	MoveComp->AvoidanceConsiderationRadius = Spec.CollisionRadius;
	MoveComp->bUseRVOAvoidance = false;
	MoveComp->UpdateNavAgent(*this);
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
#pragma endregion

#pragma region Ability System (GAS) & Combat
UAbilitySystemComponent* ANovaUnit::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
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
		else if (ANovaUnit* TargetUnit = Cast<ANovaUnit>(CommandData.TargetActor))
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

		if (!bCanAttackTarget)
		{
			// NOVA_LOG(Warning, "Unit %s cannot attack %s due to TargetType mismatch or invalid target.", *GetName(),
			//          *CommandData.TargetActor->GetName());
			return; // 공격 불가능한 대상이면 명령 무시
		}
	}

	// 1. 전용 AI 컨트롤러에게 명령 전달 (이동, 추적 등)
	if (INovaCommandInterface* CmdInterface = Cast<INovaCommandInterface>(GetController()))
	{
		CmdInterface->IssueCommand(CommandData);
	}

	// NOVA_LOG(Log, "Unit Received Command: Type %d", (int32)CommandData.CommandType);
}

void ANovaUnit::Die()
{
	if (bIsDead) return;

	// 유닛 사망 처리 로직
	bIsDead = true;
	DeathTimeElapsed = 0.0f;

	// NovaPlayerController에 선택 해제 요청
	if (ANovaPlayerController* NovaPC = Cast<ANovaPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		NovaPC->NotifyTargetUnselectable(this);
	}

	// 자원 반납
	ReturnResourcesOnDeath();

	// AI 동작 정지 요청
	if (ANovaAIController* AIC = Cast<ANovaAIController>(GetController()))
	{
		AIC->OnPawnDeath();
	}

	// 내비게이션 장애물 해제
	SetNavigationObstacle(false);

	// 충돌 비활성화
	if (GetCapsuleComponent()) GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();

	// 모든 부품 상태 사망 설정
	if (CurrentLegsPart) CurrentLegsPart->SetIsDead(true);
	if (CurrentBodyPart) CurrentBodyPart->SetIsDead(true);
	for (ANovaPart* WeaponPart : CurrentWeaponParts)
	{
		if (WeaponPart) WeaponPart->SetIsDead(true);
	}

	// 사망 초기 연출 시작 (MID 생성 및 파라미터 0 설정)
	UpdateCharredEffect(0.0f);

	// 체력바 숨기기
	if (HealthBarComponent)
	{
		HealthBarComponent->SetVisibility(false);
		HealthBarComponent->Deactivate();
	}

	// MapManager에 등록 해제
	if (ANovaMapManager* MapManager = Cast<ANovaMapManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ANovaMapManager::StaticClass())))
	{
		MapManager->UnregisterActor(this);
	}

	// 일정 시간 후 오브젝트 풀로 반환 (사망 애니메이션 대기)
	FTimerHandle ReturnToPoolTimer;
	GetWorld()->GetTimerManager().SetTimer(ReturnToPoolTimer, [this]()
	{
		// 제거 시점에 폭발 효과(GCN) 실행
		if (ExplosionCueTag.IsValid() && AbilitySystemComponent)
		{
			FGameplayCueParameters Params;
			Params.Location = GetActorLocation();
			Params.Instigator = this;
			Params.EffectCauser = this;

			AbilitySystemComponent->ExecuteGameplayCue(ExplosionCueTag, Params);
		}

		// --- 부품 분리 및 사출 ---
		FVector DeathLoc = GetActorLocation();
		float LaunchStrength = FMath::RandRange(500.f, 800.f); // 힘의 범위 상향 및 무작위성

		for (ANovaPart* Weapon : CurrentWeaponParts)
		{
			if (Weapon)
			{
				// 무기 사출 방향에 무작위성 추가
				FVector Dir = (Weapon->GetActorLocation() - DeathLoc).GetSafeNormal();
				Dir += FMath::VRand() * 0.5f; // 무작위 방향 벡터 추가
				Dir.Z = FMath::Abs(Dir.Z) + 0.5f; // 항상 위쪽을 향하도록 보정
				Weapon->ExplodeAndDetach(Dir.GetSafeNormal() * LaunchStrength);
			}
		}

		CurrentWeaponParts.Empty();

		if (CurrentBodyPart)
		{
			// 몸통 사출 방향에 무작위성 추가
			FVector Dir = (CurrentBodyPart->GetActorLocation() - DeathLoc).GetSafeNormal();
			Dir += FMath::VRand() * 0.3f; // 몸통은 무기보다 덜 퍼지게 설정
			Dir.Z = FMath::Abs(Dir.Z) + 0.5f;
			CurrentBodyPart->ExplodeAndDetach(Dir.GetSafeNormal() * LaunchStrength);
			CurrentBodyPart = nullptr; // 유닛 반납 시 중복 반납 방지
		}

		// 데미지 연출 제거
		ClearDamageEffects();

		// 오브젝트 풀로 반환
		if (UNovaObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UNovaObjectPoolSubsystem>())
		{
			PoolSubsystem->ReturnToPool(this);
		}
		else
		{
			Destroy();
		}
	}, TimeToDestroy, false);
}

void ANovaUnit::UpdateCharredEffect(float Alpha)
{
	if (CurrentLegsPart) CurrentLegsPart->SetCharredAlpha(Alpha);
	if (CurrentBodyPart) CurrentBodyPart->SetCharredAlpha(Alpha);
	for (ANovaPart* Weapon : CurrentWeaponParts)
	{
		if (Weapon) Weapon->SetCharredAlpha(Alpha);
	}
}

void ANovaUnit::ReturnResourcesOnDeath() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	float UnitWatt = ASC->GetNumericAttribute(UNovaAttributeSet::GetWattAttribute());
	UWorld* World = GetWorld();
	if (!World) return;

	if (AGameStateBase* GameState = World->GetGameState())
	{
		for (APlayerState* PlayerStateObj : GameState->PlayerArray)
		{
			ANovaPlayerState* PS = Cast<ANovaPlayerState>(PlayerStateObj);
			if (!PS) continue;

			// INovaTeamInterface를 통해 팀 ID를 확인합니다.
			INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(PS);
			if (!TeamInterface || TeamInterface->GetTeamID() != TeamID) continue;

			if (UNovaResourceComponent* ResourceComp = PS->FindComponentByClass<UNovaResourceComponent>())
			{
				ResourceComp->UpdatePopulation(-1.0f, -UnitWatt);
				// NOVA_LOG(Log, "Unit %s (Watt: %.f) died. Resources returned to team %d", *GetName(), UnitWatt, TeamID);
				return; // 해당 팀의 자원을 업데이트했으므로 중단
			}
		}
	}
}

void ANovaUnit::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	// 체력이 0 이하가 되면 Die() 호출
	if (Data.NewValue <= 0.0f) Die();

	// 실시간 체력바 업데이트 호출
	UpdateHealthBar();

	// 데미지 연출 업데이트
	UpdateDamageEffects(Data.NewValue, AttributeSet->GetMaxHealth());

	// UI 갱신 알림 브로드캐스트
	OnUnitAttributeChanged.Broadcast(this);
}

void ANovaUnit::OnSpeedChanged(const FOnAttributeChangeData& Data)
{
	// NewValue는 GameplayEffect(CurseFreeze)에 의해 0이 되거나, 효과 종료 후 원래 값이 들어옵니다.
	float NewSpeed = Data.NewValue;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// 1. 실제 이동 컴포넌트의 최대 속도 갱신
		// 지상 유닛과 공중 유닛 모두에 대응하도록 MaxWalkSpeed와 MaxFlySpeed를 모두 업데이트합니다.
		MoveComp->MaxWalkSpeed = NewSpeed;
		MoveComp->MaxFlySpeed = NewSpeed;

		// 2. 만약 속도가 0이 되었다면 (얼어붙었다면)
		if (NewSpeed <= 0.0f)
		{
			// 즉시 모든 이동 관성을 멈춥니다 (빙결 시 미끄러짐 방지 및 물리적 고정 효과)
			MoveComp->StopMovementImmediately();
		}

		NOVA_LOG(Log, "Unit %s Speed Changed: %.f", *GetName(), NewSpeed);
	}
}

void ANovaUnit::UpdateDamageEffects(float CurrentHealth, float MaxHealth)
{
	if (!AbilitySystemComponent || bIsDead) return;

	float HealthPercent = (MaxHealth > 0.0f) ? (CurrentHealth / MaxHealth) : 0.0f;

	// 0. 모든 부품 수집
	TArray<ANovaPart*> AllParts;
	if (CurrentLegsPart) AllParts.Add(CurrentLegsPart);
	if (CurrentBodyPart) AllParts.Add(CurrentBodyPart);
	for (ANovaPart* Weapon : CurrentWeaponParts)
	{
		if (Weapon) AllParts.Add(Weapon);
	}

	// 1. 연기(Smoke) 상태 업데이트
	bool bShouldHaveSmoke = (HealthPercent <= SmokeThreshold);
	if (bIsSmokeActive != bShouldHaveSmoke)
	{
		bIsSmokeActive = bShouldHaveSmoke;

		for (ANovaPart* Part : AllParts)
		{
			UPrimitiveComponent* PartMesh = Part ? Part->GetMainMesh() : nullptr;

			// 소켓이 있는 부품만 골라냅니다.
			if (PartMesh && PartMesh->DoesSocketExist(DamageSocketName))
			{
				FGameplayCueParameters Params;
				Params.Instigator = this;
				Params.EffectCauser = Part; // 각 파츠를 Causer로 설정하여 GCN이 부착 대상을 알 수 있게 함
				Params.Location = PartMesh->GetSocketLocation(DamageSocketName);
				Params.Normal = PartMesh->GetSocketRotation(DamageSocketName).Vector();
				Params.SourceObject = Part; // 파츠별 인스턴스 구분을 위해 SourceObject에 파츠 할당
				Params.TargetAttachComponent = PartMesh; // 실제 소켓이 있는 메시 컴포넌트를 직접 전달

				if (bIsSmokeActive) AbilitySystemComponent->AddGameplayCue(SmokeCueTag, Params);
				else AbilitySystemComponent->RemoveGameplayCue(SmokeCueTag);
			}
		}
	}

	// 2. 불길(Fire) 상태 업데이트
	bool bShouldHaveFire = (HealthPercent <= FireThreshold);
	if (bIsFireActive != bShouldHaveFire)
	{
		bIsFireActive = bShouldHaveFire;

		for (ANovaPart* Part : AllParts)
		{
			UPrimitiveComponent* PartMesh = Part ? Part->GetMainMesh() : nullptr;
			if (PartMesh && PartMesh->DoesSocketExist(DamageSocketName))
			{
				FGameplayCueParameters Params;
				Params.Instigator = this;
				Params.EffectCauser = Part;
				Params.Location = PartMesh->GetSocketLocation(DamageSocketName);
				Params.Normal = PartMesh->GetSocketRotation(DamageSocketName).Vector();
				Params.SourceObject = Part;
				Params.TargetAttachComponent = PartMesh; // 실제 소켓이 있는 메시 컴포넌트를 직접 전달

				if (bIsFireActive) AbilitySystemComponent->AddGameplayCue(FireCueTag, Params);
				else AbilitySystemComponent->RemoveGameplayCue(FireCueTag);
			}
		}
	}
}

void ANovaUnit::ClearDamageEffects()
{
	if (bIsSmokeActive)
	{
		AbilitySystemComponent->RemoveGameplayCue(SmokeCueTag);
		bIsSmokeActive = false;
	}

	if (bIsFireActive)
	{
		AbilitySystemComponent->RemoveGameplayCue(FireCueTag);
		bIsFireActive = false;
	}
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

bool ANovaUnit::IsTargetTooClose(const AActor* Target) const
{
	if (!IsValid(Target)) return false;

	float MinRange = 0.0f;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		MinRange = ASC->GetNumericAttribute(UNovaAttributeSet::GetMinRangeAttribute());
	}

	if (MinRange <= 0.0f) return false;

	FVector MyLoc = GetActorLocation();
	FVector TargetLoc = Target->GetActorLocation();
	float DistXY = FVector::DistXY(MyLoc, TargetLoc);

	float TargetRadius = 0.0f;
	if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Target->GetRootComponent()))
	{
		TargetRadius = Capsule->GetScaledCapsuleRadius();
	}

	// [수정] 최소 사거리 판정도 Range와 동일하게 TargetRadius를 더해주는 방식으로 변경 (일관성 유지)
	float AdjustedMinRange = MinRange + TargetRadius;
	if (DistXY < AdjustedMinRange)
	{
		return true; // 수평 거리가 최소 사거리보다 작으므로 너무 가까움
	}

	return false;
}

float ANovaUnit::GetRequiredRetreatDistance(const AActor* Target) const
{
	if (!IsValid(Target)) return 0.0f;

	float MinRange = 0.0f;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		MinRange = ASC->GetNumericAttribute(UNovaAttributeSet::GetMinRangeAttribute());
	}

	if (MinRange <= 0.0f) return 0.0f;

	FVector MyLoc = GetActorLocation();
	FVector TargetLoc = Target->GetActorLocation();
	float DistXY = FVector::DistXY(MyLoc, TargetLoc);

	float TargetRadius = 0.0f;
	if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Target->GetRootComponent()))
	{
		TargetRadius = Capsule->GetScaledCapsuleRadius();
	}

	float AdjustedMinRange = MinRange + TargetRadius;
	
	// 부족한 후퇴 거리 계산 (값이 양수이면 물러나야 함)
	return FMath::Max(0.0f, AdjustedMinRange - DistXY);
}
#pragma endregion

#pragma region Movement & AI Rotation Logic
void ANovaUnit::PushUnit(FVector PushDir, float PushAmount, int32 Depth)
{
	// 최대 연쇄 밀림 횟수 제한 (무한 루프 및 프레임 드랍 방지)
	if (Depth > 4 || bIsDead) return;

	FVector CurrentLoc = GetActorLocation();
	FVector DesiredOffset = PushDir * PushAmount;
	FVector TargetLoc = CurrentLoc + DesiredOffset;

	// [개선] NavMesh 레이캐스트를 통해 맵 밖으로 나가는지 확인
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		const ANavigationData* NavData = NavSys->GetNavDataForProps(GetNavAgentPropertiesRef());
		if (NavData)
		{
			FVector HitLocation;
			// [수정] NavData의 Raycast를 직접 호출 (true 반환 시 경계에 부딪힘)
			if (NavData->Raycast(CurrentLoc, TargetLoc, HitLocation, NavData->GetDefaultQueryFilter()))
			{
				TargetLoc = HitLocation;
			}
			
			// 내비메시 표면에 Z 고도 고정 (지상/공중 통합 처리)
			FNavLocation ProjectedLoc;
			if (NavSys->ProjectPointToNavigation(TargetLoc, ProjectedLoc, FVector(10.f, 10.f, 200.f), NavData))
			{
				TargetLoc = ProjectedLoc.Location;
				TargetLoc.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			}
			
			DesiredOffset = TargetLoc - CurrentLoc;
		}
	}

	FHitResult Hit;

	// 보정된 오프셋만큼 이동
	AddActorWorldOffset(DesiredOffset, true, &Hit);

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
#pragma endregion

#pragma region Selection & UI
void ANovaUnit::OnSelected()
{
	bIsSelected = true;

	// 선택 가시성 제어를 Component가 수행
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

	// 선택 해제되면 카메라 끄기 (성능 절약)
	if (PortraitCapture)
	{
		PortraitCapture->bCaptureEveryFrame = false;
	}

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

void ANovaUnit::SetHighlightStatus(ENovaHighlightPriority Priority, bool bActive,
                                   FLinearColor Color)
{
	switch (Priority)
	{
	case ENovaHighlightPriority::Hover:
		bIsHovered = bActive;
		break;
	case ENovaHighlightPriority::Drag:
		bIsDragHighlighted = bActive;
		break;
	case ENovaHighlightPriority::SkillRange:
		bIsSkillHighlighted = bActive;
		break;
	}
	UpdateHighlight();
}

void ANovaUnit::UpdateHighlight()
{
	// 1. 현재 플레이어 컨트롤러의 명령 상태 확인
	bool bIsSkillMode = false;
	if (auto* PC = Cast<ANovaPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		// PendingCommandType이 Skill인 경우를 체크합니다.
		bIsSkillMode = (PC->GetPendingCommandType() == ECommandType::Skill);
	}

	// 2. 우선순위에 따른 최종 하이라이트 결정
	// [우선순위 1] 스킬 범위 내 하이라이트 (스킬 모드여도 범위 안에 닿으면 표시되어야 함)
	if (bIsSkillHighlighted)
	{
		SetHighlight(true, SkillHighlightColor);
	}
	// [우선순위 2] 드래그 하이라이트
	else if (bIsDragHighlighted)
	{
		SetHighlight(true, SkillHighlightColor);
	}
	// [우선순위 3] 호버 하이라이트
	else if (bIsHovered && !bIsSkillMode)
	{
		SetHighlight(true, SkillHighlightColor);
	}
	else
	{
		// 모든 조건에 해당하지 않거나, 스킬 모드 중인데 범위 밖에서 호버 중인 경우
		SetHighlight(false);
	}
}

void ANovaUnit::NotifyActorBeginCursorOver()
{
	SetHighlightStatus(ENovaHighlightPriority::Hover, true);
}

void ANovaUnit::NotifyActorEndCursorOver()
{
	SetHighlightStatus(ENovaHighlightPriority::Hover, false);
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

void ANovaUnit::UpdateHealthBar()
{
	if (!HealthBarComponent || !AttributeSet) return;

	HealthBarComponent->UpdateHealthBar(AttributeSet->GetHealth(),
	                                    AttributeSet->GetMaxHealth(),
	                                    CachedUIColor);
}
#pragma endregion

#pragma region Navigation & Fog of War
void ANovaUnit::SetFogVisibility(bool bVisible)
{
	if (bIsVisibleByFog == bVisible) return;
	bIsVisibleByFog = bVisible;

	// 시각적 처리
	SetActorHiddenInGame(!bVisible);

	// 부품 시각적 처리
	if (CurrentLegsPart) CurrentLegsPart->SetActorHiddenInGame(!bVisible);
	if (CurrentBodyPart) CurrentBodyPart->SetActorHiddenInGame(!bVisible);
	for (ANovaPart* WeaponPart : CurrentWeaponParts)
	{
		if (WeaponPart) WeaponPart->SetActorHiddenInGame(!bVisible);
	}

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

		// NOVA_LOG(Log, "Unit %s NavModifier %hs", *GetName(), bIsObstacle ? "Activated" : "Deactivated");
	}
}
#pragma endregion

#pragma region Object Pooling
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
		if (GetCapsuleComponent() && SelectionComponent)
		{
			float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
			float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			bool bIsAir = (MovementType == ENovaMovementType::Air);

			SelectionComponent->SetupSelection(Radius, HalfHeight, bIsAir);
			SelectionComponent->SetTeamColor(CachedUIColor);
		}
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
		if (!Part) continue;

		Part->SetIsDead(false);

		if (Part->GetPartSpec().PartType == ENovaPartType::Weapon)
		{
			Part->SetTargetPitch(0.0f);
		}
	}

	// 모든 부품 머티리얼 초기화 (Charred = 0)
	UpdateCharredEffect(0.0f);

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

	UpdateHealthBar();

	// 7. 데미지 연출 초기화
	bIsSmokeActive = false;
	bIsFireActive = false;

	// MapManager에 등록
	if (ANovaMapManager* MapManager = Cast<ANovaMapManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ANovaMapManager::StaticClass())))
	{
		MapManager->RegisterActor(this);
	}

	// 8. 랠리 포인트 이동
	if (!InitialRallyLocation.IsNearlyZero())
	{
		FCommandData MoveCmd;
		MoveCmd.CommandType = ECommandType::Move;
		MoveCmd.TargetLocation = InitialRallyLocation;
		IssueCommand(MoveCmd);
	}

	// NOVA_LOG(Log, "Unit %s Re-initialized from Pool with new assembly.", *GetName());
}

void ANovaUnit::OnReturnToPool_Implementation()
{
	// [중요] 풀에 들어간 유닛은 논리적으로 사망(비활성화)한 것으로 간주하여 드래그/클릭 선택에 잡히지 않도록 강제합니다.
	bIsDead = true;

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
	if (HealthBarComponent)
	{
		HealthBarComponent->SetVisibility(false);
	}
	// MapManager에 등록 해제
	if (ANovaMapManager* MapManager = Cast<ANovaMapManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ANovaMapManager::StaticClass())))
	{
		MapManager->UnregisterActor(this);
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

	// 6. GAS 상태 초기화 (풀에 들어갈 때 모든 수치/상태 리셋)
	if (AbilitySystemComponent)
	{
		// 모든 활성 GameplayEffect 제거 (루프를 통해 확실하게 제거)
		const FActiveGameplayEffectsContainer& ActiveGEs = AbilitySystemComponent->GetActiveGameplayEffects();
		TArray<FActiveGameplayEffectHandle> AllHandles = ActiveGEs.GetAllActiveEffectHandles();
		
		for (const FActiveGameplayEffectHandle& Handle : AllHandles)
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
		}
		
		NOVA_LOG(Log, "GAS State Cleared for %s (Removed %d GEs)", *GetName(), AllHandles.Num());
		
		// 모든 어빌리티 및 큐 제거
		AbilitySystemComponent->ClearAllAbilities();
		AbilitySystemComponent->RemoveAllGameplayCues();

		// 태그 초기화 (Loose Tag들 모두 제거)
		FGameplayTagContainer AllTags;
		AbilitySystemComponent->GetOwnedGameplayTags(AllTags);
		if (!AllTags.IsEmpty())
		{
			AbilitySystemComponent->RemoveLooseGameplayTags(AllTags);
		}
	}

	LegsPartClass = nullptr;
	BodyPartClass = nullptr;
	WeaponPartClass = nullptr;

	// NOVA_LOG(Log, "Unit %s Returned to Pool.", *GetName());
}
#pragma endregion

#pragma region Material Overlay

void ANovaUnit::SetHighlight(bool bEnable, FLinearColor HighlightColor)
{
	bIsHighlightActive = bEnable;
	UMaterialInterface* MaterialToApply = nullptr;

	if (bEnable && HighlightDynamicMaterial)
	{
		// 1. 동적 인스턴스의 색상 파라미터 업데이트 (마스터 머티리얼의 파라미터 이름과 일치해야 함)
		HighlightDynamicMaterial->SetVectorParameterValue(TEXT("OverlayColor"), HighlightColor);
		MaterialToApply = HighlightDynamicMaterial;
	}

	// 2. 모든 파츠(다리, 몸체, 무기 등)에 일괄 적용
	// 각 파츠 클래스(ANovaPart) 내부에 정의된 SetHighlight를 호출하여 OverlayMaterial을 설정합니다.
	if (CurrentLegsPart) CurrentLegsPart->SetHighlight(MaterialToApply);
	if (CurrentBodyPart) CurrentBodyPart->SetHighlight(MaterialToApply);

	for (ANovaPart* WeaponPart : CurrentWeaponParts)
	{
		if (WeaponPart)
		{
			WeaponPart->SetHighlight(MaterialToApply);
		}
	}
}

#pragma endregion
