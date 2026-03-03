// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NovaPart.generated.h"

/**
 * Nova Revolution의 유닛 부품(Legs, Body, Weapon 등)의 베이스 클래스
 */
UCLASS()
class NOVAREVOLUTION_API ANovaPart : public AActor
{
	GENERATED_BODY()
	
public:	
	ANovaPart();

	// 외부에서 메시를 가져오기 위한 유틸리티 함수
	UPrimitiveComponent* GetMainMesh() const;

protected:
	// 부품이 스켈레탈 메시일 경우 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Part")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	// 부품이 스태틱 메시일 경우 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Part")
	TObjectPtr<UStaticMeshComponent> StaticMesh;
};
