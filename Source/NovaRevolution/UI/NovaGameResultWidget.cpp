// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/NovaGameResultWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UNovaGameResultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (LobbyBtn)
	{
		LobbyBtn->OnClicked.AddDynamic(this, &UNovaGameResultWidget::OnQuitToLobbyClicked);
	}
}

void UNovaGameResultWidget::SetGameResult(bool bInIsWinner)
{
	this->bIsWinner = bInIsWinner;
	// 블루프린트 이벤트 호출 (애니메이션, 텍스트 변경 등 디자인 처리)
	OnResultApplied();
}

void UNovaGameResultWidget::OnResultApplied()
{
	if (!ResultText) return;

	if (bIsWinner)
	{
		// 승리
		ResultText->SetText(FText::FromString(TEXT("VICTORY!")));
		ResultText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
	else
	{
		// 패배
		ResultText->SetText(FText::FromString(TEXT("DEFEAT")));
		ResultText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
}

void UNovaGameResultWidget::OnQuitToLobbyClicked()
{
	// 게임 일시정지가 되어 있다면 해제 후 이동 (필요 시)
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	// 로비 레벨로 이동
	// UGameplayStatics::OpenLevel(this, LobbyLevelName);
	
	// 동기 로드 및 레벨 전환 수행
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, LobbyLevel);
}

/*
void UNovaGameResultWidget::OnRestartClicked()
{
	// 게임 일시정지가 되어 있다면 해제 후 이동
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	// 현재 레벨 이름 가져오기
	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this);

	// 현재 레벨 다시 시작
	UGameplayStatics::OpenLevel(this, FName(*CurrentLevelName));
}
*/