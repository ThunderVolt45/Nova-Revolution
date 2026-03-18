#include "Core/AI/BTTask_AICommandUnits.h"
#include "Core/AI/NovaAIPlayerController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/NovaTypes.h"
#include "NovaRevolution.h"

UBTTask_AICommandUnits::UBTTask_AICommandUnits()
{
	NodeName = TEXT("AI Command All Units");
	
	// 블랙보드 필터 설정
	TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_AICommandUnits, TargetLocationKey));
}

EBTNodeResult::Type UBTTask_AICommandUnits::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ANovaAIPlayerController* AIC = Cast<ANovaAIPlayerController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	// 1. 명령 데이터 패키징 (무조건 위치 기반 이동을 위해 TargetActor는 nullptr로 고정)
	FCommandData CommandData{};
	CommandData.CommandType = CommandType;
	CommandData.TargetActor = nullptr;
	
	if (TargetLocationKey.IsSet())
	{
		CommandData.TargetLocation = BB->GetValueAsVector(TargetLocationKey.SelectedKeyName);
	}

	// 2. AI 플레이어를 통해 전 부대에 명령 하달
	AIC->IssueCommandToAllUnits(CommandData);

	return EBTNodeResult::Succeeded;
}
