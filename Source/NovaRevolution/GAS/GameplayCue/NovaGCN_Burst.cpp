#include "GAS/GameplayCue/NovaGCN_Burst.h"
#include "Core/NovaInterfaces.h"

bool UNovaGCN_Burst::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (MyTarget)
	{
		// 타겟이 가시성 인터페이스를 구현하고 있다면 가시성 확인
		if (MyTarget->GetClass()->ImplementsInterface(UNovaVisibilityInterface::StaticClass()))
		{
			if (!INovaVisibilityInterface::Execute_GetFogVisibility(MyTarget))
			{
				// 안개에 의해 보이지 않는 상태라면 실행 취소
				return false;
			}
		}
	}
	
	// 가시성이 확보되었거나 타겟이 없어 범용적인 경우(위치 기반 등) 부모 로직 실행
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
