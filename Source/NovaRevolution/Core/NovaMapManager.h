// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NovaMapManager.generated.h"

class UBoxComponent;

UCLASS()
class NOVAREVOLUTION_API ANovaMapManager : public AActor
{
	GENERATED_BODY()

public:
	ANovaMapManager();

protected:
	/** 실제 플레이 가능 영역을 정의하는 박스 (에디터에서 조절) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Map")
	TObjectPtr<UBoxComponent> MapBounds;

public:
	/** 지도의 바운드 정보 반환 */
	FBox GetMapBounds() const;

	/** 월드 좌표를 0~1 사이의 UV 좌표로 변환 (미니맵, 안개 공용) */
	UFUNCTION(BlueprintPure, Category = "Nova|Map")
	FVector2D WorldToMapUV(const FVector& WorldLocation) const;

	/** UV 좌표를 월드 좌표로 역산 */
	UFUNCTION(BlueprintPure, Category = "Nova|Map")
	FVector UVToWorldLocation(const FVector2D& UV, float ZHeight = 0.0f) const;
};
