// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/NovaAssemblyTypes.h"
#include "NovaBaseButtonWidget.generated.h"

/** 기지 버튼의 동작 타입 */
UENUM(BlueprintType)
enum class EBaseActionType : uint8
{
	ProduceUnit, // 유닛 생산
	SetRallyPoint, // 랠리 포인트 설정 모드 진입
};

/**
 * 기지 명령(유닛 생산, 랠리포인트 지정)을 수행하는 버튼 위젯
 */
UCLASS()
class NOVAREVOLUTION_API UNovaBaseButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 버튼의 동작 타입 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|Base")
	EBaseActionType ActionType = EBaseActionType::ProduceUnit;

	/** 유닛 생산 시 사용할 슬롯 인덱스 (0~9) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nova|Base",
		meta = (EditCondition = "ActionType == EBaseActionType::ProduceUnit"))
	int32 ProductionSlotIndex = 0;

	/** 버튼 아이콘 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nova|UI")
	TObjectPtr<UTexture2D> ButtonIcon;

protected:
	/** UI 컴포넌트 바인딩 */
	UPROPERTY(meta = (BindWidget))
	class UImage* ButtonImage;

	UPROPERTY(meta = (BindWidget))
	class UButton* ActionButton;

	/** 게임 시작 시 캐싱해둘 유닛 데이터 */
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Base")
	FNovaUnitAssemblyData CachedUnitData;

	/** 데이터 캐싱 성공 여부 */
	bool bIsDataCached = false;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	/** 버튼 클릭 시 실행 */
	UFUNCTION()
	void OnButtonClicked();

	/** 선택 변경 시 가시성 제어 */
	UFUNCTION()
	void HandleSelectionChanged(const TArray<AActor*>& SelectedUnits);

	/** 덱 정보를 한 번만 가져와 캐싱하는 함수 */
	void TryCacheDeckData();

	/** 캐싱된 데이터를 바탕으로 UI 업데이트 */
	void UpdateUIFromCache();

	/** 기지가 선택되었는지 확인하는 헬퍼 함수 */
	bool IsBaseSelected(const TArray<AActor*>& SelectedUnits) const;
};
