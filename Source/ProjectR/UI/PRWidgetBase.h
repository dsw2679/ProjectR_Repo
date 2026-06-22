// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (버튼 호버/클릭 시 공용 사운드 자동 재생 구현)
// Author: 배유찬 (UI 입력 제어 및 라이프사이클/스택 관리 구현)
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "PRWidgetBase.generated.h"

class UPREventManagerSubsystem;
class UPBViewModelBase;
class UButton;
class UPRUISoundDataAsset;
class USoundBase;

/**
 * 위젯이 요구하는 입력 모드
 */
UENUM(BlueprintType)
enum class EPBUIInputMode : uint8
{
	None,       // 입력 모드 변경 안함 (HUD, 데미지 넘버)
	GameAndUI,  // 카메라 이동 가능 (인벤토리, 캐릭터 시트)
	UIOnly,     // 게임 입력 차단 (대화, 레벨업)
};

/**
 * UI 레이어. ZOrder 및 스택 우선순위에 사용된다.
 * 값이 높을수록 위에 표시되며, 입력 모드 결정 시 우선한다.
 */
UENUM(BlueprintType)
enum class EPRUILayer : uint8
{
	HUD       = 0,  // 게임플레이 HUD (체력, 탄약, 크로스헤어)
	Menu      = 1,  // 인벤토리, 캐릭터 시트, 일시정지
	Modal     = 2,  // 다이얼로그, 확인창
	System    = 3,  // 로딩, 치명적 에러 등 최상위
};

/**
 * UI 전용 입력 의도.
 * UIOnly 위젯은 FKey를 이 enum으로 변환해 키보드와 게임패드 입력을 동일한 의미로 처리한다.
 */
UENUM(BlueprintType)
enum class EPRUIInputAction : uint8
{
	// 처리 대상 아님
	None,

	// 선택 확정
	Confirm,

	// 취소 또는 뒤로가기
	Cancel,

	// 위쪽 항목 이동
	NavigateUp,

	// 아래쪽 항목 이동
	NavigateDown,

	// 왼쪽 항목 이동
	NavigateLeft,

	// 오른쪽 항목 이동
	NavigateRight,

	// 이전 탭 이동
	TabLeft,

	// 다음 탭 이동
	TabRight,
};

/**
 * UI 위젯 기본 클래스
 * Push/Pop 스택에서 관리되는 모든 위젯의 부모 클래스
 */
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	// UI 위젯 기본 포커스 설정 초기화
	UPRWidgetBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 현재 월드의 EventManager 조회
	UPREventManagerSubsystem* GetEventManager() const;

	// Key 입력을 프로젝트 UI 입력 의도로 변환
	UFUNCTION(BlueprintPure, Category = "ProjectR|UI|Input")
	virtual EPRUIInputAction GetUIInputAction(const FKey& Key) const;

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	// 자식 버튼에 기본 UI 사운드 이벤트를 바인딩
	void BindDefaultButtonSoundEvents();

	// 자식 버튼의 기본 UI 사운드 이벤트 바인딩을 해제
	void UnbindDefaultButtonSoundEvents();

	// 기본 UI 사운드 데이터 에셋을 조회
	const UPRUISoundDataAsset* GetDefaultUISoundData() const;

	// UI 사운드 쿨타임을 확인
	bool CanPlayUISound(float LastPlayTime, float Cooldown) const;

	// 버튼 Style Hovered 사운드 존재 여부
	bool HasButtonHoveredStyleSound(const UButton* Button) const;

	// 버튼 Style Clicked 사운드 존재 여부
	bool HasButtonClickedStyleSound(const UButton* Button) const;

	// 2D UI 사운드를 재생
	void PlayUISound(USoundBase* Sound);

	// 기본 버튼 Hovered 사운드를 재생
	UFUNCTION()
	void HandleDefaultButtonHovered();

	// 기본 버튼 Clicked 사운드를 재생
	UFUNCTION()
	void HandleDefaultButtonClicked();

public:
	// 이 위젯이 속한 레이어. UIManager가 ZOrder/스택 정렬에 사용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	EPRUILayer Layer = EPRUILayer::Menu;

	// 이 위젯이 활성화될 때 적용할 입력 모드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	EPBUIInputMode InputMode = EPBUIInputMode::GameAndUI;

	// 이 위젯이 활성화될 때 마우스 커서 표시 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	bool bShowMouseCursor = false;

private:
	// 기본 UI 사운드가 자동 바인딩된 버튼 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> DefaultSoundBoundButtons;

	// 마지막 Hovered 사운드 재생 시각
	float LastHoveredSoundPlayTime = -FLT_MAX;

	// 마지막 Clicked 사운드 재생 시각
	float LastClickedSoundPlayTime = -FLT_MAX;
};

/** TODO [고도화]:
 *  - Push/Pop 진입/퇴장 애니메이션
 *  - ESC 키 닫기 정책 (bClosableByEsc)
 *  - 배경 블러/딤 설정 (Modal 레이어 활성 시 하위 레이어 블러 등)
 */
