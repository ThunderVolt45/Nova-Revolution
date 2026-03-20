#include "GAS/GameplayCue/NovaGCN_Looping.h"
#include "Core/NovaInterfaces.h"

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
		}
	}
	
	return Super::OnActive_Implementation(MyTarget, Parameters);
}

void ANovaGCN_Looping::SetFogVisibility_Implementation(bool bVisible)
{
	if (bIsVisibleByFog == bVisible) return;
	bIsVisibleByFog = bVisible;

	// 시각적 처리
	SetActorHiddenInGame(!bVisible);
}
