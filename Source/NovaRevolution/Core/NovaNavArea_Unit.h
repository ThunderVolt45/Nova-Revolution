#pragma once

#include "CoreMinimal.h"
#include "NavAreas/NavArea.h"
#include "NovaNavArea_Unit.generated.h"

/**
 * 유닛이 멈춰있을 때 차지하는 내비게이션 영역.
 * 지나갈 수는 있지만 비용을 높게 설정하여 다른 유닛들이 우회하도록 유도합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaNavArea_Unit : public UNavArea
{
	GENERATED_BODY()

public:
	UNovaNavArea_Unit()
	{
		// 기본 이동 비용을 높게 설정
		DefaultCost = 20.0f;
		
		// 디버그 시 보라색으로 표시되도록 설정 (P 키로 확인 가능)
		DrawColor = FColor::Purple;
	}
};
