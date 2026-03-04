// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaPart.h"
#include "Core/NovaPartData.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

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
		UAnimInstance* AnimInst = SkeletalMesh->GetAnimInstance();
		if (AnimInst)
		{
			static FName ParamName(TEXT("MovementSpeed"));
			if (FProperty* Prop = AnimInst->GetClass()->FindPropertyByName(ParamName))
			{
				if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
				{
					FloatProp->SetPropertyValue_InContainer(AnimInst, Speed);
				}
			}
		}
	}
}

void ANovaPart::SetRotationRate(float Rate)
{
	if (SkeletalMesh)
	{
		UAnimInstance* AnimInst = SkeletalMesh->GetAnimInstance();
		if (AnimInst)
		{
			static FName ParamName(TEXT("RotationRate"));
			if (FProperty* Prop = AnimInst->GetClass()->FindPropertyByName(ParamName))
			{
				if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
				{
					FloatProp->SetPropertyValue_InContainer(AnimInst, Rate);
				}
			}
		}
	}
}

void ANovaPart::SetIsDead(bool bDead)
{
	if (SkeletalMesh)
	{
		UAnimInstance* AnimInst = SkeletalMesh->GetAnimInstance();
		if (AnimInst)
		{
			static FName ParamName(TEXT("bIsDead"));
			if (FProperty* Prop = AnimInst->GetClass()->FindPropertyByName(ParamName))
			{
				if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
				{
					BoolProp->SetPropertyValue_InContainer(AnimInst, bDead);
				}
			}
		}
	}
}

void ANovaPart::PlayAttackAnimation()
{
	if (SkeletalMesh && PartData && PartData->AttackMontage)
	{
		UAnimInstance* AnimInst = SkeletalMesh->GetAnimInstance();
		if (AnimInst && !AnimInst->Montage_IsPlaying(PartData->AttackMontage))
		{
			AnimInst->Montage_Play(PartData->AttackMontage);
		}
	}
}
