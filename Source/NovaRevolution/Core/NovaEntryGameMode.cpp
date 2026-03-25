#include "Core/NovaEntryGameMode.h"

#include "NovaRevolution.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"

void ANovaEntryGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (StartupTargetLevel.IsNull())
	{
		NOVA_SCREEN(Error, "NovaEntryGameMode: StartupTargetLevel is NOT set!");
		return;
	}

	// 동기 로드 및 레벨 전환 수행
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, StartupTargetLevel);
}
