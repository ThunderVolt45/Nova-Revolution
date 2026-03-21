// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/Deck/NovaDeckSlot.h"

#include "NovaRevolution.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Core/NovaLog.h"
#include "Lobby/NovaLobbyPlayerController.h"
#include "Lobby/NovaLobbyManager.h"

ANovaDeckSlot::ANovaDeckSlot()
{
    // 1. 루트 컴포넌트 생성 (DefaultSceneRoot 역할)
    // 루트를 SceneComponent로 설정함으로써, BP 에디터에서 하위 메쉬나 조명 등을 
    // 자유롭게 추가하고 상대적 위치를 조정할 수 있는 유연성을 제공합니다.
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    // 2. 베이스 메쉬 생성 및 부착
    // 격납고 바닥이나 슬롯의 기준이 될 외형 메쉬를 담당합니다.
    // 마우스 상호작용(하이라이트 등)의 주요 대상이 됩니다.
    BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
    BaseMesh->SetupAttachment(RootComponent);

    // 3. 유닛 스폰 위치 생성 및 부착
    // 유닛(PreviewUnit)이 슬롯 위에 서 있을 정확한 위치를 정의합니다.
    // 코드 수정 없이 BP 에디터에서 이 컴포넌트만 옮겨서 스폰 지점을 미세 조정할 수 있습니다.
    UnitSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("UnitSpawnPoint"));
    UnitSpawnPoint->SetupAttachment(RootComponent);

    // 성능 최적화: 매 프레임 실행되는 틱(Tick) 로직이 필요 없으므로 비활성화합니다.
    PrimaryActorTick.bCanEverTick = false;
}

void ANovaDeckSlot::BeginPlay()
{
    Super::BeginPlay();

    Super::BeginPlay();

    // BaseMesh에 유효한 머티리얼이 있다면 하이라이트 제어용 다이나믹 머티리얼 생성
    if (BaseMesh && BaseMesh->GetMaterial(0))
    {
        BaseDynamicMaterial = BaseMesh->CreateAndSetMaterialInstanceDynamic(0);
    }
}

void ANovaDeckSlot::NotifyActorBeginCursorOver()
{
    // 마우스가 슬롯 영역에 들어오면 하이라이트 활성화
    SetHighlight(true);
}

void ANovaDeckSlot::NotifyActorEndCursorOver()
{
    // 마우스가 영역을 벗어나면 하이라이트 비활성화
    SetHighlight(false);
}

void ANovaDeckSlot::NotifyActorOnClicked(FKey ButtonPressed)
{
    // 클릭 시 플레이어 컨트롤러를 통해 로비 매니저에 접근
    if (ANovaLobbyPlayerController* PC = Cast<ANovaLobbyPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        if (ANovaLobbyManager* Manager = PC->GetLobbyManager())
        {
            // 이 슬롯의 고유 인덱스(SlotIndex)를 매니저에게 전달하여 해당 덱 편집 모드로 진입
            Manager->SelectDeckSlot(SlotIndex);
            
            // 클릭 피드백 로그
            NOVA_LOG(Log, "Deck Slot %d Clicked - Transitioning to Edit Mode.", SlotIndex);
        }
    }
}


// --- 하이라이트 로직 (BlueprintNativeEvent의 C++ 구현부) ---
void ANovaDeckSlot::SetHighlight_Implementation(bool bIsOn)
{
    // 1. C++ 기본 로직: BaseMesh의 머티리얼 파라미터를 조절하여 시각적 피드백 제공
    if (BaseDynamicMaterial)
    {
        // 머티리얼 에셋 내의 'HighlightPower' 스칼라 파라미터 이름을 참조합니다.
        // 마우스 오버 시 발광 강도를 2.0으로 높이고, 평소에는 0으로 유지합니다.
        BaseDynamicMaterial->SetScalarParameterValue(TEXT("HighlightPower"), bIsOn ? 2.0f : 0.0f);
    }

    // 2. 추가적인 조명 제어나 사운드, 이펙트 등은 블루프린트에서 
    // 'Event Set Highlight'를 오버라이드하여 자유롭게 확장할 수 있습니다.
}

FTransform ANovaDeckSlot::GetUnitSpawnTransform() const
{
    if (UnitSpawnPoint)
    {
        return UnitSpawnPoint->GetComponentTransform();
    }
    
    return GetActorTransform();
}
