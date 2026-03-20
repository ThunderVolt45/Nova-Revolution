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
    
    // 리스트 처음에 도달하면 다시 마지막으로 돌아가는 순환 구조
    CurrentIndex = (CurrentIndex - 1 + CategoryPartIDs.Num()) % CategoryPartIDs.Num();
    UpdateDisplay();
}

void UNovaUnitPartProfileWidget::UpdateDisplay()
{
    if (!CategoryPartIDs.IsValidIndex(CurrentIndex)) return;

    // 1. 현재 인덱스에 해당하는 부품 ID(RowName) 추출
    FName TargetID = CategoryPartIDs[CurrentIndex];

    // 2. 데이터 테이블에서 해당 행(Row)의 상세 스펙 데이터 조회
    static const FString ContextString(TEXT("Part Profile Update"));
    FNovaPartSpecRow* SpecData = PartSpecTable->FindRow<FNovaPartSpecRow>(TargetID, ContextString);

    if (SpecData)
    {
        // A. 부품 명칭 텍스트 갱신 (예: 로드런너, 건봇 등)
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
    
    // 3. 파트 외형 프리뷰 업데이트
    // 프리뷰 액터, 렌더 타겟, 에셋 테이블이 모두 유효할 때만 진행합니다.
    if (PreviewActor && PreviewRenderTarget && PartAssetTable)
    {
        // AssetTable에서 해당 부품의 실제 Blueprint 클래스(PartClass) 정보를 가져옴
        static const FString AssetContext(TEXT("Part Asset Lookup"));
        FNovaPartAssetRow* AssetRow = PartAssetTable->FindRow<FNovaPartAssetRow>(TargetID, AssetContext);

        if (AssetRow && AssetRow->PartClass)
        {
            // 1. 위젯 내부의 개별 파트 프리뷰 업데이트
            PreviewActor->UpdatePreview(AssetRow->PartClass, PreviewRenderTarget);
            
            // 렌더 타겟 영상을 위젯의 이미지 컨트롤에 투사
            if (Img_PartPreview)
            {
                // 렌더 타겟(UTextureRenderTarget2D)을 텍스처로서 브러시 이미지에 할당합니다.
                // 이를 통해 PreviewActor가 찍고 있는 3D 화면이 UI의 해당 영역에 실시간으로 출력됩니다.
                Img_PartPreview->SetBrushResourceObject(PreviewRenderTarget);
            }
            
            NOVA_LOG(Log, "Preview Updated for Part: %s", *TargetID.ToString());
            
            //2. 중앙의 AssemblyPreviewUnit과 실시간 동기화, 위젯이 촬영을 위해 꺼낸 클래스 정보를 그대로 매니저에게 밀어넣습니다.
            if (ANovaLobbyPlayerController* LobbyPC = Cast<ANovaLobbyPlayerController>(GetOwningPlayer()))
            {
                if (ANovaLobbyManager* Manager = LobbyPC->GetLobbyManager())
                {
                    // 이 함수 호출이 실행되는 즉시, 중앙 유닛도 해당 부품으로 갈아 끼웁니다.
                    // 위젯의 카테고리(DefaultCategory)와 선택된 파츠 ID(TargetID)를 매니저에게 전송합니다.
                    Manager->SelectPart(DefaultCategory, TargetID);
                }
            }
            NOVA_LOG(Log, "Syncing Part with Assembly: %s", *TargetID.ToString());
            
        }
    }
    
    // 1. 소유 중인 플레이어 컨트롤러를 로비 전용 컨트롤러로 캐스팅하여 가져옵니다.
    if (ANovaLobbyPlayerController* LobbyPC = Cast<ANovaLobbyPlayerController>(GetOwningPlayer()))
    {
        // 2. 컨트롤러가 보유한 로비 매니저 참조를 획득합니다.
        if (ANovaLobbyManager* Manager = LobbyPC->GetLobbyManager())
        {
            // 3. 현재 위젯이 담당하는 카테고리(DefaultCategory)와 현재 선택된 파츠 ID(TargetID)를 매니저에게 전달합니다.
            // 이 함수 호출을 통해 매니저는 덱 데이터를 갱신하고, 월드에 배치된 '전신 유닛'의 외형을 즉시 교체합니다.
            Manager->SelectPart(DefaultCategory, TargetID);
        
            // 이 시점에서 '개별 부품 프리뷰'와 '전신 조립 모습'이 동시에 업데이트되는 시각적 동기화가 이루어집니다.
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
}

