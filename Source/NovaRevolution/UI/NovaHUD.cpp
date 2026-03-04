// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/NovaHUD.h"

ANovaHUD::ANovaHUD()
{
}

void ANovaHUD::DrawHUD()
{
	Super::DrawHUD();

	if (bIsDragging)
	{
		// 박스 내부 채우기
		DrawRect(BoxFillColor, StartPosition.X, StartPosition.Y, EndPosition.X - StartPosition.X, EndPosition.Y -
		         StartPosition.Y);

		// 박스 내부
		DrawLine(StartPosition.X, StartPosition.Y, EndPosition.X, StartPosition.Y, BoxOutlineColor,
		         BoxOutlineThickness); // 상단
		DrawLine(StartPosition.X, StartPosition.Y, StartPosition.X, EndPosition.Y, BoxOutlineColor,
		         BoxOutlineThickness); // 좌측
		DrawLine(EndPosition.X, StartPosition.Y, EndPosition.X, EndPosition.Y, BoxOutlineColor,
		         BoxOutlineThickness); // 우측
		DrawLine(StartPosition.X, EndPosition.Y, EndPosition.X, EndPosition.Y, BoxOutlineColor,
		         BoxOutlineThickness); // 하단
	}
}

void ANovaHUD::UpdateDragRect(FVector2D Start, FVector2D End, bool bInIsDragging)
{
	StartPosition = Start;
	EndPosition = End;
	bIsDragging = bInIsDragging;
}
