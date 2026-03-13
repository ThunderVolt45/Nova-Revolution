// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Core/NovaInterfaces.h"
#include "NovaProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;

/**
 * Nova Revolution의 발사체 기본 클래스
 */
UCLASS()
class NOVAREVOLUTION_API ANovaProjectile : public AActor, public INovaObjectPoolable
{
	GENERATED_BODY()
	
public:	
	ANovaProjectile();

	/** 발사체 초기화 정보를 설정합니다. */
	void InitializeProjectile(const FGameplayEffectSpecHandle& InSpecHandle, const FGameplayTag& InImpactTag, float InSplashRadius = 0.0f, AActor* InTargetActor = nullptr, FVector InTargetLocation = FVector::ZeroVector, bool bInHoming = true);

	// --- INovaObjectPoolable ---
	virtual void OnSpawnFromPool_Implementation() override;
	virtual void OnReturnToPool_Implementation() override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** 실제 폭발/데미지 처리 로직 */
	void Explode(AActor* InTargetActor, const FVector& ImpactLocation);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Projectile")
	TObjectPtr<USphereComponent> SphereComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** 착탄 시 실행할 GameplayCue 태그 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Projectile")
	FGameplayTag ImpactCueTag;

	/** 광역 데미지 범위 (0이면 단일 타겟) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Projectile")
	float SplashRadius = 0.0f;
	
	/** 유도 여부 (false일 경우 최초 목표 지점으로만 비행) */
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Projectile")
	bool bHoming = true;

	// 목표와의 충돌 판정에 사용할 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nova|Projectile")
	float HitToTargetRange = 25.f;

	/** 유도 대상 액터 */
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Projectile")
	TObjectPtr<AActor> TargetActor;

	/** 목표 지점 (액터가 없을 때 사용하거나 최초 위치 저장용) */
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Projectile")
	FVector TargetLocation;

	/** 어빌리티로부터 전달받은 데미지 스펙 */
	FGameplayEffectSpecHandle DamageSpecHandle;
};
