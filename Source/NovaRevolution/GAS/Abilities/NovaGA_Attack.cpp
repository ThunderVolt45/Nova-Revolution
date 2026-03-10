// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/NovaGA_Attack.h"
#include "AbilitySystemComponent.h"
#include "Core/NovaUnit.h"
#include "Core/NovaPart.h"
#include "Core/NovaProjectile.h"
#include "GAS/NovaAttributeSet.h"
#include "NovaRevolution.h"
#include "GAS/NovaGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"

UNovaGA_Attack::UNovaGA_Attack()
{
	// 어빌리티 태그 설정 (SetAssetTags 권장 API 사용)
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(NovaGameplayTags::Ability_Attack);
	SetAssetTags(AssetTags);
}

void UNovaGA_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 어빌리티 발동 가능 여부 확인 (비용, 쿨다운 등)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. 공격 대상 확인 (베이스 클래스의 헬퍼 함수 활용)
	if (!TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Target = GetTargetActorFromEventData(*TriggerEventData);
	if (!Target)
	{
		NOVA_LOG(Warning, "GA_Attack: Target is null. Attack Canceled.");
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 실제 공격 실행
	ExecuteAttack(Target);

	// 4. 어빌리티 종료 (단발성 공격인 경우 즉시 종료)
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UNovaGA_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UNovaGA_Attack::ExecuteAttack(AActor* Target)
{
	if (!Target) return;

	ANovaUnit* Unit = Cast<ANovaUnit>(GetAvatarActorFromActorInfo());
	if (!Unit) return;

	// 1. 공격자(Source) ASC 및 속성 정보 가져오기
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	float SplashRadius = 0.0f;
	if (SourceASC)
	{
		SplashRadius = SourceASC->GetNumericAttribute(UNovaAttributeSet::GetSplashRangeAttribute());
	}

	// 2. 발사 및 타격 공통 정보 초기화
	FGameplayTag ImpactTag;
	FVector ImpactLocation = Target->GetActorLocation();
	bool bSurfaceHit = false;

	const TArray<TObjectPtr<UChildActorComponent>>& WeaponComps = Unit->GetWeaponPartComponents();

	// 3. 데미지 사양 생성 (발사체와 히트스캔 공용)
	FGameplayEffectSpecHandle DamageSpecHandle;
	if (SourceASC && DamageEffectClass)
	{
		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		ContextHandle.AddSourceObject(Unit);
		DamageSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
	}

	for (auto WeaponComp : WeaponComps)
	{
		if (ANovaPart* WeaponPart = Cast<ANovaPart>(WeaponComp->GetChildActor()))
		{
			// 3-1. 무기 부품 애니메이션 및 발사 효과 (다중 소켓 지원)
			WeaponPart->PlayFireEffects();

			if (!ImpactTag.IsValid())
			{
				ImpactTag = WeaponPart->GetImpactCueTag();
			}

			// 3-2. 소켓 위치 정보 (발사 지점)
			FVector MuzzleLocation = WeaponComp->GetComponentLocation();
			const TArray<FName>& MuzzleSocketNames = WeaponPart->GetMuzzleSocketNames();
			UPrimitiveComponent* MainMesh = WeaponPart->GetMainMesh();
			
			if (MuzzleSocketNames.Num() > 0 && MainMesh)
			{
				if (MainMesh->DoesSocketExist(MuzzleSocketNames[0]))
				{
					MuzzleLocation = MainMesh->GetSocketLocation(MuzzleSocketNames[0]);
					// NOVA_LOG(Log, "GA_Attack: Using Socket '%s' at %s", *MuzzleSocketNames[0].ToString(), *MuzzleLocation.ToString());
				}
			}

			// 4. 발사체 생성 (Projectile 방식)
			if (ProjectileClass)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = Unit;
				SpawnParams.Instigator = Unit;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				SpawnParams.bNoFail = true; // 무조건 생성 보장

				// 타겟을 향한 회전값 계산
				FRotator SpawnRotation = (Target->GetActorLocation() - MuzzleLocation).Rotation();

				NOVA_LOG(Log, "GA_Attack: Attempting to spawn Projectile '%s' at %s", *ProjectileClass->GetName(), *MuzzleLocation.ToString());
				
				if (ANovaProjectile* Projectile = GetWorld()->SpawnActor<ANovaProjectile>(ProjectileClass, MuzzleLocation, SpawnRotation, SpawnParams))
				{
					// [수정] 루트 컴포넌트(SphereComponent 등)를 가져와 이동 무시 설정
					if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Projectile->GetRootComponent()))
					{
						// 발사자(유닛) 무시
						RootPrim->IgnoreActorWhenMoving(Unit, true);
						
						// 유닛에 부착된 모든 부품(무기, 몸통 등) 무시
						TArray<AActor*> AttachedActors;
						Unit->GetAttachedActors(AttachedActors, true);
						for (AActor* Attached : AttachedActors)
						{
							RootPrim->IgnoreActorWhenMoving(Attached, true);
						}
					}

					Projectile->InitializeProjectile(DamageSpecHandle, ImpactTag, SplashRadius, Target, Target->GetActorLocation());
					NOVA_LOG(Log, "GA_Attack: Projectile Spawned Successfully!");
				}
				else
				{
					NOVA_LOG(Error, "GA_Attack: Projectile Spawn FAILED! (Class: %s)", *ProjectileClass->GetName());
				}
			}
			// 5. 히트스캔 처리 (Hitscan 방식 - ProjectileClass가 없을 때만)
			else if (!bSurfaceHit)
			{
				// NOVA_LOG(Log, "GA_Attack: ProjectileClass is NULL. Falling back to Hitscan.");
				// 소켓이 있다면 첫 번째 소켓 위치를 트레이스 시작점으로 사용
				FVector Start = MuzzleLocation;
				FVector End = Target->GetActorLocation() + (Target->GetActorLocation() - Start).GetSafeNormal() * 100.f;

				FHitResult HitResult;
				FCollisionQueryParams TraceParams;
				TraceParams.AddIgnoredActor(Unit);

				// [추가] 유닛에 부착된 모든 부품(Child Actor 등)도 트레이스에서 무시합니다.
				TArray<AActor*> AttachedActors;
				Unit->GetAttachedActors(AttachedActors, true);
				for (AActor* Attached : AttachedActors)
				{
					TraceParams.AddIgnoredActor(Attached);
				}

				if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, TraceParams))
				{
					if (HitResult.GetActor() == Target)
					{
						ImpactLocation = HitResult.ImpactPoint;
						bSurfaceHit = true;
					}
				}
			}
		}
	}

	// 6. 히트스캔 즉시 데미지 및 효과 처리 (발사체 방식이 아닐 때만)
	if (!ProjectileClass)
	{
		if (SourceASC && DamageSpecHandle.IsValid())
		{
			// 6-1. 광역 데미지 처리
			if (SplashRadius > 0.0f)
			{
				TArray<AActor*> OverlappedActors;
				TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
				ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
				ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
				ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));

				// NOVA_LOG(Log, "GA_Attack: Checking Splash Damage (Radius: %.f) at %s", SplashRadius, *ImpactLocation.ToString());

				if (UKismetSystemLibrary::SphereOverlapActors(GetWorld(), ImpactLocation, SplashRadius, ObjectTypes, AActor::StaticClass(), {Unit}, OverlappedActors))
				{
					int32 DamageCount = 0;
					for (AActor* Actor : OverlappedActors)
					{
						if (UAbilitySystemComponent* TargetASC = Actor->FindComponentByClass<UAbilitySystemComponent>())
						{
							SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
							DamageCount++;
						}
					}
					// NOVA_LOG(Log, "GA_Attack: Splash Damage applied to %d actors (Total Overlapped: %d)", DamageCount, OverlappedActors.Num());
				}
				else
				{
					// NOVA_LOG(Warning, "GA_Attack: No actors found in Splash Radius!");
				}
			}
			// 6-2. 단일 타겟 데미지 처리
			else if (UAbilitySystemComponent* TargetASC = Target->FindComponentByClass<UAbilitySystemComponent>())
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
				// NOVA_LOG(Log, "GA_Attack: Damage GE applied to %s via Hitscan", *Target->GetName());
			}
		}

		if (ImpactTag.IsValid())
		{
			FGameplayCueParameters Params;
			Params.Location = ImpactLocation;
			
			// 모델이 -90도 회전되어 있으므로, 계산된 법선을 -90도 회전시켜 방향 보정
			FVector CorrectedNormal = (Unit->GetActorLocation() - ImpactLocation).GetSafeNormal();
			CorrectedNormal = FRotator(0.f, -90.f, 0.f).RotateVector(CorrectedNormal);
			
			Params.Normal = CorrectedNormal;
			Params.Instigator = Unit;
			Params.EffectCauser = Unit;

			ExecuteGameplayCueWithParams(ImpactTag, Params);
		}
	}
}
