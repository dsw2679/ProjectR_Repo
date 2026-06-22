// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Player Stats Panel UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRPlayerStatsPanelWidget.generated.h"

class APRPlayerState;
class UDataTable;
class UPRAbilitySystemComponent;
class UPRPlayerGrowthComponent;
class UTextBlock;
struct FOnAttributeChangeData;

USTRUCT(BlueprintType)
struct FPRPlayerStatsPanelRowViewData
{
	GENERATED_BODY()

public:
	// 표시할 특성 종류
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Growth|Stats")
	EPRTraitStatType TraitType = EPRTraitStatType::MaxHealth;

	// UI에 표시할 스탯 이름
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Growth|Stats")
	FText DisplayName;

	// 실제 적용된 Attribute 값
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Growth|Stats")
	float StatValue = 0.0f;
};

USTRUCT(BlueprintType)
struct FPRPlayerStatsPanelViewData
{
	GENERATED_BODY()

public:
	// 플레이어 레벨
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Growth|Stats")
	int32 Level = 1;

	// 특성 투자로 강화 가능한 스탯 표시 목록
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Growth|Stats")
	TArray<FPRPlayerStatsPanelRowViewData> StatRows;
};

// 플레이어 성장과 특성 투자 스탯을 메뉴 공통 패널로 표시하는 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRPlayerStatsPanelWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	// 런타임 PlayerState 기준 패널 표시 소스 지정
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth|Stats")
	void SetPlayerStateSource(APRPlayerState* InPlayerState);

	// 현재 표시 데이터 재계산과 텍스트 반영
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth|Stats")
	void RefreshStatsPanelView();

	// 현재 패널 표시 데이터 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Growth|Stats")
	const FPRPlayerStatsPanelViewData& GetCurrentViewData() const { return CurrentViewData; }

protected:
	UFUNCTION()
	void HandleTraitInvestmentChanged(const FPRTraitInvestmentInfo& InvestmentInfo);

	// 표시 대상 Attribute 변경 후 패널 표시값 갱신
	void HandleObservedAttributeChanged(const FOnAttributeChangeData& ChangeData);

protected:
	// UMG에서 바인딩할 플레이어 레벨 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Stats")
	TObjectPtr<UTextBlock> LevelText;

	// UMG에서 바인딩할 최대 체력 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Stats|MaxHealth")
	TObjectPtr<UTextBlock> MaxHealthLabelText;

	// UMG에서 바인딩할 최대 체력 값 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Stats|MaxHealth")
	TObjectPtr<UTextBlock> MaxHealthValueText;

	// UMG에서 바인딩할 방어력 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Stats|Armor")
	TObjectPtr<UTextBlock> ArmorLabelText;

	// UMG에서 바인딩할 방어력 값 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Stats|Armor")
	TObjectPtr<UTextBlock> ArmorValueText;

	// UMG에서 바인딩할 이동 속도 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Stats|MovementSpeed")
	TObjectPtr<UTextBlock> MovementSpeedLabelText;

	// UMG에서 바인딩할 이동 속도 값 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Stats|MovementSpeed")
	TObjectPtr<UTextBlock> MovementSpeedValueText;

	// UMG에서 바인딩할 공격력 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Stats|AttackPower")
	TObjectPtr<UTextBlock> AttackPowerLabelText;

	// UMG에서 바인딩할 공격력 값 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Stats|AttackPower")
	TObjectPtr<UTextBlock> AttackPowerValueText;

	// UMG에서 바인딩할 최대 스태미너 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Stats|MaxStamina")
	TObjectPtr<UTextBlock> MaxStaminaLabelText;

	// UMG에서 바인딩할 최대 스태미너 값 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Stats|MaxStamina")
	TObjectPtr<UTextBlock> MaxStaminaValueText;

	// UMG에서 바인딩할 치명타 확률 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Stats|CriticalHitChance")
	TObjectPtr<UTextBlock> CriticalHitChanceLabelText;

	// UMG에서 바인딩할 치명타 확률 값 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Stats|CriticalHitChance")
	TObjectPtr<UTextBlock> CriticalHitChanceValueText;

	// UMG에서 바인딩할 치명타 피해 이름 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Stats|CriticalDamageMultiplier")
	TObjectPtr<UTextBlock> CriticalDamageMultiplierLabelText;

	// UMG에서 바인딩할 치명타 피해 값 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Stats|CriticalDamageMultiplier")
	TObjectPtr<UTextBlock> CriticalDamageMultiplierValueText;

private:
	struct FPRStatsPanelTextBinding
	{
		EPRTraitStatType TraitType = EPRTraitStatType::MaxHealth;
		UTextBlock* LabelText = nullptr;
		UTextBlock* ValueText = nullptr;
	};

	void BindGrowthComponent();
	void UnbindGrowthComponent();
	void BindAttributeChanges();
	void UnbindAttributeChanges();
	void CacheTextBindings();
	void ApplyViewDataToWidgets();
	void BuildPlayerStateViewData(APRPlayerState* InPlayerState, FPRPlayerStatsPanelViewData& OutViewData) const;
	void AddTraitRowFromPlayerState(APRPlayerState* InPlayerState, EPRTraitStatType TraitType, FPRPlayerStatsPanelViewData& OutViewData) const;
	bool FindTraitRule(EPRTraitStatType TraitType, FPRTraitStatRuleRow& OutRule) const;
	UDataTable* ResolveTraitRuleTable() const;
	FText GetTraitDisplayName(EPRTraitStatType TraitType) const;
	FText MakeTraitValueText(EPRTraitStatType TraitType, float Value) const;
	const FPRPlayerStatsPanelRowViewData* FindRowViewData(EPRTraitStatType TraitType) const;
	TArray<EPRTraitStatType> GetDisplayTraitTypes() const;

private:
	UPROPERTY(Transient)
	FPRPlayerStatsPanelViewData CurrentViewData;

	TArray<FPRStatsPanelTextBinding> TextBindings;

	UPROPERTY(Transient)
	TWeakObjectPtr<APRPlayerState> BoundPlayerState;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPRPlayerGrowthComponent> BoundGrowthComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPRAbilitySystemComponent> BoundAbilitySystemComponent;

	TArray<FGameplayAttribute> BoundAttributeChangeAttributes;

	TArray<FDelegateHandle> AttributeChangeHandles;
};
