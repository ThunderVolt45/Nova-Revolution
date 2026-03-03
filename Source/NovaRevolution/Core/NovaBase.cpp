// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaBase.h"
#include "Core/NovaGameMode.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GAS/NovaAttributeSet.h"

ANovaBase::ANovaBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// 메시 컴포넌트 생성 및 루트 설정
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;

	// ASC 및 AttributeSet 구성
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UNovaAttributeSet>(TEXT("AttributeSet"));

	// 초기 설정
	TeamID = NovaTeam::None;
	RallyPoint = FVector::ZeroVector;
}

void ANovaBase::BeginPlay()
{
	Super::BeginPlay();

	// 랠리 포인트 초기값 설정 (기지 앞쪽 500 유닛 지점 예시)
	RallyPoint = GetActorLocation() + GetActorForwardVector() * 500.0f;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 체력 변경 콜백 등록
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
			.AddUObject(this, &ANovaBase::OnHealthChanged);
	}
}

UAbilitySystemComponent* ANovaBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ANovaBase::OnSelected()
{
	bIsSelected = true;
	UE_LOG(LogTemp, Log, TEXT("Base Selected: %s (TeamID: %d)"), *GetName(), TeamID);
}

void ANovaBase::OnDeselected()
{
	bIsSelected = false;
	UE_LOG(LogTemp, Log, TEXT("Base Deselected: %s"), *GetName());
}

void ANovaBase::DestroyBase()
{
	// 기지 파괴 시 시각적/게임 로직 처리
	UE_LOG(LogTemp, Error, TEXT("Base Destroyed: %s (TeamID: %d)"), *GetName(), TeamID);

	// GameMode에 기지 파괴 이벤트 전달
	if (ANovaGameMode* GM = GetWorld()->GetAuthGameMode<ANovaGameMode>())
	{
		GM->OnBaseDestroyed(this);
	}

	Destroy();
}

void ANovaBase::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.0f)
	{
		DestroyBase();
	}
}
