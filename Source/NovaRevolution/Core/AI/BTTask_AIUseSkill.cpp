#include "Core/AI/BTTask_AIUseSkill.h"
#include "Core/AI/NovaAIPlayerController.h"
#include "Core/NovaPlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "NovaRevolution.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_AIUseSkill::UBTTask_AIUseSkill()
{
	NodeName = TEXT("AI Use Skill");

	RecommendedSkillSlotKey.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_AIUseSkill, RecommendedSkillSlotKey));
	SkillTargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_AIUseSkill, SkillTargetActorKey), AActor::StaticClass());
	SkillTargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_AIUseSkill, SkillTargetLocationKey));
}

EBTNodeResult::Type UBTTask_AIUseSkill::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ANovaAIPlayerController* AIC = Cast<ANovaAIPlayerController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;

	ANovaPlayerState* PS = AIC->GetPlayerState<ANovaPlayerState>();
	if (!PS) return EBTNodeResult::Failed;

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	// 1. 블랙보드에서 추천 슬롯 번호 가져오기
	int32 SlotIndex = BB->GetValueAsInt(RecommendedSkillSlotKey.SelectedKeyName);
	if (SlotIndex == -1)
	{
		NOVA_LOG(Log, "AI Task: No skill recommended (-1)");
		return EBTNodeResult::Succeeded;
	}

	// 2. 사령관 스킬 태그 획득
	// PlayerState에는 각 슬롯별로 할당된 GameplayTag가 있음
	// 주의: 이 태스크를 사용하려면 PlayerState의 SkillSlotTags가 초기화되어 있어야 함
	const TArray<FGameplayTag>& SlotTags = PS->GetSkillSlotTags();
	if (!SlotTags.IsValidIndex(SlotIndex) || !SlotTags[SlotIndex].IsValid())
	{
		NOVA_LOG(Warning, "AI Task: Failed to find valid skill tag for slot [%d]", SlotIndex);
		return EBTNodeResult::Failed;
	}

	FGameplayTag SkillTag = SlotTags[SlotIndex];

	// 3. 타겟 데이터 구성
	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(SkillTargetActorKey.SelectedKeyName));
	FVector TargetLocation = BB->GetValueAsVector(SkillTargetLocationKey.SelectedKeyName);

	FGameplayEventData EventData;
	EventData.Instigator = PS;
	EventData.Target = TargetActor;
	
	// 타겟 데이터를 Handle에 담아 전달
	if (TargetActor)
	{
		EventData.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(TargetActor);
	}
	else if (!TargetLocation.IsZero())
	{
		// 위치 기반은 별도의 TargetData 구성을 원할 수도 있으나, 
		// 우선은 EventData의 Context나 EventMagnitude 등을 활용하거나 
		// 필요 시 커스텀 TargetData를 쓰도록 함 (여기서는 간단히 처리)
		FGameplayAbilityTargetData_LocationInfo* LocationData = new FGameplayAbilityTargetData_LocationInfo();
		LocationData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
		LocationData->TargetLocation.LiteralTransform = FTransform(TargetLocation);
		EventData.TargetData.Add(LocationData);
	}

	// 4. GameplayEvent를 통해 사령관 스킬 트리거 (타겟 정보 포함)
	int32 TriggerCount = ASC->HandleGameplayEvent(SkillTag, &EventData);

	if (TriggerCount > 0)
	{
		NOVA_LOG(Log, "AI Task: Triggered skill slot [%d] with tag [%s]", SlotIndex, *SkillTag.ToString());
		return EBTNodeResult::Succeeded;
	}

	// 발동 실패 시 (자원 부족 등)
	NOVA_LOG(Warning, "AI Task: Failed to trigger skill slot [%d] with tag [%s] (ASC HandleGameplayEvent returned 0)", SlotIndex, *SkillTag.ToString());

	// 디버깅: 현재 부여된 모든 어빌리티 태그 출력
	TArray<FGameplayAbilitySpec> Specs = ASC->GetActivatableAbilities();
	FString AbilityList;
	for (const FGameplayAbilitySpec& Spec : Specs)
	{
		AbilityList += FString::Printf(TEXT("[%s] "), *Spec.Ability->GetClass()->GetName());
	}
	NOVA_LOG(Log, "AI Task: Currently granted abilities on PS: %s", *AbilityList);

	return EBTNodeResult::Failed;
}
