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
	virtual void BeginPlay() override;
	
	/** 실제 플레이 가능 영역을 정의하는 박스 (에디터에서 조절) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Map")
	TObjectPtr<UBoxComponent> MapBounds;

	// 액터를 등록 및 관리할 배열
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> RegisteredActors;

	/** 미니맵 배경을 캡처할 카메라 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nova|Minimap")
	TObjectPtr<USceneCaptureComponent2D> MinimapCapture;

	/** 캡처된 배경 이미지를 저장할 렌더 타겟 (에셋에서 미리 생성하거나 런타임에 생성) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Minimap")
	TObjectPtr<UTextureRenderTarget2D> MinimapBackgroundRT;

	/** 미니맵 캡처 시 카메라 높이 (지형보다 높아야 함) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Minimap")
	float CaptureHeight = 10000.0f;

public:
	/** 지도의 바운드 정보 반환 */
	FBox GetMapBounds() const;

	/** 월드 좌표를 0~1 사이의 UV 좌표로 변환 (미니맵, 안개 공용) */
	UFUNCTION(BlueprintPure, Category = "Nova|Map")
	FVector2D WorldToMapUV(const FVector& WorldLocation) const;

	/** UV 좌표를 월드 좌표로 역산 */
	UFUNCTION(BlueprintPure, Category = "Nova|Map")
	FVector UVToWorldLocation(const FVector2D& UV, float ZHeight = 0.0f) const;

	// --- 액터 등록 및 관리 ---
	/** 액터를 관리 대상에 추가 */
	void RegisterActor(AActor* Actor);

	/** 액터를 관리 대상에서 제외 */
	void UnregisterActor(AActor* Actor);

	/** 등록된 액터 리스트 반환 */
	const TArray<TWeakObjectPtr<AActor>>& GetRegisteredActors() const { return RegisteredActors; }

	/** 맵 정보를 바탕으로 1회 캡처를 수행 */
	void CaptureMapBackground();

	/** 생성된 미니맵 배경 렌더 타겟을 반환 */
	UTextureRenderTarget2D* GetMinimapBackgroundRT() const { return MinimapBackgroundRT; }
};
