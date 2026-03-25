// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/Deck/NovaDeckSlot.h"

#include "NovaRevolution.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Core/NovaLog.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Lobby/NovaLobbyPlayerController.h"
#include "Lobby/NovaLobbyManager.h"

ANovaDeckSlot::ANovaDeckSlot()
{
    // 1. 루트 컴포넌트 생성 (DefaultSceneRoot 역할)
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
    
    // Scene Capture 컴포넌트 생성 및 부착
    SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
    SceneCapture->SetupAttachment(RootComponent);

    // 유닛을 바라보도록 위치와 회전 기본값 설정 (에디터에서 미세조정 가능)
    SceneCapture->SetRelativeLocation(FVector(300.0f, 0.0f, 150.0f));
    SceneCapture->SetRelativeRotation(FRotator(-15.0f, 180.0f, 0.0f));

    // --- [성능 최적화: 수동 캡처 방식으로 변경] ---
    SceneCapture->bCaptureEveryFrame = false; 
    SceneCapture->bCaptureOnMovement = false;

    // 투명화 및 배경 제거를 위한 캡처 설정
    //알파값 반전 : 기존의 M_PartPreview_UI Material 사용
    SceneCapture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
    // 알파값 정상 : ToDo: 이후 FinalColorLDR로 변경
    //SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    SceneCapture->CompositeMode = ESceneCaptureCompositeMode::SCCM_Overwrite;

    // 불필요한 배경 요소 강제 비활성화 (알파 채널 확보용)
    SceneCapture->ShowFlags.SetAtmosphere(false);
    SceneCapture->ShowFlags.SetSkyLighting(false);
    SceneCapture->ShowFlags.SetFog(false);
    SceneCapture->ShowFlags.SetVolumetricFog(false);
    
    // 성능 최적화: 매 프레임 실행되는 틱(Tick) 로직이 필요 없으므로 비활성화합니다.
    PrimaryActorTick.bCanEverTick = false;
}

void ANovaDeckSlot::BeginPlay()
{
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

void ANovaDeckSlot::SetCaptureTarget(AActor* TargetUnit)
{
    if (!SceneCapture || !RenderTarget) return;

    // 기존 캡처 리스트 초기화 및 렌더 타겟 비우기 (투명 처리)
    SceneCapture->ShowOnlyActors.Empty();
    RenderTarget->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
    SceneCapture->TextureTarget = RenderTarget;

    if (TargetUnit)
    {
        // 1. 유닛 본체(Base Actor) 추가
        SceneCapture->ShowOnlyActors.Add(TargetUnit);

        // 2. 유닛에 부착된 모든 부품 액터(다리, 몸통, 무기 등)를 재귀적으로 찾아 추가
        TArray<AActor*> AttachedActors;
        TargetUnit->GetAttachedActors(AttachedActors, false, true); 
        
        for (AActor* Attached : AttachedActors)
        {
            if (Attached)
            {
                SceneCapture->ShowOnlyActors.Add(Attached);
            }
        }
    }
    
    // --- [성능 및 품질 최적화: 지연 캡처 로직] ---
    // 1. 기존에 예약된 타이머가 있다면 취소하여 중복 실행을 방지합니다.
    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(CaptureTimerHandle);
    }

    // 2. 0.5초 동안 엔진 렌더러가 데이터를 충분히 준비할 수 있도록 매 프레임 캡처를 활성화합니다.
    if (SceneCapture)
    {
        SceneCapture->bCaptureEveryFrame = true;

        // 0.5초 뒤에 캡처를 중단하도록 타이머 설정
        if (GetWorld())
        {
            GetWorldTimerManager().SetTimer(
                CaptureTimerHandle,
                this,
                &ANovaDeckSlot::StopEveryFrameCapture,
                0.5f,
                false
            );
        }
    }
}

void ANovaDeckSlot::StopEveryFrameCapture() const
{
    if (SceneCapture)
    {
        // 0.5초가 지났으므로 매 프레임 캡처를 끄고 정적인 상태로 유지합니다.
        SceneCapture->bCaptureEveryFrame = false;
        
        NOVA_LOG(Log, "Deck Slot %d: 0.5s Warmup Capture Finished.", SlotIndex);
    }
}


void ANovaDeckSlot::ExecuteCapture() const
{
    // SceneCapture 컴포넌트와 결과물이 저장될 RenderTarget이 유효한지 확인합니다.
    if (SceneCapture && RenderTarget)
    {
        // 실제로 렌더 타겟에 현재 설정된 ShowOnlyActors 유닛의 모습을 그립니다.
        SceneCapture->CaptureScene();
    }
}
