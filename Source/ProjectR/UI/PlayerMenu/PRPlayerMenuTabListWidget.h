// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonTabListWidgetBase.h"

#include "PRPlayerMenuTabListWidget.generated.h"

class UHorizontalBox;
class UCommonButtonBase;
class UCommonActivatableWidgetSwitcher;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRPlayerMenuTabListWidget : public UCommonTabListWidgetBase
{
	GENERATED_BODY()

public:
	// 디자인 미리보기 탭 버튼 재구성
	void RebuildDesignPreviewTabs(const TArray<FName>& TabNameIDs, TSubclassOf<UCommonButtonBase> ButtonWidgetType);

	// 런타임 탭 목록 재등록
	bool RegisterRuntimeTabs(UCommonActivatableWidgetSwitcher* InWidgetSwitcher, TSubclassOf<UCommonButtonBase> ButtonWidgetType, int32 DesiredTabIndex);
	
protected:
	/*~ UCommonTabListWidgetBase Interface ~*/
	virtual void HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;
	
	virtual void HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;
	
protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|TabList")
	TObjectPtr<UHorizontalBox> TabButtonContainer;
};
