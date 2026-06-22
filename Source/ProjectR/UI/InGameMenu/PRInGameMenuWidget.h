// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (In Game Menu UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRInGameMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UPRGameInstance;
class UPRUIControllerComponent;

// 인게임 메뉴의 저장과 메뉴 복귀를 처리하는 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRInGameMenuWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRInGameMenuWidget();
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// 버튼 이벤트 바인딩
	void BindButtonEvents();

	// 버튼 이벤트 바인딩 해제
	void UnbindButtonEvents();

	// 현재 플레이어 상태를 활성 세이브 슬롯에 저장한다
	bool SaveCurrentCharacter();

	// 상태 텍스트 갱신
	void RefreshStatusText(const FText& StatusText);

	// 현재 GameInstance를 ProjectR 타입으로 반환한다
	UPRGameInstance* GetProjectRGameInstance() const;

	// 현재 UI 컨트롤러 컴포넌트를 반환한다
	UPRUIControllerComponent* GetUIControllerComponent() const;

	// 저장 버튼 클릭 처리
	UFUNCTION()
	void HandleSaveButtonClicked();

	// 메뉴 복귀 버튼 클릭 처리
	UFUNCTION()
	void HandleExitToMenuButtonClicked();

	// 옵션 버튼 클릭 처리
	UFUNCTION()
	void HandleOptionButtonClicked();

	// 닫기 버튼 클릭 처리
	UFUNCTION()
	void HandleCloseButtonClicked();

protected:
	// UMG에서 바인딩할 저장 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|InGameMenu")
	TObjectPtr<UButton> SaveButton;

	// UMG에서 바인딩할 메뉴 복귀 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|InGameMenu")
	TObjectPtr<UButton> ExitToMenuButton;

	// UMG에서 바인딩할 옵션 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|InGameMenu")
	TObjectPtr<UButton> OptionButton;

	// UMG에서 바인딩할 닫기 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|InGameMenu")
	TObjectPtr<UButton> CloseButton;

	// UMG에서 바인딩할 상태 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|InGameMenu")
	TObjectPtr<UTextBlock> StatusText;
};
