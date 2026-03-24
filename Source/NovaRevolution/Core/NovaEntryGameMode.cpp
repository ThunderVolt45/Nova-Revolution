#include "Core/NovaEntryGameMode.h"
#include "Core/NovaGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"

void ANovaEntryGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (StartupTargetLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("NovaEntryGameMode: StartupTargetLevel is NOT set!"));
		return;
	}

	if (UNovaGameInstance* GameInstance = Cast<UNovaGameInstance>(GetGameInstance()))
	{
		// 로딩 화면 표시 시작
		GameInstance->BeginLoadingScreen(StartupTargetLevel.GetAssetName());

		// 비동기 로드 요청
		LoadPackageAsync(StartupTargetLevel.GetLongPackageName(),
			FLoadPackageAsyncDelegate::CreateUObject(this, &ANovaEntryGameMode::OnStartupLevelLoaded));
	}
}

void ANovaEntryGameMode::OnStartupLevelLoaded(const FName& PackageName, UPackage* LoadedPackage, EAsyncLoadingResult::Type Result)
{
	if (Result == EAsyncLoadingResult::Succeeded)
	{
		// 로딩이 완료되면 실제 레벨로 전환
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, StartupTargetLevel);
	}
}
