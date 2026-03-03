// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaUnit.h"
#include "Core/NovaPart.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GAS/NovaAttributeSet.h"
#include "Components/CapsuleComponent.h"

ANovaUnit::ANovaUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	// 상속받은 기본 메시는 사용하지 않으므로 숨김 및 충돌 비활성화
	if (GetMesh())
	{
		GetMesh()->SetHiddenInGame(true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// ASC 및 AttributeSet 구성
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UNovaAttributeSet>(TEXT("AttributeSet"));

	// --- 차일드 액터 컴포넌트 초기화 (런타임 생성용) ---
	// Legs(다리)는 캡슐(Root)에 직접 부착
	LegsPartComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("LegsPartComponent"));
	LegsPartComponent->SetupAttachment(GetRootComponent());

	// Body(몸통)는 다리의 소켓에 부착될 준비 (BeginPlay에서 처리)
	BodyPartComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("BodyPartComponent"));
	BodyPartComponent->SetupAttachment(LegsPartComponent);

	// 기본 유닛 설정
	Team = ENovaTeam::None;
}

void ANovaUnit::BeginPlay()
{
	Super::BeginPlay();

	// 1. 설정된 클래스 정보를 바탕으로 부품 생성 및 할당
	ConstructUnitParts();

	// 2. 부품들 실제 소켓에 정렬 부착
	InitializePartAttachments();

	if (AbilitySystemComponent)
	{
		// ASC 초기화 (ActorInfo 설정)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 속성 변경 콜백 등록
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
			.AddUObject(this, &ANovaUnit::OnHealthChanged);
	}
}

void ANovaUnit::ConstructUnitParts()
{
	// 다리(Legs) 할당
	if (LegsPartClass)
	{
		LegsPartComponent->SetChildActorClass(LegsPartClass);
	}

	// 몸통(Body) 할당
	if (BodyPartClass)
	{
		BodyPartComponent->SetChildActorClass(BodyPartClass);
	}

	// 무기(Weapons) 할당 및 컴포넌트 생성
	for (int32 i = 0; i < WeaponSlotConfigs.Num(); ++i)
	{
		const FNovaWeaponPartSlot& Config = WeaponSlotConfigs[i];
		if (Config.WeaponPartClass)
		{
			// 무기 컴포넌트 동적 생성 및 등록
			FName CompName = FName(*FString::Printf(TEXT("WeaponPartComponent_%d"), i));
			UChildActorComponent* WeaponComp = NewObject<UChildActorComponent>(this, CompName);
			
			if (WeaponComp)
			{
				WeaponComp->RegisterComponent();
				WeaponComp->SetChildActorClass(Config.WeaponPartClass);
				WeaponComp->AttachToComponent(BodyPartComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
				
				WeaponPartComponents.Add(WeaponComp);
			}
		}
	}
}

void ANovaUnit::InitializePartAttachments()
{
	// 1. 다리(Legs) 내에서 몸통(Body)이 붙을 소켓을 찾음
	if (ANovaPart* LegsActor = Cast<ANovaPart>(LegsPartComponent->GetChildActor()))
	{
		if (UPrimitiveComponent* LegsMesh = LegsActor->GetMainMesh())
		{
			// 몸통을 지정된 소켓에 부착
			BodyPartComponent->AttachToComponent(LegsMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, BodyTargetSocketName);
		}
	}

	// 2. 몸통(Body) 내에서 무기(Weapons)들이 붙을 소켓들을 찾음
	if (ANovaPart* BodyActor = Cast<ANovaPart>(BodyPartComponent->GetChildActor()))
	{
		if (UPrimitiveComponent* BodyMesh = BodyActor->GetMainMesh())
		{
			for (int32 i = 0; i < WeaponPartComponents.Num(); ++i)
			{
				if (i < WeaponSlotConfigs.Num())
				{
					// 각 무기를 슬롯 설정에 지정된 전용 소켓에 부착
					WeaponPartComponents[i]->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponSlotConfigs[i].TargetSocketName);
				}
			}
		}
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
