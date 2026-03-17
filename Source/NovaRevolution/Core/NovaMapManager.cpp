// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/NovaMapManager.h"

#include "Components/BoxComponent.h"

// Sets default values
ANovaMapManager::ANovaMapManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// 1. 실제 맵 범위를 정의할 박스 컴포넌트 생성
	MapBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("MapBounds"));
	RootComponent = MapBounds;

	// 기본 설정 (에디터에서 육안으로 확인하기 편하게 설정)
	MapBounds->SetBoxExtent(FVector(10000.f, 10000.f, 500.f));
	MapBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

FBox ANovaMapManager::GetMapBounds() const
{
	if (!MapBounds) return FBox(ForceInit);
	return MapBounds->Bounds.GetBox();
}

FVector2D ANovaMapManager::WorldToMapUV(const FVector& WorldLocation) const
{
	FBox Bounds = GetMapBounds();
	FVector Size = Bounds.GetSize();

	// 0으로 나누기 방지
	if (Size.X <= 0.f || Size.Y <= 0.f) return FVector2D(0.5f, 0.5f);

	// (현재위치 - 최소위치) / 전체크기 -> 0~1 사이의 비율 계산
	float U = (WorldLocation.X - Bounds.Min.X) / Size.X;
	float V = (WorldLocation.Y - Bounds.Min.Y) / Size.Y;
	
	// 맵 밖으로 나간 유닛도 미니맵 테두리에 걸치게 하려면 Clamp 처리
	return FVector2D(FMath::Clamp(U, 0.f, 1.f), FMath::Clamp(V, 0.f, 1.f));
}

FVector ANovaMapManager::UVToWorldLocation(const FVector2D& UV, float ZHeight) const
{
	FBox Bounds = GetMapBounds();
	FVector Size = Bounds.GetSize();

	// UV(0~1) 비율을 실제 월드 좌표로 역산 (Lerp와 동일한 원리)
	float WorldX = Bounds.Min.X + (UV.X * Size.X);
	float WorldY = Bounds.Min.Y + (UV.Y * Size.Y);

	return FVector(WorldX, WorldY, ZHeight);
}
