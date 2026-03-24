#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NovaEntryGameMode.generated.h"

/**
 * 게임 시작 시 EntryMap에서 비동기 로딩을 트리거하기 위한 최소 기능 게임 모드
 */
UCLASS()
class NOVAREVOLUTION_API ANovaEntryGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

protected:
	/** 게임 시작 시 로드할 실제 첫 레벨 (에디터에서 지정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Loading")
	TSoftObjectPtr<UWorld> StartupTargetLevel;

	/** 비동기 로딩 완료 콜백 */
	void OnStartupLevelLoaded(const FName& PackageName, UPackage* LoadedPackage, EAsyncLoadingResult::Type Result);
};
