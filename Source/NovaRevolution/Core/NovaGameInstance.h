// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "NovaGameInstance.generated.h"

/**
 * Nova Revolution 프로젝트의 메인 게임 인스턴스
 * 로딩 화면 시스템의 시작점 역할을 합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	/** 맵 로딩 시작 시 호출되는 핸들러 */
	virtual void BeginLoadingScreen(const FString& MapName);

	/** 맵 로딩 종료 시 호출되는 핸들러 */
	virtual void EndLoadingScreen(UWorld* InLoadedWorld);

protected:
	/** 로딩 화면 배경으로 사용할 이미지 (하나의 이미지만 공통 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading")
	TSoftObjectPtr<UTexture2D> LoadingImage;

private:
	/** 가비지 컬렉션 방지를 위한 현재 로드된 텍스트 인스턴스 보관 */
	UPROPERTY()
	UTexture2D* LoadingTextureInstance;
};
