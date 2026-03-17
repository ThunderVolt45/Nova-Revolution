// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaPart.h"
#include "Core/NovaPartData.h"
#include "Core/Animation/NovaWeaponAnimInstance.h"
#include "Core/Animation/NovaAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "NovaObjectPoolSubsystem.h"
#include "NovaRevolution.h"

ANovaPart::ANovaPart()
{
	PrimaryActorTick.bCanEverTick = false;

	// 루트 컴포넌트 생성
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// 두 종류의 메시 컴포넌트 미리 생성 (필요한 것만 블루프린트에서 할당)
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(RootComponent);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(RootComponent);
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANovaPart::OnSpawnFromPool_Implementation()
{
	// 부활 시 애니메이션 상태 초기화
	SetIsDead(false);
	CurrentPitch = 0.0f;
	TargetPitch = 0.0f;
}

void ANovaPart::OnReturnToPool_Implementation()
{
	// 1. 부모로부터 분리
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// 2. 모든 메시 컴포넌트의 물리 및 트랜스폼 리셋
	TArray<UPrimitiveComponent*> Meshes;
	GetComponents<UPrimitiveComponent>(Meshes);

	for (UPrimitiveComponent* MeshComp : Meshes)
	{
		if (MeshComp && MeshComp != RootComponent)
		{
			// 물리 시뮬레이션 완전 중지 및 속도 초기화
			MeshComp->SetSimulatePhysics(false);
			MeshComp->PutAllRigidBodiesToSleep();
			MeshComp->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
			MeshComp->SetAllPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

			// 충돌 비활성화
			MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			// 중요: 물리 시뮬레이션으로 인해 끊어졌을 수 있는 계층 구조를 복구합니다.
			MeshComp->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);

			// 튕겨나가면서 변한 상대 트랜스폼을 루트 기준으로 리셋
			MeshComp->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		}
	}

	// 3. 애니메이션 중지 및 상태 초기화
	if (SkeletalMesh)
	{
		if (UAnimInstance* AnimInst = SkeletalMesh->GetAnimInstance())
		{
			AnimInst->Montage_Stop(0.0f);
		}
	}
}

void ANovaPart::UpdateAiming(float DeltaTime)
{
	// 1. 목표 각도로 부드럽게 보간 (FInterpTo)
	CurrentPitch = FMath::FInterpTo(CurrentPitch, TargetPitch, DeltaTime, AimInterpSpeed);

	// 2. SkeletalMesh가 있고 애니메이션 인스턴스가 생성한 UNovaWeaponAnimInstance라면 값 전달
	if (SkeletalMesh)
	{
		if (UNovaWeaponAnimInstance* WeaponAnim = Cast<UNovaWeaponAnimInstance>(SkeletalMesh->GetAnimInstance()))
		{
			// C++ 변수에 보간된 값을 대입 -> ABP가 이 값을 보고 뼈를 움직임, pitch 입력값이 0일 때 하늘을 바라보기 때문에 값 보정 수행
			WeaponAnim->AimPitch = 90.f - CurrentPitch ;
		}
	}
}

void ANovaPart::BeginPlay()
{
	Super::BeginPlay();

	// 데이터 테이블로부터 스펙 초기화
	InitializePartSpec();
}

void ANovaPart::InitializePartSpec()
{
	if (PartDataTable && !PartID.IsNone())
	{
		FNovaPartSpecRow* FoundRow = PartDataTable->FindRow<FNovaPartSpecRow>(PartID, TEXT("ANovaPart::InitializePartSpec"));
		if (FoundRow)
		{
			PartSpec = *FoundRow;
			NOVA_LOG(Log, "Part '%s' initialized with Spec from DataTable (PartID: %s)", *GetName(), *PartID.ToString());
			return;
		}
		
		NOVA_CHECK(false, "Failed to find PartID '%s' in DataTable '%s' for Part '%s'", 
			*PartID.ToString(), *PartDataTable->GetName(), *GetName());
	}
}

UPrimitiveComponent* ANovaPart::GetMainMesh() const
{
	if (SkeletalMesh && SkeletalMesh->GetSkeletalMeshAsset())
	{
		// NOVA_LOG(Log, "Part '%s': MainMesh is SkeletalMesh", *GetName());
		return SkeletalMesh;
	}
	
	if (StaticMesh && StaticMesh->GetStaticMesh())
	{
		// NOVA_LOG(Log, "Part '%s': MainMesh is StaticMesh", *GetName());
		return StaticMesh;
	}

	return nullptr;
}

void ANovaPart::SetHighlight(UMaterialInterface* InHighlightMaterial)
{
	// 1. 액터에 포함된 모든 종류의 메시 컴포넌트(Skeletal, Static 모두 포함)를 찾습니다.
	// TArray에 담아 컴포넌트 구조가 복잡하더라도 일괄 처리가 가능하게 합니다.
	TArray<UMeshComponent*> MeshComponents;
	GetComponents<UMeshComponent>(MeshComponents);

	// 2. 찾은 모든 메시 컴포넌트를 순회하며 오버레이 머티리얼을 적용합니다.
	for (UMeshComponent* MeshComp : MeshComponents)
	{
		if (MeshComp)
		{
			// 전달받은 머티리얼이 있으면 하이라이트가 켜지고, nullptr이면 꺼집니다.
			MeshComp->SetOverlayMaterial(InHighlightMaterial);
		}
	}
}

void ANovaPart::SetMovementSpeed(float Speed)
{
	if (SkeletalMesh)
	{
		if (UNovaAnimInstance* AnimInst = Cast<UNovaAnimInstance>(SkeletalMesh->GetAnimInstance()))
		{
			AnimInst->MovementSpeed = Speed;
		}
	}
}

void ANovaPart::SetRotationRate(float Rate)
{
	if (SkeletalMesh)
	{
		if (UNovaAnimInstance* AnimInst = Cast<UNovaAnimInstance>(SkeletalMesh->GetAnimInstance()))
		{
			AnimInst->RotationRate = Rate;
		}
	}
}

void ANovaPart::SetIsDead(bool bDead)
{
	if (SkeletalMesh)
	{
		if (UNovaAnimInstance* AnimInst = Cast<UNovaAnimInstance>(SkeletalMesh->GetAnimInstance()))
		{
			AnimInst->bIsDead = bDead;
			if (!bDead)
			{
				// 유닛이 부활할 때 애니메이션 인스턴스의 상태를 강제로 초기화
				AnimInst->MovementSpeed = 0.0f;
				AnimInst->RotationRate = 0.0f;

				// 몽타주가 재생 중이었다면 정지시킵니다
				AnimInst->Montage_Stop(0.0f);

				// AnimGraph의 스테이트 머신이 Death 상태에서 빠져나오지 못하는 문제를 방지하기 위해 완전히 리셋
				SkeletalMesh->InitAnim(true);
			}
		}
	}
}

void ANovaPart::PlayAttackAnimation()
{
	// 클래스에 직접 할당된 AttackMontage를 사용합니다.
	if (SkeletalMesh && AttackMontage)
	{
		UAnimInstance* AnimInst = SkeletalMesh->GetAnimInstance();
		if (AnimInst && !AnimInst->Montage_IsPlaying(AttackMontage))
		{
			AnimInst->Montage_Play(AttackMontage);
		}
	}
}

void ANovaPart::PlayFireEffects()
{
	// 1. 애니메이션 재생
	PlayAttackAnimation();

	// 2. GameplayCue 실행 (발사 효과)
	if (FireCueTag.IsValid())
	{
		// 유닛 본체(IAbilitySystemInterface 구현체)를 찾습니다.
		AActor* AbilityOwner = GetOwner();
		
		// 만약 Owner가 인터페이스를 구현하지 않는다면, 부착된 부모 액터들을 거슬러 올라가며 찾습니다.
		if (AbilityOwner == nullptr || AbilityOwner->GetInterfaceAddress(UAbilitySystemInterface::StaticClass()) == nullptr)
		{
			AActor* CurrentSearch = this;
			while (CurrentSearch)
			{
				AActor* ParentActor = CurrentSearch->GetAttachParentActor();
				if (ParentActor && ParentActor->GetInterfaceAddress(UAbilitySystemInterface::StaticClass()))
				{
					AbilityOwner = ParentActor;
					break;
				}
				CurrentSearch = ParentActor;
			}
		}

		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(AbilityOwner))
		{
			if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
			{
				UPrimitiveComponent* Mesh = GetMainMesh();
				
				// 지정된 모든 총구 소켓에서 동시에 효과 발생
				if (MuzzleSocketNames.Num() > 0 && Mesh)
				{
					for (const FName& SocketName : MuzzleSocketNames)
					{
						FGameplayCueParameters Params;
						Params.Location = Mesh->GetSocketLocation(SocketName);
						Params.Normal = Mesh->GetSocketRotation(SocketName).Vector();
						Params.Instigator = AbilityOwner;
						Params.EffectCauser = this;
						
						ASC->ExecuteGameplayCue(FireCueTag, Params);
					}
				}
				else
				{
					FGameplayCueParameters Params;
					Params.Location = GetActorLocation();
					Params.Normal = GetActorForwardVector();
					Params.Instigator = AbilityOwner;
					Params.EffectCauser = this;
					
					ASC->ExecuteGameplayCue(FireCueTag, Params);
				}
			}
		}
		else
		{
			NOVA_LOG(Warning, "PlayFireEffects: Could not find AbilitySystemInterface on Owner or Parent of %s", *GetName());
		}
	}
}

void ANovaPart::SetCharredAlpha(float Alpha)
{
	TArray<UPrimitiveComponent*> Meshes;
	GetComponents<UPrimitiveComponent>(Meshes);

	for (UPrimitiveComponent* MeshComp : Meshes)
	{
		if (!MeshComp) continue;

		int32 NumMaterials = MeshComp->GetNumMaterials();
		for (int32 i = 0; i < NumMaterials; ++i)
		{
			UMaterialInterface* Mat = MeshComp->GetMaterial(i);
			UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mat);

			// Alpha가 0일 때(사망 초기) 다이나믹 머티리얼이 없다면 생성합니다.
			if (!MID && Alpha <= 0.01f)
			{
				MID = MeshComp->CreateDynamicMaterialInstance(i);
			}

			if (MID)
			{
				MID->SetScalarParameterValue(TEXT("Charred"), Alpha);
				
				// 서서히 검은색으로 변하는 연출 (FLinearColor::Black은 0,0,0)
				FLinearColor BlackColor = FLinearColor::Black;
				MID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::LerpUsingHSV(FLinearColor::White, BlackColor, Alpha));
				MID->SetVectorParameterValue(TEXT("Color"), FLinearColor::LerpUsingHSV(FLinearColor::White, BlackColor, Alpha));
			}
		}
	}
}

void ANovaPart::ExplodeAndDetach(FVector Impulse)
{
	// 1. 부모로부터 분리 (월드 변환 유지)
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// 2. 물리 설정 활성화
	if (UPrimitiveComponent* MeshComp = GetMainMesh())
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComp->SetCollisionProfileName(TEXT("PhysicsActor"));
		MeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		MeshComp->SetSimulatePhysics(true);
		MeshComp->SetCanEverAffectNavigation(false);
		
		// 튕겨나가는 힘 적용
		MeshComp->AddImpulse(Impulse, NAME_None, true);
	}

	// 3. 일정 시간 후 오브젝트 풀로 반납 (자가 반납)
	FTimerHandle ReturnTimer;
	GetWorld()->GetTimerManager().SetTimer(ReturnTimer, [this]()
	{
		if (UNovaObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UNovaObjectPoolSubsystem>())
		{
			PoolSubsystem->ReturnToPool(this);
		}
		else
		{
			Destroy();
		}
	}, 5.0f, false);
}
