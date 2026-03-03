// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaUnit.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GAS/NovaAttributeSet.h"
#include "Components/CapsuleComponent.h"

ANovaUnit::ANovaUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	// ASC 및 AttributeSet 구성
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UNovaAttributeSet>(TEXT("AttributeSet"));

	// 기본 유닛 설정
	Team = ENovaTeam::None;
}

void ANovaUnit::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		// ASC 초기화 (ActorInfo 설정)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 속성 변경 콜백 등록 (예: 체력 변경 시 UI 업데이트 또는 사망 처리)
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
			.AddUObject(this, &ANovaUnit::OnHealthChanged);
	}
}

UAbilitySystemComponent* ANovaUnit::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ANovaUnit::OnSelected()
{
	bIsSelected = true;
	// TODO: 팀원 B가 구현할 하이라이트/데칼 로직이 들어갈 자리
	UE_LOG(LogTemp, Log, TEXT("Unit Selected: %s"), *GetName());
}

void ANovaUnit::OnDeselected()
{
	bIsSelected = false;
	// TODO: 팀원 B가 구현할 하이라이트 해제 로직
	UE_LOG(LogTemp, Log, TEXT("Unit Deselected: %s"), *GetName());
}

bool ANovaUnit::IsSelectable() const
{
	// 플레이어 팀만 선택 가능하게 하려면 여기서 필터링 가능 (기본값 true)
	return true;
}

void ANovaUnit::IssueCommand(const FCommandData& CommandData)
{
	// TODO: 팀원 C가 구현할 AIController로의 명령 전달 로직
	// 예: GetController<ANovaAIController>()->ReceiveCommand(CommandData);
	UE_LOG(LogTemp, Log, TEXT("Unit Received Command: Type %d"), (int32)CommandData.CommandType);
}

void ANovaUnit::Die()
{
	// 유닛 사망 처리 로직
	UE_LOG(LogTemp, Error, TEXT("Unit Died: %s"), *GetName());

	// 충돌 비활성화 및 소멸 처리 (필요에 따라 래그돌 또는 파편화 연출 추가 가능)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// TODO: 팀원들과 상의하여 유닛 소멸 방식 결정 (즉시 Destroy 또는 애니메이션 대기)
	// Destroy();
}

void ANovaUnit::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	// 체력이 0 이하가 되면 Die() 호출
	if (Data.NewValue <= 0.0f)
	{
		Die();
	}
}
