#include "Core/AI/BTTask_AIProduceUnit.h"

#include "NovaRevolution.h"
#include "Core/AI/NovaAIPlayerController.h"
#include "Core/NovaBase.h"
#include "Core/NovaInterfaces.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_AIProduceUnit::UBTTask_AIProduceUnit()
{
	NodeName = TEXT("AI Produce Unit");
	
	// 인구수나 와트가 부족해 실질적으로 생산되지 않더라도 태스크 자체는 '명령 하달'로 성공 처리
	// 실제 생산 제약은 ProduceUnit 내부의 CanProduceUnit에서 판단함
	RecommendedUnitSlotKey.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_AIProduceUnit, RecommendedUnitSlotKey));
}

EBTNodeResult::Type UBTTask_AIProduceUnit::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ANovaAIPlayerController* AIC = Cast<ANovaAIPlayerController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;

	ANovaBase* MyBase = AIC->GetManagedBase();
	if (!MyBase) return EBTNodeResult::Failed;

	// 1. 블랙보드에서 추천 슬롯 번호 가져오기
	int32 SlotIndex = OwnerComp.GetBlackboardComponent()->GetValueAsInt(RecommendedUnitSlotKey.SelectedKeyName);
	
	if (SlotIndex == -1) // 생산할 유닛이 없는 상황 (자원 극심 부족 등)
	{
		NOVA_LOG(Log, "AI Task: No unit recommended for production (-1)");
		return EBTNodeResult::Succeeded;
	}

	// 2. 생산 명령 수행 (MyBase는 INovaProductionInterface를 상속받음)
	bool bSpawned = MyBase->ProduceUnit(SlotIndex);
	
	if (bSpawned)
	{
		NOVA_LOG(Log, "AI Task: Triggered production for slot [%d]", SlotIndex);

		// [과생산 방지] 생산 명령을 하달했으므로, 다음 서비스 틱에서 다시 추천하기 전까지 키를 초기화합니다.
		OwnerComp.GetBlackboardComponent()->SetValueAsInt(RecommendedUnitSlotKey.SelectedKeyName, -1);
	}

	return EBTNodeResult::Succeeded;
}
