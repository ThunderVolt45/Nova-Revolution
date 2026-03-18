// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/NovaMinimapWidget.h"

#include "Core/NovaBase.h"
#include "Core/NovaFogManager.h"
#include "Core/NovaInterfaces.h"
#include "Core/NovaPlayerState.h"
#include "Core/NovaUnit.h"
#include "Kismet/GameplayStatics.h"
#include "Player/NovaPawn.h"
#include "Player/NovaPlayerController.h"
#include "Core/NovaMapManager.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Runtime/SlateCore/Public/Rendering/DrawElements.h"

void UNovaMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 월드에서 MapManager 찾기
	MapManager = Cast<ANovaMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ANovaMapManager::StaticClass()));
	// 월드에서 FogManager 찾기
	FogManager = Cast<ANovaFogManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ANovaFogManager::StaticClass()));

	// PlayerController 참조
	NovaPC = Cast<ANovaPlayerController>(GetOwningPlayer());

	// 다이나믹 머터리얼 생성 및 안개 RT 연결
	if (MinimapMaterial && FogManager)
	{
		MinimapMaterialInst = UMaterialInstanceDynamic::Create(MinimapMaterial, this);
		// M_Minimap에서 설정한 파라미터 이름 'FogTexture'에 CurrentFogRT를 전달
		MinimapMaterialInst->SetTextureParameterValue(FName("FogTexture"), FogManager->GetCurrentFogRT());
	}

	if (MinimapMaterial && MapManager)
	{
		MinimapMaterialInst = UMaterialInstanceDynamic::Create(MinimapMaterial, this);

		// 1. 안개 텍스처 연결
		if (FogManager)
			MinimapMaterialInst->SetTextureParameterValue(FName("FogTexture"), FogManager->GetCurrentFogRT());

		// 2. 캡처된 배경 지형 텍스처 연결 (머터리얼 파라미터 이름: 'TerrainTexture')
		if (UTextureRenderTarget2D* BackgroundRT = MapManager->GetMinimapBackgroundRT())
		{
			MinimapMaterialInst->SetTextureParameterValue(FName("TerrainTexture"), BackgroundRT);
		}
	}
}

void UNovaMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	// Super::NativeTick(MyGeometry, InDeltaTime);
	// --- 마우스 드래그로 미니맵 카메라 움직이기 ---

	// 드래그 중일 때 매 프레임 마우스 위치로 카메라 이동
	if (bIsDragging)
	{
		// 현재 마우스의 스크린 좌표 가져오기
		FVector2D MouseScreenPos;
		if (GetOwningPlayer()->GetMousePosition(MouseScreenPos.X, MouseScreenPos.Y))
		{
			HandleMinimapClick(MyGeometry, MouseScreenPos);
		}
	}
}

int32 UNovaMinimapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                      const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                      int32 LayerId,
                                      const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// 1. 배경(지형 + 안개) 그리기
	if (MinimapMaterialInst)
	{
		// 실제로는 머터리얼을 입힌 박스를 그려야 하므로 SlateBrush를 머터리얼로 설정하여 그립니다.
		FSlateBrush MinimapBrush;
		MinimapBrush.SetResourceObject(MinimapMaterialInst);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), &MinimapBrush);
	}

	// 매니저들이 없으면 중단
	if (!FogManager || !MapManager || !NovaPC) return LayerId;

	ANovaPlayerState* PS = NovaPC->GetPlayerState<ANovaPlayerState>();
	if (!PS) return LayerId;

	FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
	int32 UnitLayer = LayerId + 1;

	// 2. 등록된 액터들만 순회
	const TArray<TWeakObjectPtr<AActor>>& RegisteredActors = MapManager->GetRegisteredActors();

	for (const TWeakObjectPtr<AActor>& WeakActor : RegisteredActors)
	{
		AActor* Actor = WeakActor.Get();

		// 유닛의 가시성 확인 (안개 속에 있으면 안 그림 - 적군인 경우)
		// 아군은 항상 그리고, 적군은 FogManager가 설정한 Visibility를 따름
		bool bIsFriendly = false;
		if (INovaTeamInterface* TeamActor = Cast<INovaTeamInterface>(Actor))
		{
			bIsFriendly = (TeamActor->GetTeamID() == NovaPC->GetPlayerState<ANovaPlayerState>()->GetTeamID());
		}

		// 가시성 체크 (NovaUnit에 정의된 bIsVisible 활용)
		bool bVisibleOnMinimap = bIsFriendly;
		if (!bIsFriendly)
		{
			if (ANovaUnit* Unit = Cast<ANovaUnit>(Actor))
			{
				bVisibleOnMinimap = Unit->GetFogVisibility();
			}
			if (ANovaBase* Base = Cast<ANovaBase>(Actor))
			{
				bVisibleOnMinimap = Base->GetFogVisibility();
			}
		}

		if (bVisibleOnMinimap)
		{
			FVector2D UV = MapManager->WorldToMapUV(Actor->GetActorLocation());

			// --- [축 교환 로직] ---
			// UV.X (월드 X: 앞/뒤)를 위젯의 Y(상/하)로
			// UV.Y (월드 Y: 좌/우)를 위젯의 X(좌/우)로
			FVector2D PaintPos;
			PaintPos.X = UV.Y * WidgetSize.X; // 월드 Y가 가로축
			PaintPos.Y = (1.0f - UV.X) * WidgetSize.Y; // 월드 X가 세로축 (반전 적용)

			FLinearColor IconColor =
				bIsFriendly ? FLinearColor(0.f, 1.f, 0.f, 1.0f) : FLinearColor(1.f, 0.f, 0.f, 1.0f);

			// 점(아이콘) 그리기
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				UnitLayer,
				AllottedGeometry.ToPaintGeometry(FVector2D(UnitIconSize), FSlateLayoutTransform(
					                                 PaintPos - (UnitIconSize * 0.5f))),
				&UnitIconBrush,
				ESlateDrawEffect::None,
				IconColor
			);
		}
	}

	// 3. 카메라 영역(가이드 사각형) 그리기
	if (ANovaPawn* Pawn = Cast<ANovaPawn>(NovaPC->GetPawn()))
	{
		if (MapManager)
		{
			FVector2D CameraUV = MapManager->WorldToMapUV(Pawn->GetActorLocation());
			// --- [축 교환 로직 적용] ---
			FVector2D CameraPos;
			CameraPos.X = CameraUV.Y * WidgetSize.X; // 월드 Y -> 위젯 X
			CameraPos.Y = (1.0f - CameraUV.X) * WidgetSize.Y; // 월드 X -> 위젯 Y (반전)

			// 대략적인 카메라 시야 사각형 크기 (줌에 따라 조절 가능)
			FVector2D RectSize = FVector2D(WidgetSize.X * 0.15f, WidgetSize.Y * 0.1f);

			TArray<FVector2D> Points;
			Points.Add(CameraPos + FVector2D(-RectSize.X, -RectSize.Y));
			Points.Add(CameraPos + FVector2D(RectSize.X, -RectSize.Y));
			Points.Add(CameraPos + FVector2D(RectSize.X, RectSize.Y));
			Points.Add(CameraPos + FVector2D(-RectSize.X, RectSize.Y));
			Points.Add(CameraPos + FVector2D(-RectSize.X, -RectSize.Y));

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				UnitLayer + 1,
				AllottedGeometry.ToPaintGeometry(),
				Points,
				ESlateDrawEffect::None,
				FLinearColor::White,
				true,
				CameraLineThickness
			);
		}
	}

	return UnitLayer + 1;
}

FReply UNovaMinimapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = true;
		HandleMinimapClick(InGeometry, InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return FReply::Unhandled();
}

FReply UNovaMinimapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply UNovaMinimapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UNovaMinimapWidget::HandleMinimapClick(const FGeometry& MyGeometry, const FVector2D& MouseScreenPos)
{
	// 스크린 좌표를 위젯 로컬 좌표로 변환
	FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseScreenPos);
	FVector2D WidgetSize = MyGeometry.GetLocalSize();

	// --- [축 역산 로직] ---
	// 위젯 X (가로) -> 월드 Y (V)
	// 위젯 Y (세로) -> 월드 X (U)
	float TargetV = FMath::Clamp(LocalPos.X / WidgetSize.X, 0.0f, 1.0f);
	float TargetU = 1.0f - FMath::Clamp(LocalPos.Y / WidgetSize.Y, 0.0f, 1.0f); // 세로 반전 복구

	// 월드 좌표로 변환하여 카메라 이동
	FVector TargetWorldPos = MapManager->UVToWorldLocation(FVector2D(TargetU, TargetV), 0.0f);
	if (APawn* Pawn = NovaPC->GetPawn())
	{
		// Z값은 현재 카메라 높이 유지
		TargetWorldPos.Z = Pawn->GetActorLocation().Z;
		Pawn->SetActorLocation(TargetWorldPos);
	}
}

FVector UNovaMinimapWidget::MinimapUVToWorld(const FVector2D& UV) const
{
	if (MapManager)
	{
		// MapManager의 공용 역산 함수 사용 (Z높이는 0으로 설정)
		return MapManager->UVToWorldLocation(UV, 0.0f);
	}

	return FVector::ZeroVector;
}
