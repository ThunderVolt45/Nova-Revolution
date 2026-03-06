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

	// 2. 주변 액터 탐색
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(MyUnit);

	bool bHit = GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		MyUnit->GetActorLocation(),
		FQuat::Identity,
		ECC_Pawn, // 유닛 탐색용 채널 (기획에 따라 조정 가능)
		FCollisionShape::MakeSphere(FinalSearchRadius),
		CollisionParams
	);

	AActor* NearestEnemy = nullptr;
	float MinDistanceSq = MAX_FLT;

	if (bHit)
	{
		int32 MyTeamID = MyUnit->GetTeamID();

		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* PotentialTarget = Result.GetActor();
			if (!PotentialTarget) continue;

			// 팀 인터페이스 확인
			if (INovaTeamInterface* TeamInterface = Cast<INovaTeamInterface>(PotentialTarget))
			{
				// 적군인 경우에만 처리
				if (!TeamInterface->IsFriendly(MyTeamID))
				{
					// 사망 여부 확인 (INovaSelectableInterface 등 활용 가능)
					// 여기서는 간단하게 Actor의 생존 여부 확인
					float DistSq = FVector::DistSquared(MyUnit->GetActorLocation(), PotentialTarget->GetActorLocation());
					if (DistSq < MinDistanceSq)
					{
						MinDistanceSq = DistSq;
						NearestEnemy = PotentialTarget;
					}
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
