// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NovaFogManager.generated.h"

class ANovaMapManager;
class UTextureRenderTarget2D;
class UBoxComponent;
class UCanvas;
class UMaterialParameterCollection;

UCLASS()
class NOVAREVOLUTION_API ANovaFogManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANovaFogManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// virtual void Tick(float DeltaTime) override;

	// 맵의 실제 크기를 정의하는 볼륨 -> MapManager 관할로 이전
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Nova|Fog")
	// TObjectPtr<UBoxComponent> FogVolume;
	
	/** 지도의 바운드 정보를 제공할 맵 매니저 */
	UPROPERTY(Transient)	// 저장할 필요 없는 런타임 참조
	TObjectPtr<ANovaMapManager> MapManager;
	
	// 실시간 시야 (Red 채널 활용) | RT : Render Taget
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nova|Fog")
	TObjectPtr<UTextureRenderTarget2D> CurrentFogRT;

	// 누적된 탐험 영역 (Green 채널 활용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nova|Fog")
	TObjectPtr<UTextureRenderTarget2D> HistoryFogRT;

	// 탐험 영역을 업데이트할 때 사용할 머터리얼
	UPROPERTY(EditAnywhere, Category="Nova|Fog")
	TObjectPtr<UMaterialInterface> HistoryUpdateMaterial;

	// 렌더 타겟 해상도
	UPROPERTY(EditAnywhere, Category="Nova|Fog")
	int32 TextureResolution = 1024;

	// 시야 원의 경계를 부드럽게 만들기 위한 머터리얼 (흰색 원형 그라데이션)
	UPROPERTY(EditAnywhere, Category="Nova|Fog")
	TObjectPtr<UMaterialInterface> BrushMaterial;

	// 안개 업데이트 주기 (0.1f : 초당 10번)
	UPROPERTY(EditAnywhere, Category="Nova|Fog")
	float UpdateInterval = 0.1f;

	// 에디터에서 MPC_Fog 에셋을 할당할 변수
	UPROPERTY(EditAnywhere, Category="Nova|Fog")
	TObjectPtr<UMaterialParameterCollection> FogMPC;

private:
	// Tick 대신 사용할 갱신용 타이머 핸들 (UpdateFog())
	FTimerHandle FogUpdateTimerHandle;

	// Fog 갱신
	void UpdateFog();

	// MPC 값을 계산하고 설정하는 내부 함수
	void UpdateMPCParameters();

	// 첫 번째 유효한 프레임에 초기화 수행하기 위한 bool 변수(Standalone 안전장치)
	bool bIsFogInitialized = false;

public:
	// --- Getter ---
	
	UTextureRenderTarget2D* GetCurrentFogRT() const {return CurrentFogRT;};
	
	// 월드 좌표를 렌더 타겟의 0~1 UV 좌표로 변환 (현실 좌표를 지도 좌표로 바꾸기)
	FVector2D WorldToFogUV(const FVector& WorldLocation) const;
};
