// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Core/NovaPartData.h"
#include "NovaUnitPartProfileWidget.generated.h"

class ANovaPartPreviewActor;
class UTextureRenderTarget2D;

/**
 * UNovaUnitPartProfileWidget
 * 부품의 공식 명칭과 상세 스펙 표를 데이터 테이블과 연결하여 동적으로 보여주는 위젯입니다.
 */
UCLASS()
class NOVAREVOLUTION_API UNovaUnitPartProfileWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 특정 카테고리(다리/몸통/무기)의 부품들만 필터링하여 리스트업합니다.
	 * (WBP의 Construct나 버튼 이벤트에서 호출)
	 */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby|UI")
	void InitCategory(ENovaPartType Category);

	/** 화살표 버튼 이벤트 (UI 버튼에 바인딩) */
	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby|UI")
	void ShowNextPart();

	UFUNCTION(BlueprintCallable, Category = "Nova|Lobby|UI")
	void ShowPrevPart();
	
	//플레이 전 에디터에서 변경사항 확인 가능
	virtual void NativePreConstruct() override;
	
	/** 위젯이 처음 생성되거나 초기화될 때 기본으로 표시할 부품 카테고리 (에디터에서 설정 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Preview")
	ENovaPartType DefaultCategory = ENovaPartType::Legs;

protected:
	/** 현재 인덱스의 데이터를 읽어와 이름과 표 위젯을 실시간으로 업데이트합니다. */
	void UpdateDisplay();

	// --- PartSpec 데이터 테이블 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Data")
	TObjectPtr<UDataTable> PartSpecTable;
	
	// --- 부품 ID로 클래스(BP)를 찾기 위한 PartAsset 데이터 테이블 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Data")
	TObjectPtr<UDataTable> PartAssetTable;

	// --- [추가] 프리뷰 설정 ---
	/** 맵에 배치된 PreviewActor 인스턴스 (실제 촬영을 담당) */
	
	// 레벨에 배치된 액터의 '태그' 이름으로 파트별 PartPreviewActor 인스턴스 찾기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Preview")
	FName PreviewActorTag;
	
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Preview")
	TObjectPtr<ANovaPartPreviewActor> PreviewActor;

	// 해당 위젯 전용 렌더 타겟 (이미지에 바인딩되어 실시간 화면을 출력)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nova|Preview")
	TObjectPtr<UTextureRenderTarget2D> PreviewRenderTarget;

	// --- 에디터 WBP 위젯 바인딩 (이름 일치 필수) ---
	/** 부품 공식 명칭 표시 (로드런너, 건봇 등) */
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_PartName;
	
	/** [추가] 부품 외형 프리뷰를 출력할 이미지 컨트롤 (WBP의 이름과 일치해야 함) */
	UPROPERTY(meta = (BindWidget))
	class UImage* Img_PartPreview;

	/** 상세 능력치 표 위젯 (이미 구현된 TableWidget 인스턴스) */
	UPROPERTY(meta = (BindWidget))
	class UNovaUnitPartSpecTableWidget* WBP_PartSpecTable;

	// --- 내부 상태 관리 ---
	/** 현재 카테고리에 속한 부품 ID 리스트 (순회용) */
	TArray<FName> CategoryPartIDs; 
    
	int32 CurrentIndex = 0;
	
	
};