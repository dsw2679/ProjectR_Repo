// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PlayerMenu/PRPlayerMenuTypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRPlayerMenu.generated.h"

class APRPlayerState;
class UCommonButtonBase;
class UCommonTabListWidgetBase;
class UCommonActivatableWidgetSwitcher;
class UPREquipmentManagerComponent;
class UPRInventoryComponent;
class UPRQuickSlotComponent;
class UPRWeaponManagerComponent;
/**
 *
 */
UCLASS()
class PROJECTR_API UPRPlayerMenu : public UPRWidgetBase
{
	GENERATED_BODY()
public:
	// 플레이어 메뉴 기본 입력 설정 초기화
	UPRPlayerMenu();

	// 디자인 타임 미리보기 처리
	virtual void NativePreConstruct() override;

	// 런타임 탭 목록 갱신
	bool RefreshRuntimeTabs(EPRPlayerMenuTabType DesiredTabType);

	// 런타임 탭 선택
	UFUNCTION(BlueprintCallable, Category = "ProjectR|PlayerMenu")
	bool SelectRuntimeTab(EPRPlayerMenuTabType TabType);

	// 현재 선택된 런타임 탭 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|PlayerMenu")
	EPRPlayerMenuTabType GetSelectedRuntimeTabType() const;

	// 하위 메뉴 위젯 데이터 소스 설정
	void SetMenuSources(UPRInventoryComponent* InInventoryComponent, UPRWeaponManagerComponent* InWeaponManagerComponent, UPRQuickSlotComponent* InQuickSlotComponent, UPREquipmentManagerComponent* InEquipmentManagerComponent, APRPlayerState* InPlayerState);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|PlayerMenu")
	TObjectPtr<UCommonActivatableWidgetSwitcher> WidgetSwitcher;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|PlayerMenu")
	TObjectPtr<UCommonTabListWidgetBase> TabList;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|PlayerMenu")
	TSubclassOf<UCommonButtonBase> TabButtonClass;

private:
	// 탭 유형에 해당하는 스위처 인덱스 조회
	int32 ResolveRuntimeTabIndex(EPRPlayerMenuTabType TabType) const;

	// 스위처 인덱스에 해당하는 탭 유형 조회
	EPRPlayerMenuTabType ResolveRuntimeTabTypeByIndex(int32 TabIndex) const;
};
