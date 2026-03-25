// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby/UI/NovaUnitPartProfileWidget.h"

#include "NovaRevolution.h"
#include "Lobby/UI/NovaUnitPartSpecTableWidget.h"
#include "Components/TextBlock.h"
#include "Core/NovaLog.h"
#include "Core/NovaPart.h"
#include "Lobby/Preview/NovaPartPreviewActor.h" // 프리뷰 액터 헤더 포함
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Lobby/NovaLobbyManager.h"
#include "Lobby/NovaLobbyPlayerController.h"
#include "TimerManager.h"

void UNovaUnitPartProfileWidget::InitCategory(ENovaPartType Category)
{
    if (!PartSpecTable) return;

    CategoryPartIDs.Empty();

    // 1. 데이터 테이블의 모든 행(RowName)을 가져와 현재 카테고리에 맞는 부품만 필터링
    TArray<FName> AllRowNames = PartSpecTable->GetRowNames();
    for (const FName& RowName : AllRowNames)
    {
        FNovaPartSpecRow* Spec = PartSpecTable->FindRow<FNovaPartSpecRow>(RowName, TEXT(""));
        if (Spec && Spec->PartType == Category)
        {
            CategoryPartIDs.Add(RowName);
        }
    }
    // 2. 리스트의 첫 번째 항목으로 초기 인덱스 설정 및 화면 갱신
    CurrentIndex = 0;
    
    // 런타임에 태그를 기반으로 레벨 내 배치된 프리뷰 액터를 찾아 연결합니다.
    // 에디터에서 직접 할당하지 않았더라도 태그가 일치하면 자동으로 바인딩됩니다.
    if (!PreviewActor && !PreviewActorTag.IsNone())
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsWithTag(GetWorld(), PreviewActorTag, FoundActors);

        if (FoundActors.Num() > 0)
        {
            // 첫 번째로 발견된 일치하는 태그의 액터를 프리뷰 액터로 캐스팅하여 할당
            PreviewActor = Cast<ANovaPartPreviewActor>(FoundActors[0]);
        
            if (PreviewActor)
            {
                NOVA_LOG(Log, "Successfully bound PreviewActor via Tag: %s", *PreviewActorTag.ToString());
            }
        }
    }
    
    UpdateDisplay();
}

//버튼 Pressed 이벤트에 연결

void UNovaUnitPartProfileWidget::ShowNextPart()
{
    if (CategoryPartIDs.Num() == 0) return;
    
    // 리스트 끝에 도달하면 다시 처음(0)으로 돌아가는 순환 구조
    CurrentIndex = (CurrentIndex + 1) % CategoryPartIDs.Num();
    UpdateDisplay();
}

void UNovaUnitPartProfileWidget::ShowPrevPart()
{
    if (CategoryPartIDs.Num() == 0) return;
    
    bIsUserOperating = true; // 플래그 ON
    
    // 리스트 처음에 도달하면 다시 마지막으로 돌아가는 순환 구조
    CurrentIndex = (CurrentIndex - 1 + CategoryPartIDs.Num()) % CategoryPartIDs.Num();
    UpdateDisplay();
    
    bIsUserOperating = false; // 플래그 OFF
}

void UNovaUnitPartProfileWidget::SetCurrentPartByClass(TSubclassOf<class ANovaPart> TargetClass)
{
    if (!TargetClass || !PartAssetTable) return;

    // 1. 현재 카테고리 리스트(CategoryPartIDs)를 돌며 해당 클래스를 가진 ID를 찾습니다.
    for (int32 i = 0; i < CategoryPartIDs.Num(); ++i)
    {
        FNovaPartAssetRow* AssetRow = PartAssetTable->FindRow<FNovaPartAssetRow>(CategoryPartIDs[i], TEXT(""));
        if (AssetRow && AssetRow->PartClass == TargetClass)
        {
            // 2. 일치하는 파츠를 찾았다면 인덱스를 갱신하고 화면을 업데이트합니다.
            CurrentIndex = i;
            UpdateDisplay();
            break;
        }
    }
}

void UNovaUnitPartProfileWidget::UpdateDisplay()
{
    if (!CategoryPartIDs.IsValidIndex(CurrentIndex)) return;

    // 1. 현재 인덱스에 해당하는 부품 ID(RowName) 추출
    FName TargetID = CategoryPartIDs[CurrentIndex];

    // 2. 데이터 테이블에서 상세 스펙 데이터 조회
    static const FString ContextString(TEXT("Part Profile Update"));
    FNovaPartSpecRow* SpecData = PartSpecTable->FindRow<FNovaPartSpecRow>(TargetID, ContextString);

    if (SpecData)
    {
        // A. 부품 명칭 텍스트 갱신
        if (Txt_PartName)
        {
            Txt_PartName->SetText(FText::FromString(SpecData->PartName));
        }

        // B. 이미 구현된 '상세 스펙 표 위젯(WBP_PartSpecTable)'에게 데이터를 통째로 전달
        // 이 호출 한 번으로 표 안의 공격력, 체력, 연사력 등 수십 개 항목이 일괄 갱신됩니다.
        if (WBP_PartSpecTable)
        {
            WBP_PartSpecTable->UpdateTable(*SpecData);
        }

        NOVA_LOG(Log, "Profile UI Updated: %s (Index: %d)", *TargetID.ToString(), CurrentIndex);
    }

    // 3. [핵심] 3D 프리뷰 및 매니저 동기화를 다음 프레임(NextTick)으로 예약
    // 파츠 부착 및 연산이 현재 프레임에 완료된 후, 안정적으로 캡처와 동기화가 일어나도록 합니다.
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UNovaUnitPartProfileWidget::Update3DPreview);
    }
}

void UNovaUnitPartProfileWidget::Update3DPreview()
{
    if (!CategoryPartIDs.IsValidIndex(CurrentIndex)) return;
    FName TargetID = CategoryPartIDs[CurrentIndex];

    // 1. 개별 파트 프리뷰 업데이트
    if (PreviewActor && PreviewRenderTarget && PartAssetTable)
    {
        static const FString AssetContext(TEXT("Part Asset Lookup"));
        FNovaPartAssetRow* AssetRow = PartAssetTable->FindRow<FNovaPartAssetRow>(TargetID, AssetContext);

        if (AssetRow && AssetRow->PartClass)
        {
            // 1. 위젯용 단독 파트 프리뷰 갱신
            PreviewActor->UpdatePreview(AssetRow->PartClass, PreviewRenderTarget);
            
            // 2. 다이나믹 머티리얼 생성 및 렌더 타겟 주입
            if (!PreviewDynamicMaterial && PreviewMaterialBase)
            {
                PreviewDynamicMaterial = UMaterialInstanceDynamic::Create(PreviewMaterialBase, this);
            }

            if (PreviewDynamicMaterial)
            {
                // 머티리얼의 텍스처 파라미터(PreviewTexture)에 렌더 타겟 연결
                PreviewDynamicMaterial->SetTextureParameterValue(TEXT("PreviewTexture"), PreviewRenderTarget);

                // 이미지 위젯에 머티리얼 적용
                if (Img_PartPreview)
                {
                    Img_PartPreview->SetBrushFromMaterial(PreviewDynamicMaterial);
                }
            }
            else
            {
                // 렌더 타겟 영상을 위젯의 이미지 컨트롤에 투사
                if (Img_PartPreview)
                {
                    // 렌더 타겟(UTextureRenderTarget2D)을 텍스처로서 브러시 이미지에 할당합니다.
                    // 이를 통해 PreviewActor가 찍고 있는 3D 화면이 UI의 해당 영역에 실시간으로 출력됩니다.
                    Img_PartPreview->SetBrushResourceObject(PreviewRenderTarget);
                }
            }
            
            NOVA_LOG(Log, "3D Preview Updated for Part: %s", *TargetID.ToString());
        }
    }
    
    // 2. 중앙의 AssemblyPreviewUnit 및 로비 매니저와 실시간 동기화
    if (ANovaLobbyPlayerController* LobbyPC = Cast<ANovaLobbyPlayerController>(GetOwningPlayer()))
    {
        if (ANovaLobbyManager* Manager = LobbyPC->GetLobbyManager())
        {
            // 이 함수 호출을 통해 중앙 유닛의 외형이 최종적으로 교체됩니다.
            Manager->SelectPart(DefaultCategory, TargetID);
            NOVA_LOG(Log, "Manager Sync Completed for Part: %s", *TargetID.ToString());
        }
    }
}

void UNovaUnitPartProfileWidget::OnManagerDataChanged(int32 SlotIndex, const FString& UnitName,
    const FNovaUnitAssemblyData& AssemblyData)
{
    // [중요] 사용자가 직접 UI 화살표를 눌러서 발생한 신호라면, 이미 인덱스가 맞춰져 있으므로 무시합니다. (무한 루프 방지)
    if (bIsUserOperating) return;

    // 1. 내 위젯의 카테고리에 해당하는 파츠 클래스 추출
    TSubclassOf<class ANovaPart> TargetClass = nullptr;
    switch (DefaultCategory)
    {
    case ENovaPartType::Legs:
        TargetClass = AssemblyData.LegsClass;
        break;
    case ENovaPartType::Body:
        TargetClass = AssemblyData.BodyClass;
        break;
    case ENovaPartType::Weapon:
        TargetClass = AssemblyData.WeaponClass;
        break;
    }

    if (!TargetClass) return;

    // 2. 현재 내 리스트(CategoryPartIDs)에서 해당 클래스를 가진 인덱스 찾기
    for (int32 i = 0; i < CategoryPartIDs.Num(); ++i)
    {
        FNovaPartAssetRow* Asset = PartAssetTable->FindRow<FNovaPartAssetRow>(CategoryPartIDs[i], TEXT(""));
        if (Asset && Asset->PartClass == TargetClass)
        {
            // 3. 일치하는 파츠를 찾았다면 인덱스를 강제 세팅하고 화면만 업데이트
            if (CurrentIndex != i)
            {
                CurrentIndex = i;
                UpdateDisplay(); // 이때 UpdateDisplay 내의 Manager->SelectPart 호출 로직에도 bIsUserOperating 체크가 필요할 수 있음
            }
            break;
        }
    }
    
}

void UNovaUnitPartProfileWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    // 에디터 디자인 화면에서도 특정 카테고리(예: Legs)를 미리 불러와서
    // 데이터 테이블의 수치가 UI 표에 정상적으로 들어오는지 즉시 확인할 수 있습니다.
    if (!PartSpecTable) return;
    
    InitCategory(DefaultCategory);
    
    // 1. 매니저를 찾아 델리게이트 구독 (ControlWidget과 동일한 패턴)
    if (ANovaLobbyPlayerController* PC = Cast<ANovaLobbyPlayerController>(GetOwningPlayer()))
    {
        if (ANovaLobbyManager* Manager = PC->GetLobbyManager())
        {
            Manager->OnAssemblyDataChanged.AddDynamic(this, &UNovaUnitPartProfileWidget::OnManagerDataChanged);

            // 초기 동기화 호출
            OnManagerDataChanged(Manager->GetSelectedSlotIndex(), Manager->GetPendingData().UnitName, Manager->GetPendingData());
        }
    }
}

