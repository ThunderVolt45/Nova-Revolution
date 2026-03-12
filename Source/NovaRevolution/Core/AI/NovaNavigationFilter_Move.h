#pragma once

#include "CoreMinimal.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "Core/NovaNavArea_Unit.h"
#include "NovaNavigationFilter_Move.generated.h"

/**
 * 유닛이 이동 경로를 계산할 때 사용하는 필터.
 * 다른 유닛이 차지하고 있는 고비용 영역을 일반 평지처럼 인식하게 하여 
 * 뭉쳐있는 상태에서도 즉시 경로를 생성하고 탈출할 수 있게 합니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaNavigationFilter_Move : public UNavigationQueryFilter
{
	GENERATED_BODY()

public:
	UNovaNavigationFilter_Move()
	{
		// 유닛 전용 영역(UNovaNavArea_Unit)의 비용을 1.0으로 재설정 (무시)
		FNavigationFilterArea UnitAreaEntry;
		UnitAreaEntry.AreaClass = UNovaNavArea_Unit::StaticClass();
		UnitAreaEntry.TravelCostOverride = 1.0f;
		UnitAreaEntry.EnteringCostOverride = 0.0f;
		
		Areas.Add(UnitAreaEntry);
	}
};
