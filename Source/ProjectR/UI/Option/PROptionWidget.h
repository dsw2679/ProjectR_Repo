// Copyright ProjectR. All Rights Reserved.
// 김동석 (bgm옵션 기능 추가)

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PROptionWidget.generated.h"

class UButton;
class UPRUIControllerComponent;

// 인게임 옵션 설정을 표시하는 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPROptionWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPROptionWidget();

	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// 버튼 이벤트 바인딩
	void BindButtonEvents();

	// 버튼 이벤트 바인딩 해제
	void UnbindButtonEvents();

	// 현재 UI 컨트롤러 컴포넌트 반환
	UPRUIControllerComponent* GetUIControllerComponent() const;

	// 닫기 버튼 클릭 처리
	UFUNCTION()
	void HandleCloseButtonClicked();

protected:
	// UMG에서 바인딩할 닫기 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Option")
	TObjectPtr<UButton> CloseButton;
};
