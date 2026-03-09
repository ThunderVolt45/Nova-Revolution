// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "NovaProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;

/**
 * Nova Revolution의 발사체 기본 클래스
 */
UCLASS()
class NOVAREVOLUTION_API ANovaProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	ANovaProjectile();

	/** 발사체 초기화 정보를 설정합니다. */
	void InitializeProjectile(const FGameplayEffectSpecHandle& InSpecHandle, const FGameplayTag& InImpactTag, float InSplashRadius = 0.0f);

protected:
	virtual void BeginPlay() override;

	/** 착탄 시 호출되는 함수 */
	UFUNCTION()
	virtual void OnProjectileHit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 실제 폭발/데미지 처리 로직 */
	void Explode(AActor* TargetActor, const FVector& ImpactLocation);

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

	/** 어빌리티로부터 전달받은 데미지 스펙 */
	FGameplayEffectSpecHandle DamageSpecHandle;
};
