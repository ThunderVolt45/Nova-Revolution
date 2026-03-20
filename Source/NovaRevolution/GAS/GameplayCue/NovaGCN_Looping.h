#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Looping.h"
#include "Core/NovaInterfaces.h"
#include "NovaGCN_Looping.generated.h"

/**
 * 안개 가시성(Fog of War)을 체크하여 보이거나 숨겨지는 Looping GCN 기반 클래스
 */
UCLASS()
class NOVAREVOLUTION_API ANovaGCN_Looping : public AGameplayCueNotify_Looping, public INovaVisibilityInterface
{
	GENERATED_BODY()

public:
	ANovaGCN_Looping();

	// --- INovaVisibilityInterface ---
	virtual bool GetFogVisibility_Implementation() const override { return bIsVisibleByFog; }
	virtual void SetFogVisibility_Implementation(bool bVisible) override;

	// --- AGameplayCueNotify_Looping ---
	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Visibility")
	bool bIsVisibleByFog = true;
};
