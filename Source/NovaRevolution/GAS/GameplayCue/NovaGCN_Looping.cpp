#include "GAS/GameplayCue/NovaGCN_Looping.h"
#include "Core/NovaInterfaces.h"
#include "Core/NovaMapManager.h"
#include "Kismet/GameplayStatics.h"

ANovaGCN_Looping::ANovaGCN_Looping()
{
	PrimaryActorTick.bCanEverTick = false;
}

bool ANovaGCN_Looping::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (MyTarget)
	{
		// 타겟이 가시성 인터페이스를 구현하고 있다면 가시성 확인
		if (MyTarget->GetClass()->ImplementsInterface(UNovaVisibilityInterface::StaticClass()))
		{
			bIsVisibleByFog = INovaVisibilityInterface::Execute_GetFogVisibility(MyTarget);
			SetActorHiddenInGame(!bIsVisibleByFog);
			
			UE_LOG(LogTemp, Log, TEXT("GCN_Looping: OnActive for Target [%s], Initial Visibility: %s"), 
				*MyTarget->GetName(), bIsVisibleByFog ? TEXT("Visible") : TEXT("Hidden"));
		}

		// 안개 시스템에 등록하여 동적 가시성 업데이트를 받음
		if (ANovaMapManager* MapManager = Cast<ANovaMapManager>(UGameplayStatics::GetActorOfClass(this, ANovaMapManager::StaticClass())))
		{
			MapManager->RegisterActor(this);
		}
	}
	
	return Super::OnActive_Implementation(MyTarget, Parameters);
}

bool ANovaGCN_Looping::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	// 안개 시스템 등록 해제
	if (ANovaMapManager* MapManager = Cast<ANovaMapManager>(UGameplayStatics::GetActorOfClass(this, ANovaMapManager::StaticClass())))
	{
		MapManager->UnregisterActor(this);
	}

	return Super::OnRemove_Implementation(MyTarget, Parameters);
}

void ANovaGCN_Looping::SetFogVisibility_Implementation(bool bVisible)
{
	if (bIsVisibleByFog == bVisible) return;
	bIsVisibleByFog = bVisible;

	// 시각적 처리
	SetActorHiddenInGame(!bVisible);
}
