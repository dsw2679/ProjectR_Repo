// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Boss Health Bar UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRBossHealthBarWidget.generated.h"

class APRBossBaseCharacter;
class UAbilitySystemComponent;
class USizeBox;
struct FOnAttributeChangeData;

// 화면 상단 보스 HP 바를 표시하는 부모 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRBossHealthBarWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRBossHealthBarWidget(const FObjectInitializer& ObjectInitializer);

	// 보스 캐릭터를 HP 바에 바인딩한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Boss")
	void BindBoss(APRBossBaseCharacter* InBoss);

	// 현재 보스 바인딩을 해제하고 HP 바를 숨긴다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Boss")
	void ClearBoss();

	// 보스 바인딩 여부를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|HUD|Boss")
	bool IsBossBound() const { return CachedBoss.IsValid(); }

	// 현재 HP 비율을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|HUD|Boss")
	float GetHealthPercent() const { return HealthPercent; }

	// 감소 지연 HP 비율을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|HUD|Boss")
	float GetDelayedHealthPercent() const { return DelayedHealthPercent; }

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// HP 레이어 비율이 바뀔 때 BP 연출 갱신을 위해 호출한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|HUD|Boss")
	void BP_OnBossHealthChanged(float InHealthPercent, float InDelayedHealthPercent,
		float InCurrentHealth, float InMaxHealth);

	// 보스 이름이 바뀔 때 BP 표시 갱신을 위해 호출한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|HUD|Boss")
	void BP_OnBossNameChanged(const FText& InBossName);

	// 보스 페이즈 마커 비율이 바뀔 때 BP 표시 갱신을 위해 호출한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|HUD|Boss")
	void BP_OnBossPhaseMarkersChanged(const TArray<float>& InMarkerPercents);

	// 보스 HP 바 표시 상태가 바뀔 때 BP 연출 갱신을 위해 호출한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|HUD|Boss")
	void BP_OnBossHealthVisibilityChanged(bool bVisible);

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
	void SetBossHealthVisible(bool bVisible);

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
	float MaxBarWidth = 900.0f;

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

	TWeakObjectPtr<APRBossBaseCharacter> CachedBoss;
	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;
	FDelegateHandle HealthChangedDelegateHandle;
	FDelegateHandle MaxHealthChangedDelegateHandle;

	float DelayedStartPercent = 0.0f;
	float DelayedElapsed = 0.0f;
	bool bIsDelayedLayerActive = false;
};
