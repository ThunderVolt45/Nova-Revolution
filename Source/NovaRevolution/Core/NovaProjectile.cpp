// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "NovaRevolution.h"
#include "Kismet/KismetSystemLibrary.h"

ANovaProjectile::ANovaProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// 충돌체 설정: 장애물을 무시하기 위해 기본적으로 Overlap 모드 사용
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->InitSphereRadius(15.0f);
	SphereComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SphereComponent->SetGenerateOverlapEvents(true);
	RootComponent = SphereComponent;

	// 투사체 이동 컴포넌트
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2000.f;
	ProjectileMovement->MaxSpeed = 2000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f; // 기본적으로 직선 비행
}

void ANovaProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	// 적중 이벤트 등록
	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &ANovaProjectile::OnProjectileHit);

	// 일정 시간 후 자동 소멸 (안전장치)
	SetLifeSpan(5.0f);
}

void ANovaProjectile::InitializeProjectile(const FGameplayEffectSpecHandle& InSpecHandle, const FGameplayTag& InImpactTag, float InSplashRadius)
{
	DamageSpecHandle = InSpecHandle;
	ImpactCueTag = InImpactTag;
	SplashRadius = InSplashRadius;
}

void ANovaProjectile::OnProjectileHit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 발사자 자신은 무시
	if (OtherActor == GetInstigator()) return;

	// 타겟이 유효한 경우 폭발 처리
	if (OtherActor)
	{
		Explode(OtherActor, GetActorLocation());
	}
}

void ANovaProjectile::Explode(AActor* TargetActor, const FVector& ImpactLocation)
{
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
			else if (TargetActor)
			{
				// 단일 타겟 데미지 처리
				if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
				{
					SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
				}
			}
		}
	}

	Destroy();
}
