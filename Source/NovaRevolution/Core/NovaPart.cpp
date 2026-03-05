// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaPart.h"
#include "Core/NovaPartData.h"
#include "Core/Animation/NovaAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
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
