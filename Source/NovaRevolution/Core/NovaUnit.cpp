// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/NovaUnit.h"
#include "Core/NovaPart.h"
#include "Core/NovaPartData.h"
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

void ANovaUnit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 1. 설정된 클래스 정보를 바탕으로 부품 생성 및 할당 (에디터 프리뷰용)
	ConstructUnitParts();

	// 2. 부품들 실제 소켓에 정렬 부착
	InitializePartAttachments();
}

void ANovaUnit::BeginPlay()
{
	Super::BeginPlay();

	// 런타임 시작 시 다시 한번 초기화 및 부착 보강
	ConstructUnitParts();
	InitializePartAttachments();

	// 3. 모든 부품이 부착된 후 스탯 합산 및 초기화
	InitializeAttributesFromParts();

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

	// 기존에 동적으로 생성된 무기 컴포넌트들 제거 (중복 생성 방지)
	for (UChildActorComponent* Comp : WeaponPartComponents)
	{
		if (Comp)
		{
			Comp->DestroyComponent();
		}
	}
	WeaponPartComponents.Empty();

	// 무기(Weapons) 할당 및 컴포넌트 생성
	for (int32 i = 0; i < WeaponSlotConfigs.Num(); ++i)
	{
		const FNovaWeaponPartSlot& Config = WeaponSlotConfigs[i];
		if (Config.WeaponPartClass)
		{
			// 무기 컴포넌트 동적 생성 및 등록
			FName CompName = FName(*FString::Printf(TEXT("WeaponPartComponent_%d"), i));
			
			// NewObject 시 이름을 명시하여 고유성 유지
			UChildActorComponent* WeaponComp = NewObject<UChildActorComponent>(this, CompName);
			
			if (WeaponComp)
			{
				WeaponComp->CreationMethod = EComponentCreationMethod::UserConstructionScript;
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

void ANovaUnit::InitializeAttributesFromParts()
{
	if (!AttributeSet) return;

	// 임시 합산 변수들
	float TotalWatt = 0.0f;
	float TotalHealth = 0.0f;
	float TotalAttack = 0.0f;
	float TotalDefense = 0.0f;
	float TotalSpeed = 0.0f;
	float TotalFireRate = 0.0f;
	float TotalSight = 0.0f;
	float TotalRange = 0.0f;
	float TotalMinRange = 0.0f;
	float TotalSplashRange = 0.0f;

	// 수집할 모든 부품 액터들을 리스트업
	TArray<AActor*> PartActors;
	PartActors.Add(LegsPartComponent->GetChildActor());
	PartActors.Add(BodyPartComponent->GetChildActor());
	for (auto WeaponComp : WeaponPartComponents)
	{
		PartActors.Add(WeaponComp->GetChildActor());
	}

	// 각 부품에서 스탯 수집
	for (AActor* Actor : PartActors)
	{
		if (ANovaPart* Part = Cast<ANovaPart>(Actor))
		{
			if (const UNovaPartData* Data = Part->GetPartData())
			{
				TotalWatt += Data->Watt;
				TotalHealth += Data->Health;
				TotalAttack += Data->Attack;
				TotalDefense += Data->Defense;
				TotalSpeed += Data->Speed;
				TotalFireRate += Data->FireRate;
				TotalSight += Data->Sight;
				TotalRange += Data->Range;
				TotalMinRange += Data->MinRange;
				TotalSplashRange += Data->SplashRange;
			}
		}
	}

	// AttributeSet 초기화
	AttributeSet->InitWatt(TotalWatt);
	AttributeSet->InitHealth(TotalHealth);
	AttributeSet->InitMaxHealth(TotalHealth); // MaxHealth도 동일하게 초기화
	AttributeSet->InitAttack(TotalAttack);
	AttributeSet->InitDefense(TotalDefense);
	AttributeSet->InitSpeed(TotalSpeed);
	AttributeSet->InitFireRate(TotalFireRate);
	AttributeSet->InitSight(TotalSight);
	AttributeSet->InitRange(TotalRange);
	AttributeSet->InitMinRange(TotalMinRange);
	AttributeSet->InitSplashRange(TotalSplashRange);

	UE_LOG(LogTemp, Log, TEXT("Unit Stats Initialized: HP(%f), Watt(%f), Attack(%f)"), TotalHealth, TotalWatt, TotalAttack);
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
