// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NovaAudioSubsystem.generated.h"

class USoundBase;
class UAudioComponent;

/**
 * Nova Revolution 전역 오디오 관리 서브시스템
 */
UCLASS()
class NOVAREVOLUTION_API UNovaAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** 
	 * 배경 음악(BGM)을 재생합니다. 
	 * @param NewBGM 재생할 사운드 에셋
	 * @param bLoop 루핑 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Audio")
	void PlayBGM(USoundBase* NewBGM, bool bLoop = true);

	/** 
	 * 현재 재생 중인 BGM을 중지합니다.
	 * @param FadeOutDuration 페이드 아웃 지속 시간 (초)
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Audio")
	void StopBGM(float FadeOutDuration = 0.5f);

protected:
	/** 현재 재생 중인 BGM 컴포넌트 */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> BGMComponent;

	/** 현재 재생 중인 BGM 에셋 (동일 에셋 재생 요청 무시용) */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> CurrentBGMAsset;
};
