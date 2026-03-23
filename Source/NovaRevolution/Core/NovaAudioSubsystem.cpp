// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaAudioSubsystem.h"

#include "NovaRevolution.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

void UNovaAudioSubsystem::PlayBGM(USoundBase* NewBGM, bool bLoop)
{
	if (!NewBGM)
	{
		StopBGM();
		return;
	}

	// 이미 동일한 BGM이 재생 중이면 무시
	if (BGMComponent && BGMComponent->IsPlaying() && CurrentBGMAsset == NewBGM)
	{
		return;
	}

	// 기존 BGM 중지 (부드러운 전환을 위해 페이드 아웃 고려 가능하나, 일단 즉시 정지 후 교체)
	StopBGM(0.2f);

	// 새로운 BGM 재생
	BGMComponent = UGameplayStatics::SpawnSound2D(GetWorld(), NewBGM, 1.0f, 1.0f, 0.0f, nullptr, true, true);
	if (BGMComponent)
	{
		BGMComponent->bAutoDestroy = true;
		CurrentBGMAsset = NewBGM;
		
		NOVA_LOG(Log, "Playing BGM: %s", *NewBGM->GetName());
	}
}

void UNovaAudioSubsystem::StopBGM(float FadeOutDuration)
{
	if (BGMComponent && BGMComponent->IsPlaying())
	{
		if (FadeOutDuration > 0.0f)
		{
			BGMComponent->FadeOut(FadeOutDuration, 0.0f);
		}
		else
		{
			BGMComponent->Stop();
		}
		
		NOVA_LOG(Log, "BGM Stopped (FadeOut: %f)", FadeOutDuration);
	}

	CurrentBGMAsset = nullptr;
}
