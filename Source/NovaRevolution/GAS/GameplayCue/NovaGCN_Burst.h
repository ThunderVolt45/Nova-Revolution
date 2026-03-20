#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Burst.h"
#include "NovaGCN_Burst.generated.h"

/**
 * 안개 가시성(Fog of War)을 체크하여 실행 여부를 결정하는 Burst GCN 기반 클래스
 */
UCLASS()
class NOVAREVOLUTION_API UNovaGCN_Burst : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};
