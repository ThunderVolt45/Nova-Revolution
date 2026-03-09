// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NovaWeaponAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class NOVAREVOLUTION_API UNovaWeaponAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	/**
 * 무기가 타겟을 향해 조준해야 할 상하 각도 (Pitch)
 * ABP의 Transform (Modify) Bone 노드에서 이 값을 참조하여 포신/총구를 움직입니다.
 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Weapon")
	float AimPitch = 0.0f;

	/**
	 * (추가 가능) 무기가 현재 발사 중인지 여부
	 * 발사 애니메이션 루프나 이펙트 제어에 사용될 수 있습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Weapon")
	bool bIsFiring = false;
	
};
