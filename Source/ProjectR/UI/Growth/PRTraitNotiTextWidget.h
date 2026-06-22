// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (성장 특성 Noti Text UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRTraitNotiTextWidget.generated.h"

class UAbilitySystemComponent;
class UPanelWidget;
class UTextBlock;
struct FOnAttributeChangeData;

// 남은 특성 포인트 사용 안내를 표시하는 HUD 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRTraitNotiTextWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRTraitNotiTextWidget(const FObjectInitializer& ObjectInitializer);

	// 소유 플레이어의 성장 Attribute를 다시 찾아 안내 표시를 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Growth")
	void RefreshTraitPointFromOwner();

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 남은 특성 포인트 안내 상태 변경 시 BP 연출 갱신
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|HUD|Growth")
	void BP_OnTraitPointNotificationChanged(bool bVisible, int32 TraitPoint);

private:
	void TryBindToOwnerAbilitySystem();
	void BindToAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent);
	void UnbindFromAbilitySystem();
	void RefreshTraitPointFromAbilitySystem();
	void ApplyTraitPoint(int32 NewTraitPoint);
	void HandleTraitPointChanged(const FOnAttributeChangeData& ChangeData);
	void HandleBindRetryTimer();

protected:
	// 안내 전체 패널
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Growth")
	TObjectPtr<UPanelWidget> TraitNotiPanel;

	// 안내 문구 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|HUD|Growth")
	TObjectPtr<UTextBlock> TraitNotiText;

	// 표시 문구 형식
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|HUD|Growth")
	FText MessageFormat;

	// ASC 준비 전 재시도 간격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|HUD|Growth", meta = (ClampMin = "0.01"))
	float BindRetryInterval = 0.1f;

	// ASC 바인딩 최대 재시도 횟수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|HUD|Growth", meta = (ClampMin = "0"))
	int32 MaxBindRetryCount = 20;

private:
	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;
	FDelegateHandle TraitPointChangedDelegateHandle;
	FTimerHandle BindRetryTimerHandle;
	int32 CurrentBindRetryCount = 0;
	int32 CurrentTraitPoint = 0;
};
