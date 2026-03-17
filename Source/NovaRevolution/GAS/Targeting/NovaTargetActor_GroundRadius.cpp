// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Targeting/NovaTargetActor_GroundRadius.h"

#include "NovaRevolution.h"
#include "Abilities/GameplayAbility.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h" // 추가: APlayerState를 사용하기 위해 필수
#include "Core/NovaInterfaces.h" // TeamID 확인을 위한 인터페이스
#include "Core/NovaLog.h"
#include "Core/NovaTypes.h" // 추가: NovaTeam::None 사용을 위해 필수
#include "Core/NovaUnit.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"


ANovaTargetActor_GroundRadius::ANovaTargetActor_GroundRadius()
{
	// 마우스 추적을 위해 틱 활성화
	PrimaryActorTick.bCanEverTick = true;

	// 1. 컴포넌트 생성 및 설정
	CollisionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	RootComponent = CollisionCapsule;

	// 2. 물리 설정: 오직 쿼리(Query)만 수행하도록 설정
	CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 유닛(Pawn)만 감지

	// 3. 기본 크기 (Radius는 GA에서 넘겨받은 값을 Tick에서 동적으로 적용 가능)
	CollisionCapsule->SetCapsuleRadius(Radius);
	CollisionCapsule->SetCapsuleHalfHeight(1000.0f); // 공중 유닛을 잡기 위한 충분한 높이

	// 4. 디버깅용 시각화 활성화
	CollisionCapsule->SetHiddenInGame(false);
}

void ANovaTargetActor_GroundRadius::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector MouseLoc = GetMouseLocationOnGround();
	SetActorLocation(MouseLoc);

	// 2. 현재 범위 내 아군 유닛 목록 가져오기
	TArray<AActor*> CurrentActors;
	GetFilteredActorsInRange(CurrentActors);

	// 가독성을 위해 ANovaUnit 포인터 배열로 변환
	TArray<TWeakObjectPtr<ANovaUnit>> CurrentUnits;
	for (AActor* Actor : CurrentActors)
	{
		if (ANovaUnit* Unit = Cast<ANovaUnit>(Actor))
			CurrentUnits.Add(Unit);
	}

	// 3. [하이라이트 로직]
	// (A) 영역을 벗어난 유닛들: 이전 목록에는 있지만 현재 목록에는 없는 경우 -> 끄기
	for (int32 i = LastHighlightedUnits.Num() - 1; i >= 0; --i)
	{
		TWeakObjectPtr<ANovaUnit> OldUnit = LastHighlightedUnits[i];
		// 유닛이 유효하지 않거나(파괴됨), 현재 감지 범위 내에 없다면 하이라이트 해제
		if (OldUnit.IsValid() && !CurrentUnits.Contains(OldUnit))
		{
			// OldUnit->SetHighlight(false);
			// LastHighlightedUnits.RemoveAt(i);
			// [인터페이스 호출] 스킬 하이라이트 해제
			if (INovaHighlightInterface* Highlighter = Cast<INovaHighlightInterface>(OldUnit))
			{
				Highlighter->SetHighlightStatus(ENovaHighlightPriority::SkillRange, false);
			}
			LastHighlightedUnits.RemoveAt(i);
		}
		else if (!OldUnit.IsValid())
		{
			// 유닛이 이미 파괴된 경우 리스트에서만 제거
			LastHighlightedUnits.RemoveAt(i);
		}
	}

	// (B) 새로 들어온 유닛들: 현재 목록에는 있지만 이전 목록에는 없는 경우 -> 켜기
	for (auto& NewUnit : CurrentUnits)
	{
		if (NewUnit.IsValid() && !LastHighlightedUnits.Contains(NewUnit))
		{
			// 흰색 하이라이트 적용 (소환 대기 상태 시각화)
			// NewUnit->SetHighlight(true, CurrentHighlightColor);
			// LastHighlightedUnits.Add(NewUnit);
			// [인터페이스 호출] 스킬 하이라이트 활성화
			if (INovaHighlightInterface* Highlighter = Cast<INovaHighlightInterface>(NewUnit))
			{
				Highlighter->SetHighlightStatus(ENovaHighlightPriority::SkillRange, true, CurrentHighlightColor);
				LastHighlightedUnits.Add(NewUnit);
			}
		}
	}


	if (CurrentUnits.Num() > 0)
	{
		FString UnitNames = "";
		for (AActor* Actor : CurrentActors)
		{
			if (ANovaUnit* Unit = Cast<ANovaUnit>(Actor))
			{
				UnitNames += Unit->GetUnitName() + TEXT(", ");
			}
		}
		UnitNames.RemoveFromEnd(TEXT(", "));
		NOVA_SCREEN(Log, "Units in Range (%d): %s", CurrentUnits.Num(), *UnitNames);
	}
	else
	{
		NOVA_SCREEN(Log, "No Allied Units in Range.");
	}

	// 지면(XY평면)에 평행한 원 그리기
	FMatrix CircleMatrix = FMatrix::Identity;
	CircleMatrix.SetOrigin(MouseLoc + FVector(0.f, 0.f, 5.f)); // 지면보다 살짝 위에 그림 (Z-Fighting 방지)
	CircleMatrix.SetAxis(2, FVector::ForwardVector); // 원이 바라보는 방향 (Up축 설정)

	DrawDebugCircle(
		GetWorld(),
		CircleMatrix,
		Radius,
		64, // 세그먼트 수 (64 정도면 충분히 부드러움)
		FColor::Cyan, // 하늘색 원형 가이드
		false, // Persistent (한 프레임만 표시)
		-1.0f, // Lifetime
		0, // Depth Priority
		3.0f, // 선 두께
		false // 원 중심에서 선을 그릴지 여부
	);
}

void ANovaTargetActor_GroundRadius::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);

	// [중요] GA에서 TargetActor->Radius = RecallRadius; 라고 설정한 직후에 이 함수가 호출됩니다.
	// 이때 딱 한 번 캡슐의 물리적 반지름을 설정합니다.
	if (CollisionCapsule)
	{
		CollisionCapsule->SetCapsuleRadius(Radius);
		CollisionCapsule->SetCapsuleHalfHeight(2000.0f); // 공중 유닛 감지 높이 고정
	}
}

void ANovaTargetActor_GroundRadius::ConfirmTargetingAndContinue()
{
	if (!PrimaryPC)
	{
		CancelTargeting();
		return;
	}

	// [수정됨] 공통 함수를 사용하여 최종 타겟을 확정합니다.
	TArray<AActor*> FinalUnits;
	GetFilteredActorsInRange(FinalUnits);

	if (FinalUnits.Num() > 0)
	{
		TArray<TWeakObjectPtr<AActor>> FilteredWeakActors;
		for (AActor* Actor : FinalUnits) FilteredWeakActors.Add(Actor);

		FGameplayAbilityTargetDataHandle Handle = StartLocation.MakeTargetDataHandleFromActors(FilteredWeakActors);
		TargetDataReadyDelegate.Broadcast(Handle);
	}
	else
	{
		CancelTargeting();
	}
}

bool ANovaTargetActor_GroundRadius::IsValidTarget(int32 MyTeamID, int32 TargetTeamID) const
{
	// NovaTeam 네임스페이스와 비교하여 필터링 수행
	switch (TargetingFilter)
	{
	case ENovaTargetingFilter::Ally:
		return MyTeamID == TargetTeamID;

	case ENovaTargetingFilter::Enemy:
		// 내 팀이 아니고, 중립/없음 상태가 아닌 경우 적군으로 간주
		return (MyTeamID != TargetTeamID) && (TargetTeamID != NovaTeam::None);

	case ENovaTargetingFilter::Both:
		return true;
	}

	return false;
}

void ANovaTargetActor_GroundRadius::GetFilteredActorsInRange(TArray<AActor*>& OutActors) const
{
	OutActors.Empty();
	if (!PrimaryPC) return;

	// [매우 간결해진 로직] 컴포넌트가 이미 감지 중인 오버랩 목록을 즉시 가져옵니다.
	TArray<AActor*> OverlappingActors;
	CollisionCapsule->GetOverlappingActors(OverlappingActors, ANovaUnit::StaticClass());

	//GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn, Sphere, Params);

	// 내 팀 ID 확인
	int32 MyTeamID = NovaTeam::None;
	if (INovaTeamInterface* TeamInterface = PrimaryPC->GetPlayerState<INovaTeamInterface>())
	{
		MyTeamID = TeamInterface->GetTeamID();
	}

	for (AActor* Actor : OverlappingActors)
	{
		ANovaUnit* Unit = Cast<ANovaUnit>(Actor);
		// 사망하지 않았으며 필터링 조건에 맞는 유닛만 선별
		if (Unit && !Unit->IsDead() && IsValidTarget(MyTeamID, Unit->GetTeamID()))
		{
			OutActors.Add(Actor);
		}
	}
}

void ANovaTargetActor_GroundRadius::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 스킬이 끝나거나 취소되어 액터가 사라질 때, 남아있는 모든 하이라이트를 끕니다.
	for (auto& Unit : LastHighlightedUnits)
	{
		if (Unit.IsValid())
		{
			// Unit->SetHighlight(false);
			if (INovaHighlightInterface* Highlighter = Cast<INovaHighlightInterface>(Unit))
			{
				Highlighter->SetHighlightStatus(ENovaHighlightPriority::SkillRange, false);
			}
		}
	}
	LastHighlightedUnits.Empty();

	Super::EndPlay(EndPlayReason);
}

FVector ANovaTargetActor_GroundRadius::GetMouseLocationOnGround() const
{
	if (PrimaryPC)
	{
		FHitResult Hit;
		// ECC_Visibility 대신 지형(WorldStatic)만 감지하거나, 유닛을 무시하는 설정을 추가합니다.
		FCollisionQueryParams Params;

		// 유닛 클래스들을 트레이스에서 제외 (커서가 유닛에 가려 지면 좌표를 놓치는 현상 방지)
		TArray<AActor*> AllUnits;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaUnit::StaticClass(), AllUnits);
		Params.AddIgnoredActors(AllUnits);

		// 지면(보통 WorldStatic) 채널을 우선적으로 체크
		if (PrimaryPC->GetHitResultUnderCursor(ECC_WorldStatic, false, Hit))
		{
			return Hit.ImpactPoint;
		}
	}
	return FVector::ZeroVector;
}
