// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/NovaGA_Attack.h"
#include "AbilitySystemComponent.h"
#include "Core/NovaUnit.h"
#include "Core/NovaPart.h"
#include "NovaRevolution.h"
#include "GAS/NovaGameplayTags.h"

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

	// 1. 발사 및 타격 정보 초기화
	FGameplayTag ImpactTag;
	FVector ImpactLocation = Target->GetActorLocation();
	bool bSurfaceHit = false;

	const TArray<TObjectPtr<UChildActorComponent>>& WeaponComps = Unit->GetWeaponPartComponents();

	for (auto WeaponComp : WeaponComps)
	{
		if (ANovaPart* WeaponPart = Cast<ANovaPart>(WeaponComp->GetChildActor()))
		{
			// 1-1. 무기 부품 애니메이션 및 발사 효과 (다중 소켓 지원)
			WeaponPart->PlayFireEffects();

			if (!ImpactTag.IsValid())
			{
				ImpactTag = WeaponPart->GetImpactCueTag();
			}

			// 1-2. 히트스캔 표면 적중 지점 계산 (Trace)
			if (!bSurfaceHit)
			{
				// 소켓이 있다면 첫 번째 소켓 위치를 트레이스 시작점으로 사용
				FVector Start = WeaponPart->GetActorLocation();
				if (WeaponPart->GetMuzzleSocketNames().Num() > 0 && WeaponPart->GetMainMesh())
				{
					Start = WeaponPart->GetMainMesh()->GetSocketLocation(WeaponPart->GetMuzzleSocketNames()[0]);
				}

				FVector End = Target->GetActorLocation() + (Target->GetActorLocation() - Start).GetSafeNormal() * 100.f;

				FHitResult HitResult;
				FCollisionQueryParams TraceParams;
				TraceParams.AddIgnoredActor(Unit);

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

	// 2. 공격자(Source)와 대상(Target)의 ASC를 가져옵니다.
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = Target->FindComponentByClass<UAbilitySystemComponent>();

	if (SourceASC && TargetASC && DamageEffectClass)
	{
		// GameplayEffectSpec 생성 (레벨은 1로 고정)
		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		ContextHandle.AddSourceObject(Unit);

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			// 대상에게 이펙트 적용 (히트스캔 방식: 즉시 적용)
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

			NOVA_LOG(Log, "GA_Attack: Damage GE applied to %s via Hitscan", *Target->GetName());
		}
	}

	// 3. 적중 효과 (Impact GameplayCue) 실행
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

