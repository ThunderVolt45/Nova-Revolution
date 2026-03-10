// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "NovaRevolution.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Core/NovaUnit.h"

ANovaProjectile::ANovaProjectile()
{
	PrimaryActorTick.bCanEverTick = true; // 유도 및 거리 체크를 위해 Tick 활성화

	// 충돌체 설정: 물리적 충돌 없이 위치 정보만 활용하므로 NoCollision 설정
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->InitSphereRadius(15.0f);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SphereComponent->SetGenerateOverlapEvents(false);
	RootComponent = SphereComponent;

	// 투사체 이동 컴포넌트
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2000.f;
	ProjectileMovement->MaxSpeed = 2000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f; // 기본적으로 직선 비행

	// 유도(Homing) 설정 초기화
	ProjectileMovement->bIsHomingProjectile = false;
	ProjectileMovement->HomingAccelerationMagnitude = 100000.f; // 유도 성능 (회전력)
}

void ANovaProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	// 목표에 도달할 때까지 사라지지 않도록 무한 수명 설정
	SetLifeSpan(0.0f);
}

void ANovaProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. 유도 모드인 경우에만 타겟 액터의 실시간 위치를 갱신합니다.
	// 유도 모드가 아니면 생성 시점의 TargetLocation을 유지합니다.
	if (bHoming && IsValid(TargetActor))
	{
		TargetLocation = TargetActor->GetActorLocation();
	}

	// 2. 목표 지점에 도달했는지 확인 (RTS 특성상 빗나감 방지)
	if (!TargetLocation.IsZero())
	{
		float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetLocation);

		// 충돌 체크 없이 거리상으로 도달했다면 즉시 폭발
		if (DistanceToTarget <= HitToTargetRange)
		{
			Explode(TargetActor, TargetLocation);
		}
	}
	// 3. 만약 타겟 정보가 아예 없는 비정상적인 상황이라면 소멸 처리
	else
	{
		if (GetLifeSpan() <= 0.0f)
		{
			SetLifeSpan(1.0f);
		}
	}
}

void ANovaProjectile::InitializeProjectile(const FGameplayEffectSpecHandle& InSpecHandle, const FGameplayTag& InImpactTag, float InSplashRadius, AActor* InTargetActor, FVector InTargetLocation, bool bInHoming)
{
	DamageSpecHandle = InSpecHandle;
	ImpactCueTag = InImpactTag;
	SplashRadius = InSplashRadius;
	TargetActor = InTargetActor;
	TargetLocation = InTargetLocation;
	bHoming = bInHoming;

	// 유도 기능 설정
	if (ProjectileMovement && IsValid(TargetActor) && bHoming)
	{
		ProjectileMovement->bIsHomingProjectile = true;

		// 타겟의 루트 컴포넌트를 유도 대상으로 설정
		if (USceneComponent* TargetRoot = TargetActor->GetRootComponent())
		{
			ProjectileMovement->HomingTargetComponent = TargetRoot;
		}
	}

	// 목표 지점이 설정되어 있지 않고 타겟 액터가 있다면 초기 위치 저장
	if (TargetLocation.IsZero() && IsValid(TargetActor))
	{
		TargetLocation = TargetActor->GetActorLocation();
	}
}

void ANovaProjectile::Explode(AActor* InTargetActor, const FVector& ImpactLocation)
{
	// ... (기존 Explode 내용 동일)
	// 1. GameplayCue 재생 (폭발 효과)
	if (ImpactCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Location = ImpactLocation;
		Params.Instigator = GetInstigator();
		Params.EffectCauser = this;
		
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator()))
		{
			ASC->ExecuteGameplayCue(ImpactCueTag, Params);
		}
	}

	// 2. 데미지 적용 (GAS)
	if (DamageSpecHandle.IsValid())
	{
		UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator());
		if (SourceASC)
		{
			if (SplashRadius > 0.0f)
			{
				// 광역 데미지 처리
				TArray<AActor*> OverlappedActors;
				TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
				ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
				ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
				ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));

				if (UKismetSystemLibrary::SphereOverlapActors(GetWorld(), ImpactLocation, SplashRadius, ObjectTypes, AActor::StaticClass(), {GetInstigator()}, OverlappedActors))
				{
					for (AActor* Actor : OverlappedActors)
					{
						if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor))
						{
							SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
						}
					}
				}
			}
			else if (InTargetActor)
			{
				// 단일 타겟 데미지 처리
				if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InTargetActor))
				{
					SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
				}
			}
		}
	}

	Destroy();
}
