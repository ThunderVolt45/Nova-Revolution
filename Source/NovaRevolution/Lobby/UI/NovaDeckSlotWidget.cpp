// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/UI/NovaDeckSlotWidget.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Lobby/NovaLobbyManager.h"
#include "Lobby/NovaLobbyPlayerController.h"

void UNovaDeckSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 버튼 클릭 이벤트 바인딩
	if (SlotButton)
	{
		SlotButton->OnClicked.AddDynamic(this, &UNovaDeckSlotWidget::OnSlotButtonClicked);
	}
}

void UNovaDeckSlotWidget::InitSlot(int32 InSlotIndex, UTextureRenderTarget2D* RenderTarget)
{
	SlotIndex = InSlotIndex;
	
	// 텍스트 설정: "BU " + (인덱스 + 1)
	if (SlotText)
	{
		// FString::Printf를 사용하여 포맷팅된 문자열 생성
		// 인덱스가 0부터 시작하므로 사람이 보기 편하게 +1을 해줍니다.
		FString DisplayName = FString::Printf(TEXT("BU %d"), SlotIndex + 1);

		// UTextBlock에 텍스트 적용 (FText로 변환 필요)
		SlotText->SetText(FText::FromString(DisplayName));
	}

	if (SlotImage && RenderTarget)
	{
		// 1. 다이나믹 머티리얼이 아직 없으면 베이스 머티리얼을 바탕으로 생성합니다.
		if (!SlotDynamicMaterial && SlotMaterialBase)
		{
			SlotDynamicMaterial = UMaterialInstanceDynamic::Create(SlotMaterialBase, this);
		}

		if (SlotDynamicMaterial)
		{
			// 2. 머티리얼 내부의 'PreviewTexture' 파라미터에 렌더 타겟을 연결합니다.
			SlotDynamicMaterial->SetTextureParameterValue(TEXT("PreviewTexture"), RenderTarget);

			// 3. 이미지 위젯의 브러시를 이 다이나믹 머티리얼로 설정합니다.
			SlotImage->SetBrushFromMaterial(SlotDynamicMaterial);
		}
		else
		{
			// 머티리얼이 설정되지 않은 경우를 대비한 폴백(Fallback) 처리
			SlotImage->SetBrushResourceObject(RenderTarget);
		}
	}
}

void UNovaDeckSlotWidget::OnSlotButtonClicked()
{
	// 플레이어 컨트롤러를 통해 매니저를 찾고, 3D 슬롯을 클릭했을 때와 동일하게 동작시킵니다.
	if (ANovaLobbyPlayerController* PC = Cast<ANovaLobbyPlayerController>(GetOwningPlayer()))
	{
		if (ANovaLobbyManager* Manager = PC->GetLobbyManager())
		{
			Manager->SelectDeckSlot(SlotIndex);
		}
	}
}