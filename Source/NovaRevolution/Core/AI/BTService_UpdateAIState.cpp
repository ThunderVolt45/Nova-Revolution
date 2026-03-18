// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/BTService_UpdateAIState.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/AI/NovaAIPlayerController.h"
#include "Core/NovaPlayerState.h"
#include "NovaRevolution.h"

UBTService_UpdateAIState::UBTService_UpdateAIState()
{
	NodeName = TEXT("Update AI State");
	
	// 서비스 주기 설정 (기본 0.5초 정도로 적절히 유지)
	Interval = 0.5f;
	RandomDeviation = 0.1f;

	// 키 필터 설정
	WattKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateAIState, WattKey));
	SPKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateAIState, SPKey));
	PopulationKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateAIState, PopulationKey));
	MaxPopulationKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateAIState, MaxPopulationKey));
	TotalUnitWattKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateAIState, TotalUnitWattKey));
	MaxUnitWattKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateAIState, MaxUnitWattKey));
}

void UBTService_UpdateAIState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	ANovaAIPlayerController* AIC = Cast<ANovaAIPlayerController>(OwnerComp.GetAIOwner());
	if (!AIC) return;

	ANovaPlayerState* PS = AIC->GetPlayerState<ANovaPlayerState>();
	if (!PS) return;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	// 1. 와트 상태 업데이트
	BB->SetValueAsFloat(WattKey.SelectedKeyName, PS->GetCurrentWatt());

	// 2. SP 상태 업데이트
	BB->SetValueAsFloat(SPKey.SelectedKeyName, PS->GetCurrentSP());

	// 3. 인구수(유닛 수) 상태 업데이트
	BB->SetValueAsFloat(PopulationKey.SelectedKeyName, PS->GetCurrentPopulation());
	BB->SetValueAsFloat(MaxPopulationKey.SelectedKeyName, PS->GetMaxPopulation());

	// 4. 유닛 와트 총합 상태 업데이트
	BB->SetValueAsFloat(TotalUnitWattKey.SelectedKeyName, PS->GetTotalUnitWatt());
	BB->SetValueAsFloat(MaxUnitWattKey.SelectedKeyName, PS->GetMaxUnitWatt());

	// 디버그용 로그 (필요 시 주석 해제)
	// NOVA_LOG(Log, "AI State Updated - Watt: %.1f, SP: %.1f, Pop: %.1f/%.1f, TotalUnitWatt: %.1f", 
	// 	PS->GetCurrentWatt(), PS->GetCurrentSP(), PS->GetCurrentPopulation(), PS->GetMaxPopulation(), PS->GetTotalUnitWatt());
}
