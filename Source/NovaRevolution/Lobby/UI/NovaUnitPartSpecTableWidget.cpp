// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/UI/NovaUnitPartSpecTableWidget.h"
#include "Lobby/UI/NovaUnitPartSpecEntryWidget.h"

void UNovaUnitPartSpecTableWidget::UpdateTable(const FNovaPartSpecRow& Spec)
{
	// 1. 공통 및 문자열 데이터 설정
	//if (Entry_PartName) Entry_PartName->SetStatText(TEXT("부품 명칭"), Spec.PartName);
	if (Entry_Watt)     Entry_Watt->SetStatData(TEXT("소모 와트(W)"), Spec.Watt);
	if (Entry_Weight)   Entry_Weight->SetStatData(TEXT("부품 무게(WT)"), Spec.Weight);

	// 2. 기본 스펙 데이터 설정
	if (Entry_Health)   Entry_Health->SetStatData(TEXT("추가 체력(HP)"), Spec.Health);
	if (Entry_Attack)   Entry_Attack->SetStatData(TEXT("공격력(ATK)"), Spec.Attack);
	if (Entry_Defense)  Entry_Defense->SetStatData(TEXT("방어력(DEF)"), Spec.Defense);
    
	// 이동 속도와 연사력은 정밀한 수치 확인을 위해 소수점(false)을 표시합니다.
	if (Entry_Speed)    Entry_Speed->SetStatData(TEXT("이동 속도(SPD)"), Spec.Speed, false);
	if (Entry_FireRate) Entry_FireRate->SetStatData(TEXT("연사력(FR)"), Spec.FireRate, false);
	if (Entry_Sight)    Entry_Sight->SetStatData(TEXT("시야 범위(VIS)"), Spec.Sight);

	// 3. 전투 및 특수 데이터 설정
	if (Entry_Range)       Entry_Range->SetStatData(TEXT("최대 사거리"), Spec.Range);
	if (Entry_MinRange)    Entry_MinRange->SetStatData(TEXT("최소 사거리"), Spec.MinRange);
	if (Entry_SplashRange) Entry_SplashRange->SetStatData(TEXT("폭발 범위"), Spec.SplashRange);

	// 4. 열거형(Enum) 데이터 텍스트 변환 및 설정
	if (Entry_MovementType)
	{
		FString MoveStr = (Spec.MovementType == ENovaMovementType::Ground) ? TEXT("지상") : TEXT("공중");
		Entry_MovementType->SetStatText(TEXT("이동 방식"), MoveStr);
	}

	if (Entry_TargetType)
	{
		FString TargetStr = TEXT("전체");
		if (Spec.TargetType == ENovaTargetType::GroundOnly) TargetStr = TEXT("지상 전용");
		else if (Spec.TargetType == ENovaTargetType::AirOnly) TargetStr = TEXT("공중 전용");
        
		Entry_TargetType->SetStatText(TEXT("공격 대상"), TargetStr);
	}

	// 5. 불리언(Bool) 데이터 텍스트 변환 및 설정
	if (Entry_Homing)
	{
		Entry_Homing->SetStatText(TEXT("유도 기능"), Spec.bHomingProjectile ? TEXT("지원") : TEXT("미지원"));
	}
	
	// 6. 충돌 반경(Collision Radius) 설정 [추가]
	if (Entry_CollisionRadius)
	{
		// 유닛의 물리적 크기나 점유 면적을 결정하는 중요한 수치를 UI에 표시합니다.
		Entry_CollisionRadius->SetStatData(TEXT("충돌 반경"), Spec.CollisionRadius);
	}
	
}
