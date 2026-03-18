// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AI/NovaBTService_FindTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Core/NovaUnit.h"
#include "Core/NovaInterfaces.h"
#include "AbilitySystemComponent.h"
#include "GAS/NovaAttributeSet.h"
#include "Engine/OverlapResult.h"
#include "NovaRevolution.h"

UNovaBTService_FindTarget::UNovaBTService_FindTarget()
{
	NodeName = TEXT("Find Nearest Enemy");
	Interval = 0.5f;
	RandomDeviation = 0.1f;

	// 블랙보드 키 필터링
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTService_FindTarget, TargetActorKey), AActor::StaticClass());
	CommandTypeKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(UNovaBTService_FindTarget, CommandTypeKey), StaticEnum<ECommandType>());
}

void UNovaBTService_FindTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	// 현재 명령 상태 확인
	uint8 CommandTypeValue = BB->GetValueAsEnum(CommandTypeKey.SelectedKeyName);
	ECommandType CurrentCommand = static_cast<ECommandType>(CommandTypeValue);

	// 공격(Attack) 또는 순찰(Patrol) 상태에서는 타겟이 없을 때만 탐색
	if (CurrentCommand == ECommandType::Attack || CurrentCommand == ECommandType::Patrol)
	{
		if (BB->GetValueAsObject(TargetActorKey.SelectedKeyName) != nullptr)
		{
			return;
		}
	}
	// Hold 상태이거나 명령이 없는(None) 경우에만 자동 탐색 허용
	else if (CurrentCommand != ECommandType::None && CurrentCommand != ECommandType::Hold)
	{
		return;
	}

	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (!ControllingPawn) return;

	ANovaUnit* MyUnit = Cast<ANovaUnit>(ControllingPawn);
	if (!MyUnit) return;

	// 1. 탐색 범위 결정: 시야(Sight)와 사거리(Range) 중 더 큰 값 사용
	float FinalSearchRadius = 1000.0f; // 기본값
	if (UAbilitySystemComponent* ASC = MyUnit->GetAbilitySystemComponent())
	{
		float Sight = ASC->GetNumericAttribute(UNovaAttributeSet::GetSightAttribute());
		float Range = ASC->GetNumericAttribute(UNovaAttributeSet::GetRangeAttribute());
		
		FinalSearchRadius = FMath::Max(Sight, Range);
	}

	// 2. 주변 액터 탐색 (유닛 및 기지/건물 포함)
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(ControllingPawn);

	// Pawn뿐만 아니라 WorldStatic, WorldDynamic 오브젝트들도 탐색 대상에 포함 (기지 등 대응)
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	// [수정] Sphere 대신 Capsule(원기둥) 형상을 사용하여 고도 차이에 유연하게 대응
	// Radius는 탐색 범위, HalfHeight는 1500.f로 설정하여 지상/공중 통합 감지
	bool bHit = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		ControllingPawn->GetActorLocation(),
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeCapsule(FinalSearchRadius, 1500.0f),
		CollisionParams
	);

	AActor* NearestEnemy = nullptr;
	float MinDistanceSq = MAX_FLT;

	if (bHit)
	{
		int32 MyTeamID = MyUnit->GetTeamID();
		ENovaTargetType MyWeaponTargetType = MyUnit->GetTargetType();

		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* PotentialTarget = Result.GetActor();
			
			// 자기 자신 제외 및 유효성 확인
			if (!PotentialTarget || PotentialTarget == MyUnit) continue;

			// 1. 공격 가능 대상 확인 (ASC 구현 여부)
			IAbilitySystemInterface* ASCTarget = Cast<IAbilitySystemInterface>(PotentialTarget);
			if (!ASCTarget) continue;

			// 2. 팀 확인 (적군인 경우에만 처리)
			INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(PotentialTarget);
			if (!TeamInterface || TeamInterface->IsFriendly(MyTeamID)) continue;

			// 3. 공격 가능 여부 확인 (지상/공중 타입 필터링)
			bool bCanAttack = false;
			ANovaUnit* TargetUnit = Cast<ANovaUnit>(PotentialTarget);

			if (TargetUnit)
			{
				ENovaMovementType TargetMoveType = TargetUnit->GetMovementType();
				
				switch (MyWeaponTargetType)
				{
				case ENovaTargetType::GroundOnly:
					bCanAttack = (TargetMoveType == ENovaMovementType::Ground);
					break;
				case ENovaTargetType::AirOnly:
					bCanAttack = (TargetMoveType == ENovaMovementType::Air);
					break;
				case ENovaTargetType::All:
					bCanAttack = true;
					break;
				case ENovaTargetType::None:
					bCanAttack = false;
					break;
				}
			}
			else
			{
				// 유닛이 아니지만 ASC를 가진 대상(기지 등)은 기본적으로 지상 타겟으로 간주
				bCanAttack = (MyWeaponTargetType != ENovaTargetType::AirOnly);
			}

			if (!bCanAttack) continue;

			// [추가] 최소 사거리 내에 있는 적은 탐색 대상에서 제외
			if (MyUnit->IsTargetTooClose(PotentialTarget)) continue;

			// 4. [수정] 최단 거리 적 갱신 시 캡슐 기반 사거리 판정 함수 활용
			if (MyUnit->IsTargetInRange(PotentialTarget, FinalSearchRadius))
			{
				float DistSq = FVector::DistSquaredXY(MyUnit->GetActorLocation(), PotentialTarget->GetActorLocation());
				if (DistSq < MinDistanceSq)
				{
					MinDistanceSq = DistSq;
					NearestEnemy = PotentialTarget;
				}
			}
		}
	}

	// 3. 블랙보드 업데이트
	OwnerComp.GetBlackboardComponent()->SetValueAsObject(TargetActorKey.SelectedKeyName, NearestEnemy);
	
	if (NearestEnemy)
	{
		NOVA_LOG(Log, "Unit %s found target: %s", *MyUnit->GetName(), *NearestEnemy->GetName());
	}
}
