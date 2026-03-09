// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaPart.h"
#include "Core/NovaPartData.h"
#include "Core/Animation/NovaWeaponAnimInstance.h"
#include "Core/Animation/NovaAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
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
		return SkeletalMesh;
	}
	
	return StaticMesh;
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
		// ChildActor인 경우 GetOwner()가 즉시 유효하지 않을 수 있으므로 ParentComponent를 통해 Owner를 확인합니다.
		AActor* OwnerActor = GetOwner();
		if (!OwnerActor && GetParentComponent())
		{
			OwnerActor = GetParentComponent()->GetOwner();
		}

		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OwnerActor))
		{
			if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
			{
				FGameplayCueParameters Params;
				Params.Location = GetActorLocation();
				Params.Normal = GetActorForwardVector();
				Params.Instigator = OwnerActor;
				Params.EffectCauser = this;
				
				ASC->ExecuteGameplayCue(FireCueTag, Params);
			}
		}
		else
		{
			NOVA_LOG(Warning, "PlayFireEffects: Could not find AbilitySystemInterface on Owner of %s", *GetName());
		}
	}
}
