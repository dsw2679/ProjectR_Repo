// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Enemy World Health Bar UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PREnemyWorldHealthBarWidget.generated.h"

class UAbilitySystemComponent;
class USizeBox;
struct FOnAttributeChangeData;

// 일반 몬스터 머리 위 HP 바를 표시하는 부모 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPREnemyWorldHealthBarWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPREnemyWorldHealthBarWidget(const FObjectInitializer& ObjectInitializer);

	// 지정한 ASC를 기준으로 HP Attribute 변경 델리게이트를 바인딩한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Enemy")
	void InitializeFromAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent);

	// 현재 바인딩된 HP 정보 출처를 해제한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Enemy")
	void ClearHealthSource();

	// 현재 HP 비율을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|HUD|Enemy")
	float GetHealthPercent() const { return HealthPercent; }

	// 감소 지연 HP 비율을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|HUD|Enemy")
	float GetDelayedHealthPercent() const { return DelayedHealthPercent; }

	// 감소 지연 레이어를 현재 HP 비율에 즉시 맞춘다.
	void CompleteDelayedLayer();

	// 컴포넌트가 수신한 HP 변경값으로 표시를 즉시 갱신한다.
	void ApplyHealthChange(float InOldHealth, float InNewHealth, float InMaxHealth);

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// HP 레이어 비율이 바뀔 때 BP 연출 갱신을 위해 호출한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|HUD|Enemy")
	void BP_OnEnemyHealthChanged(float InHealthPercent, float InDelayedHealthPercent,
		float InCurrentHealth, float InMaxHealth);

private:
	void BindToAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent);
	void UnbindFromAbilitySystem();
	void RefreshHealthFromAbilitySystem(bool bForceDelayedPercent);
	void RefreshHealthPercents(bool bForceDelayedPercent);
	void StartDelayedLayer(float InStartPercent);
	void BroadcastHealthChanged();
	void ApplyDisplayedPercentsToFill();
	void ApplyBarLayoutWidths();
	float GetBackBorderWidth() const;
	float GetFillAreaWidth() const;
	float GetMaxFillWidth() const;
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData);

private:
	// 전체 HP 바 프레임 기준 SizeBox
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<USizeBox> BackBorderSizeBox;

	// 실제 HP Fill이 배치되는 기준 영역 SizeBox
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<USizeBox> FillAreaSizeBox;

	// 현재 HP Fill 폭을 조정할 SizeBox
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<USizeBox> HealthFillSizeBox;

	// 감소 지연 HP Fill 폭을 조정할 SizeBox
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<USizeBox> DelayedFillSizeBox;

	// 전체 HP 바 프레임 기준 폭
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "1.0"), Category = "HUD")
	float MaxBarWidth = 140.0f;

	// 감소 지연 레이어가 현재 HP 비율에 도달하는 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0.01"), Category = "HUD")
	float DelayedLayerDuration = 1.5f;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float CurrentHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float CurrentMaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float HealthPercent = 0.0f;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float DelayedHealthPercent = 0.0f;

	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;
	FDelegateHandle HealthChangedDelegateHandle;
	FDelegateHandle MaxHealthChangedDelegateHandle;

	float DelayedStartPercent = 0.0f;
	float DelayedElapsed = 0.0f;
	bool bIsDelayedLayerActive = false;
};
