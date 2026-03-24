// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaGameInstance.h"
#include "NovaRevolution.h"
#include "MoviePlayer.h"
#include "UI/SNovaLoadingScreenWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"

void UNovaGameInstance::Init()
{
	// 로딩 이미지 미리 로드 (로딩 화면 표시 시 지연 방지)
	if (LoadingImage.IsValid())
	{
		LoadingTextureInstance = Cast<UTexture2D>(LoadingImage.TryLoad());
		if (LoadingTextureInstance)
		{
			// GC 대상에서 제외하여 영구적으로 보관 (패키지 빌드 오류 방지)
			LoadingTextureInstance->AddToRoot();

			// 이미지가 깨져 보이거나 늦게 로드되는 것을 방지하기 위해 스트리밍 비활성화 및 UI 그룹 설정
			LoadingTextureInstance->NeverStream = true;
			LoadingTextureInstance->LODGroup = TEXTUREGROUP_UI;
			LoadingTextureInstance->UpdateResource();
		}
	}
	
	Super::Init();

	// 맵 로딩 관련 델리게이트 등록
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UNovaGameInstance::BeginLoadingScreen);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UNovaGameInstance::EndLoadingScreen);
}

void UNovaGameInstance::BeginLoadingScreen(const FString& MapName)
{
	if (IsRunningDedicatedServer()) return;

	// 엔트리 맵(최초 진입점)으로 들어갈 때는 로딩 화면을 띄우지 않음
	if (MapName.Contains(TEXT("Lvl_Entry")))
	{
		return;
	}

	const FSlateBrush* BackgroundBrush = nullptr;

	// 로드된 이미지가 있다면 브러쉬 생성 (4:3 비율로 강제 설정)
	if (LoadingTextureInstance)
	{
		const float OriginalHeight = (float)LoadingTextureInstance->GetSizeY();
		const float ForcedWidth = OriginalHeight * (4.0f / 3.0f);
		BackgroundBrush = new FSlateImageBrush(LoadingTextureInstance, FVector2D(ForcedWidth, OriginalHeight));
	}

	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
	LoadingScreen.WidgetLoadingScreen = SNew(SNovaLoadingScreenWidget).LoadingImageBrush(BackgroundBrush);

	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
}

void UNovaGameInstance::EndLoadingScreen(UWorld* InLoadedWorld)
{
	// 텍스트 인스턴스를 여기서 null로 만들지 않습니다. (다음 로딩을 위해 캐싱 유지)
}