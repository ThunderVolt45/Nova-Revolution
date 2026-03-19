// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Core/NovaTypes.h"
#include "GameFramework/PlayerController.h"
#include "NovaPlayerController.generated.h"

// 인 게임 내 유닛 하단의 체력바용
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShowHealthBarsChanged, bool, bShow);

// 유닛 선택 시 정보를 표시할 UI용
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectionChanged, const TArray<AActor*>&, SelectedUnits);

struct FInputActionValue;
class UNovaInputConfig;
class UInputMappingContext;
class UInputAction;
class UUserWidget;
class UNiagaraSystem;

/**
 * 부대 지정의 대상들을 담아둘 구조체
 */
USTRUCT(BlueprintType)
struct FNovaControlGroup
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<AActor>> Targets;

	// 자동 부대 지정 편입을 조절하기 위한 bool 변수 (유저가 수동으로 부대 지정 시 false)
	UPROPERTY(BlueprintReadOnly)
	bool bIsAutoAssignActive = true;
};

/**
 * ANovaPlayerController
 * 플레이어의 입력을 받아 유닛을 선택하고 명령을 내리는 전술 지휘관 클래스
 */
UCLASS()
class NOVAREVOLUTION_API ANovaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANovaPlayerController();

protected:
	virtual void BeginPlay() override;

	// 커스텀 입력 컴포넌트를 사용하기 위해 오버라이드 합니다.
	virtual void SetupInputComponent() override;

	// 매 프레임 마우스 엣지 체크를 위해 오버라이드
	virtual void PlayerTick(float DeltaTime) override;

	// 현재 드래그 상태인지 여부
	bool bIsDraggingBox = false;

	// Shift 모드 여부 (선택 추가/제거, 즉시 생산 명령)
	bool bIsShiftDown = false;

	// Ctrl 모드 여부 (부대 지정)
	bool bIsCtrlDown = false;

	// Alt 모드 여부 (스킬 즉시 사용 명령)
	bool bIsAltDown = false;

	// 현재 체력바 표시 여부 (기본값 : true)
	bool bShowHealthBars = true;

	// 엣지 스크롤링 활성화 여부
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Input")
	bool bEnableEdgeScrolling = true;

	// 화면 끝에서 몇 픽셀 이내일 때 화면을 이동시킬지 설정
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Input")
	float EdgeScrollMargin = 15.f;

	// 에디터에서 할당할 IMC
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Input")
	TObjectPtr<UInputMappingContext> IMC;

	// 에디터에서 할당할 입력 설정 데이터 에셋
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Input")
	TObjectPtr<UNovaInputConfig> InputConfig;

	// 에디터에서 할당할 카메라 이동 입력 액션
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Input")
	TObjectPtr<UInputAction> MoveCameraAction;

	// 에디터에서 할당할 카메라 줌 액션
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Input")
	TObjectPtr<UInputAction> ZoomAction;

	// 현재 마우스로 선택한 액터들을 담아두는 배열
	UPROPERTY(BlueprintReadOnly, Category="Nova|Selection")
	TArray<TObjectPtr<AActor>> SelectedActors;

	// 부대 지정을 위한 구조체 배열 (TArray<TArray<>> 형태를 사용할 수 없어 구조체로 선언)
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Selection")
	TArray<FNovaControlGroup> ControlGroups;

	// 현재 대기 중인 명령 타입 (None이 아니면 다음 좌클릭 시 해당 명령 수행)
	UPROPERTY(BlueprintReadOnly, Category = "Nova|Command")
	ECommandType PendingCommandType = ECommandType::None;

	// 드래그 시작 시점의 화면 좌표 (픽셀 단위)
	FVector2D DragStartPos;

	// UNovaInputComponent에서 태그를 매개변수로 넘겨주게 됩니다.
	// 마우스 Pressed
	void Input_AbilityInputTagPressed(FGameplayTag InputTag);

	// 마우스 Released
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);

	// 마우스 Held
	void Input_AbilityInputTagHeld(FGameplayTag InputTag);

	// 실제 카메라 이동 처리 함수 -> 마우스 스크롤링 추가되어 공용 함수 ApplyCameraMovement이용
	void Input_MoveCamera(const FInputActionValue& Value);

	// 공통 카메라 이동처리 함수 (키보드와 마우스 엣지에서 공용으로 사용)
	void ApplyCameraMovement(float ForwardInput, float RightInput);

	// 카메라 줌 처리 함수
	void Input_Zoom(const FInputActionValue& Value);

	// 드래그 선택을 수행하는 실제 판정 함수
	void PerformBoxSelection();

	// 선택된 유닛들에게 실제 명령을 하달하는 헬퍼 함수
	void IssueCommandToSelectedUnits(const FCommandData& CommandData);

	// 명령 취소 (우클릭 시 대기 중인 명령 해제 - ESC로 취소 추가 예정)
	void CancelPendingCommand();

	/**
	 * 유닛 선택 및 카메라 포커싱 공통 로직(대상이 둘 이상일 경우 대상들의 중심으로 카메라 이동)
	 * TagetActors : 선택할 액터 배열
	 * FocusID : 호출 대상을 구분할 고유 ID (1 ~ 0 : 부대, 10 : 기지)
	 */
	void HandleFocusAndSelection(const TArray<AActor*>& TargetActors, int32 FocusID);

	// 체력바 토글 입력 처리 함수
	void ToggleHealthBar(FGameplayTag InputTag);

private:
	// 마우스 커서 아래에 무엇이 있는지 감지하는 헬퍼 함수
	void GetCursorHitResult(FHitResult& OutHitResult);

	// 헬퍼 함수 : 선택 비우기
	void ClearSelection();

	// 연타 판정을 위한 상태 변수
	int32 LastFocusID = -1;
	float LastFocusTime = 0.f;

	// 연타 인정 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Nova|Input")
	float DoubleClickThreshold = 0.3f;

public:
	// 생성된 유닛 자동 부대 편입
	void OnUnitProduced(AActor* Unit, int32 SlotIndex);

	// 대상이 죽거나(파괴되거나) 안개 속으로 숨겨졌을 때 호출되는 알림 함수
	void NotifyTargetUnselectable(AActor* SelectedTargets);

	// 델리게이트 인스턴스
	UPROPERTY(BlueprintAssignable, Category = "Nova|UI")
	FOnShowHealthBarsChanged OnShowHealthBarsChanged;

	// 현재 체력바 표시 옵션 Getter(유닛/기지 초기화용)
	UFUNCTION(BlueprintPure, Category = "Nova|UI")
	bool GetShowHealthBars() const { return bShowHealthBars; }
	
	// 추가) GA에서 사용하기 위한 PendingCommandType Getter/Setter 함수
	// 명령 대기 상태를 설정합니다. (GA에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Nova|Command")
	void SetPendingCommandType(ECommandType NewType) { PendingCommandType = NewType; }

	// 현재 명령 대기 상태를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Nova|Command")
	ECommandType GetPendingCommandType() const { return PendingCommandType; }
	
protected:
	/** 화면에 띄울 메인 HUD 위젯 클래스 (블루프린트에서 설정) */
	UPROPERTY(EditDefaultsOnly, Category = "Nova|UI")
	TSubclassOf<UUserWidget> MainHUDClass;

	/** 생성된 HUD 인스턴스 저장용 */
	UPROPERTY()
	UUserWidget* MainHUDInstance;

	// 이동 명령 시 생성할 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "Nova|UI|Command")
	TObjectPtr<UNiagaraSystem> MoveCommandEffect;

	// 공격 명령 시 생성할 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "Nova|UI|Command")
	TObjectPtr<UNiagaraSystem> AttackCommandEffect;

	// 명령 종류에 따른 시각화 실행 함수
	void SpawnCommandVisualEffect(const FVector& Loc, ECommandType CommandType, AActor* TargetActor = nullptr);

	// 유닛의 PortraitCapture를 조절해줄 헬퍼 함수
	void UpdatePortraitCaptures();
	
	void NotifySelectionChanged();
public:
	/** 선택된 유닛 배열이 변경될 때 호출되는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Nova|UI")
	FOnSelectionChanged OnSelectionChanged;

	/** 현재 선택된 유닛 리스트를 직접 가져오는 Getter */
	UFUNCTION(BlueprintPure, Category = "Nova|Selection")
	const TArray<AActor*>& GetSelectedUnits() const { return SelectedActors; }
};
