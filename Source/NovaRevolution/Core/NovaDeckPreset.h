// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/NovaAssemblyTypes.h"
#include "NovaDeckPreset.generated.h"

/**
 * 게임에서 사용할 수 있는 유닛 덱 프리셋 데이터 에셋
 * AI 또는 초보 플레이어의 기본 덱으로 활용됩니다.
 */
UCLASS(BlueprintType)
class NOVAREVOLUTION_API UNovaDeckPreset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 프리셋 이름 (예: 기본형, 공격형 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Deck")
	FString PresetName;

	// 프리셋에 포함된 유닛 조합 데이터
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Deck")
	FNovaDeckInfo DeckInfo;
};
