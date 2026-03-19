// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby/Preview/NovaPartPreviewActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Core/NovaPart.h"
#include "Core/NovaObjectPoolSubsystem.h"

ANovaPartPreviewActor::ANovaPartPreviewActor()
{
    // 1. 실시간 회전을 위해 Tick 활성화
    PrimaryActorTick.bCanEverTick = true;

    PreviewRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewRoot"));
    RootComponent = PreviewRoot;

    CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComponent"));
    CaptureComponent->SetupAttachment(PreviewRoot);

    // 카메라 위치 및 각도 설정 (Nova 1492의 쿼터뷰 감성 재현)
    CaptureComponent->SetRelativeLocation(FVector(200.0f, 0.0f, 40.0f));
    //CaptureComponent->SetRelativeRotation(FRotator(-15.0f, 180.0f, 0.0f));

    // 배경을 제외하고 부품만 찍기 위한 설정
    CaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

    // 2. [변경] 부품이 회전하는 모습을 보여주기 위해 매 프레임 촬영 활성화
    CaptureComponent->bCaptureEveryFrame = true;
    CaptureComponent->bCaptureOnMovement = true;
}

void ANovaPartPreviewActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 3. 현재 프리뷰 중인 부품이 있다면 설정된 속도(RotationSpeed)로 회전
    if (CurrentPreviewPart)
    {
        // Y축(Yaw)을 기준으로 회전시켜 360도 외형을 모두 노출
        CurrentPreviewPart->AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));
    }
}

void ANovaPartPreviewActor::UpdatePreview(TSubclassOf<ANovaPart> PartClass, UTextureRenderTarget2D* TargetTexture)
{
    UNovaObjectPoolSubsystem* Pool = GetWorld()->GetSubsystem<UNovaObjectPoolSubsystem>();
    if (!Pool || !PartClass || !TargetTexture) return;

    // 기존 부품이 있다면 풀(Pool)로 반환하여 재사용
    if (CurrentPreviewPart)
    {
        Pool->ReturnToPool(CurrentPreviewPart);
        CurrentPreviewPart = nullptr;
    }

    // 촬영 리스트 초기화 및 새로운 렌더 타겟 연결
    CaptureComponent->ShowOnlyActors.Reset();
    CaptureComponent->TextureTarget = TargetTexture;

    // 오브젝트 풀에서 새 부품 스폰
    CurrentPreviewPart = Cast<ANovaPart>(Pool->SpawnFromPool(PartClass, GetActorTransform()));

    if (CurrentPreviewPart)
    {
        CurrentPreviewPart->AttachToComponent(PreviewRoot, FAttachmentTransformRules::SnapToTargetIncludingScale);

        // 4. 부품 교체 시점에 회전값을 초기화하여 정면부터 보여주기 시작
        CurrentPreviewPart->SetActorRelativeRotation(FRotator::ZeroRotator);

        // 카메라 촬영 리스트에 부품 및 부속 액터 등록
        CaptureComponent->ShowOnlyActors.Add(CurrentPreviewPart);

        TArray<AActor*> AttachedActors;
        CurrentPreviewPart->GetAttachedActors(AttachedActors);
        for (AActor* Child : AttachedActors)
        {
            CaptureComponent->ShowOnlyActors.Add(Child);
        }
    }
}

