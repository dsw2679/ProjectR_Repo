// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (능력치 특성 투자 버튼 및 스탯 변경 정보 구현)
// Author: 배유찬 (특성창 UI 스택 입력 연동)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/Player/Components/PRPlayerGrowthComponent.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRTraitWindowWidget.generated.h"

class APRPlayerState;
class UButton;
class UTextBlock;

USTRUCT(BlueprintType)
struct FPRTraitPreviewInfo
{
	GENERATED_BODY()

public:
	// 표시할 특성 종류
	UPROPERTY(BlueprintReadOnly)
	EPRTraitStatType TraitType = EPRTraitStatType::MaxHealth;

	// 서버에 확정되어 있는 투자 포인트
	UPROPERTY(BlueprintReadOnly)
	int32 CommittedPoint = 0;

	// UI에서 임시로 조정 중인 투자 포인트
	UPROPERTY(BlueprintReadOnly)
	int32 PreviewPoint = 0;

	// 확정값 대비 임시 증감 포인트
	UPROPERTY(BlueprintReadOnly)
	int32 PendingPoint = 0;

	// 해당 특성의 최대 투자 포인트
	UPROPERTY(BlueprintReadOnly)
	int32 MaxPoint = 0;

	// 현재 남은 포인트 안에서 추가로 올릴 수 있는 포인트
	UPROPERTY(BlueprintReadOnly)
	int32 AddablePoint = 0;

	// 확정값으로 되돌릴 수 있는 임시 증가 포인트
	UPROPERTY(BlueprintReadOnly)
	int32 RemovablePoint = 0;

	// 확정 투자 기준 보너스 수치
	UPROPERTY(BlueprintReadOnly)
	float CommittedBonusValue = 0.0f;

	// 임시 투자 기준 보너스 수치
	UPROPERTY(BlueprintReadOnly)
	float PreviewBonusValue = 0.0f;

	// 확정값 대비 임시 보너스 증감 수치
	UPROPERTY(BlueprintReadOnly)
	float DeltaBonusValue = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPRTraitPreviewChangedSignature);

// 특성 투자창의 C++ 부모 위젯
UCLASS()
class PROJECTR_API UPRTraitWindowWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRTraitWindowWidget();
	
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	// 성장 UI가 참조할 PlayerState와 성장 컴포넌트를 지정한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth")
	void SetGrowthSource(APRPlayerState* InPlayerState);

	// 외부에서 넘긴 투자 내역을 서버에 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth")
	void RequestConfirmTraitInvestment(const FPRTraitInvestmentInfo& DesiredInvestment);

	// 확정 버튼 입력 시 현재 임시 투자 내역을 서버에 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth")
	void RequestConfirmPreviewInvestment();

	// 초기화 버튼 입력 시 서버에 즉시 초기화를 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth")
	void RequestResetTraitInvestment();

	// 성장 컴포넌트의 확정 투자값으로 임시 투자값을 다시 맞춘다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth")
	void RefreshPreviewFromGrowth();

	// 특성 임시 투자값을 지정한 양만큼 증가시킨다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth")
	bool IncreaseTraitPoint(EPRTraitStatType TraitType, int32 Amount);

	// 특성 임시 투자값을 지정한 양만큼 감소시킨다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth")
	bool DecreaseTraitPoint(EPRTraitStatType TraitType, int32 Amount);

	// 남은 포인트와 최대 투자치를 고려해 해당 특성에 가능한 만큼 모두 투자한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth")
	bool IncreaseTraitPointToAvailableMax(EPRTraitStatType TraitType);

	// 해당 특성의 임시 증가분을 모두 제거해 확정 투자값으로 되돌린다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth")
	bool DecreaseTraitPointToCommitted(EPRTraitStatType TraitType);

	// 현재 바인딩된 성장 컴포넌트를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Growth")
	UPRPlayerGrowthComponent* GetGrowthComponent() const { return BoundGrowthComponent.Get(); }

	// 현재 임시 투자 내역을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Growth")
	const FPRTraitInvestmentInfo& GetPreviewInvestmentInfo() const { return PreviewInvestmentInfo; }

	// 현재 확정 투자 내역을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Growth")
	const FPRTraitInvestmentInfo& GetCommittedInvestmentInfo() const { return CommittedInvestmentInfo; }

	// 임시 투자 기준 남은 포인트를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Growth")
	int32 GetPreviewRemainingPoint() const;

	// 임시 투자 기준 사용 포인트를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Growth")
	int32 GetPreviewSpentPoint() const;

	// 확정값과 임시값의 포인트 및 보너스 차이를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Growth")
	FPRTraitPreviewInfo GetTraitPreviewInfo(EPRTraitStatType TraitType) const;

	// 모든 특성의 포인트 및 보너스 차이를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Growth")
	TArray<FPRTraitPreviewInfo> GetAllTraitPreviewInfos() const;

	// 바인딩된 텍스트와 버튼 상태를 현재 임시 투자 내역 기준으로 갱신한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Growth")
	void RefreshTraitWindowView();

protected:
	UFUNCTION()
	void HandleTraitInvestmentChanged(const FPRTraitInvestmentInfo& InvestmentInfo);

	UFUNCTION()
	void HandleTraitInvestmentResult(const FPRTraitInvestmentResult& Result);

private:
	// 하위 위젯 버튼 이벤트를 바인딩한다
	void BindChildWidgetEvents();

	// 하위 위젯 버튼 이벤트 바인딩을 정리한다
	void UnbindChildWidgetEvents();

	// 플레이어 컨트롤러 결과 이벤트를 바인딩한다
	void BindSourceEvents();

	// 플레이어 컨트롤러 결과 이벤트 바인딩을 정리한다
	void UnbindSourceEvents();

	// 단일 특성 행의 표시와 버튼 상태를 갱신한다
	void RefreshTraitRow(EPRTraitStatType TraitType, UTextBlock* PointText, UTextBlock* ValueText, UTextBlock* DeltaText,
		UButton* IncreaseButton, UButton* DecreaseButton, UButton* IncreaseMaxButton, UButton* DecreaseMaxButton);

	// 보너스 수치 표시용 텍스트를 만든다
	FText MakeTraitValueText(EPRTraitStatType TraitType, float Value) const;

	// 보너스 증감 수치 표시용 텍스트를 만든다
	FText MakeTraitDeltaText(EPRTraitStatType TraitType, float Value) const;

	// 결과 실패 사유 텍스트를 만든다
	FText MakeTraitInvestmentFailReasonText(EPRTraitInvestmentFailReason FailReason) const;

	// 최대 체력 1포인트 증가 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleMaxHealthIncreaseButtonClicked();

	// 최대 체력 1포인트 감소 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleMaxHealthDecreaseButtonClicked();

	// 최대 체력 가능한 모든 포인트 증가 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleMaxHealthIncreaseMaxButtonClicked();

	// 최대 체력 가능한 모든 임시 포인트 감소 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleMaxHealthDecreaseMaxButtonClicked();

	// 방어력 1포인트 증가 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleArmorIncreaseButtonClicked();

	// 방어력 1포인트 감소 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleArmorDecreaseButtonClicked();

	// 방어력 가능한 모든 포인트 증가 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleArmorIncreaseMaxButtonClicked();

	// 방어력 가능한 모든 임시 포인트 감소 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleArmorDecreaseMaxButtonClicked();

	// 이동속도 1포인트 증가 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleMovementSpeedIncreaseButtonClicked();

	// 이동속도 1포인트 감소 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleMovementSpeedDecreaseButtonClicked();

	// 이동속도 가능한 모든 포인트 증가 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleMovementSpeedIncreaseMaxButtonClicked();

	// 이동속도 가능한 모든 임시 포인트 감소 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleMovementSpeedDecreaseMaxButtonClicked();

	// 공격력 1포인트 증가 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleAttackPowerIncreaseButtonClicked();

	// 공격력 1포인트 감소 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleAttackPowerDecreaseButtonClicked();

	// 공격력 가능한 모든 포인트 증가 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleAttackPowerIncreaseMaxButtonClicked();

	// 공격력 가능한 모든 임시 포인트 감소 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleAttackPowerDecreaseMaxButtonClicked();

	// 최대 스태미나 1포인트 증가 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleMaxStaminaIncreaseButtonClicked();

	// 최대 스태미나 1포인트 감소 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleMaxStaminaDecreaseButtonClicked();

	// 최대 스태미나 가능한 모든 포인트 증가 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleMaxStaminaIncreaseMaxButtonClicked();

	// 최대 스태미나 가능한 모든 임시 포인트 감소 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleMaxStaminaDecreaseMaxButtonClicked();

	// 치명타 확률 1포인트 증가 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleCriticalHitChanceIncreaseButtonClicked();

	// 치명타 확률 1포인트 감소 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleCriticalHitChanceDecreaseButtonClicked();

	// 치명타 확률 가능한 모든 포인트 증가 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleCriticalHitChanceIncreaseMaxButtonClicked();

	// 치명타 확률 가능한 모든 임시 포인트 감소 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleCriticalHitChanceDecreaseMaxButtonClicked();

	// 치명타 피해 배율 1포인트 증가 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleCriticalDamageMultiplierIncreaseButtonClicked();

	// 치명타 피해 배율 1포인트 감소 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleCriticalDamageMultiplierDecreaseButtonClicked();

	// 치명타 피해 배율 가능한 모든 포인트 증가 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleCriticalDamageMultiplierIncreaseMaxButtonClicked();

	// 치명타 피해 배율 가능한 모든 임시 포인트 감소 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleCriticalDamageMultiplierDecreaseMaxButtonClicked();

	// 확정 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleConfirmButtonClicked();

	// 초기화 버튼 클릭을 처리한다
	UFUNCTION()
	void HandleResetButtonClicked();

public:
	// 임시 투자 내역이 바뀔 때 UI 갱신용으로 발행한다
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Growth")
	FPRTraitPreviewChangedSignature OnTraitPreviewChanged;

protected:
	// UMG에서 바인딩할 남은 특성 포인트 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth")
	TObjectPtr<UTextBlock> RemainingPointText;

	// UMG에서 바인딩할 현재 레벨 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth")
	TObjectPtr<UTextBlock> LevelText;

	// UMG에서 바인딩할 사용 특성 포인트 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth")
	TObjectPtr<UTextBlock> SpentPointText;

	// UMG에서 바인딩할 특성 투자 결과 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth")
	TObjectPtr<UTextBlock> ResultText;

	// UMG에서 바인딩할 확정 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth")
	TObjectPtr<UButton> ConfirmButton;

	// UMG에서 바인딩할 초기화 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth")
	TObjectPtr<UButton> ResetButton;

	// UMG에서 바인딩할 최대 체력 투자 포인트 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MaxHealth")
	TObjectPtr<UTextBlock> MaxHealthPointText;

	// UMG에서 바인딩할 최대 체력 보너스 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MaxHealth")
	TObjectPtr<UTextBlock> MaxHealthValueText;

	// UMG에서 바인딩할 최대 체력 증감 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MaxHealth")
	TObjectPtr<UTextBlock> MaxHealthDeltaText;

	// UMG에서 바인딩할 최대 체력 1포인트 증가 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MaxHealth")
	TObjectPtr<UButton> MaxHealthIncreaseButton;

	// UMG에서 바인딩할 최대 체력 1포인트 감소 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MaxHealth")
	TObjectPtr<UButton> MaxHealthDecreaseButton;

	// UMG에서 바인딩할 최대 체력 가능한 모든 포인트 증가 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MaxHealth")
	TObjectPtr<UButton> MaxHealthIncreaseMaxButton;

	// UMG에서 바인딩할 최대 체력 가능한 모든 임시 포인트 감소 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MaxHealth")
	TObjectPtr<UButton> MaxHealthDecreaseMaxButton;

	// UMG에서 바인딩할 방어력 투자 포인트 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Armor")
	TObjectPtr<UTextBlock> ArmorPointText;

	// UMG에서 바인딩할 방어력 보너스 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Armor")
	TObjectPtr<UTextBlock> ArmorValueText;

	// UMG에서 바인딩할 방어력 증감 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Armor")
	TObjectPtr<UTextBlock> ArmorDeltaText;

	// UMG에서 바인딩할 방어력 1포인트 증가 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Armor")
	TObjectPtr<UButton> ArmorIncreaseButton;

	// UMG에서 바인딩할 방어력 1포인트 감소 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Armor")
	TObjectPtr<UButton> ArmorDecreaseButton;

	// UMG에서 바인딩할 방어력 가능한 모든 포인트 증가 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Armor")
	TObjectPtr<UButton> ArmorIncreaseMaxButton;

	// UMG에서 바인딩할 방어력 가능한 모든 임시 포인트 감소 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|Armor")
	TObjectPtr<UButton> ArmorDecreaseMaxButton;

	// UMG에서 바인딩할 이동속도 투자 포인트 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MovementSpeed")
	TObjectPtr<UTextBlock> MovementSpeedPointText;

	// UMG에서 바인딩할 이동속도 보너스 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MovementSpeed")
	TObjectPtr<UTextBlock> MovementSpeedValueText;

	// UMG에서 바인딩할 이동속도 증감 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MovementSpeed")
	TObjectPtr<UTextBlock> MovementSpeedDeltaText;

	// UMG에서 바인딩할 이동속도 1포인트 증가 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MovementSpeed")
	TObjectPtr<UButton> MovementSpeedIncreaseButton;

	// UMG에서 바인딩할 이동속도 1포인트 감소 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MovementSpeed")
	TObjectPtr<UButton> MovementSpeedDecreaseButton;

	// UMG에서 바인딩할 이동속도 가능한 모든 포인트 증가 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MovementSpeed")
	TObjectPtr<UButton> MovementSpeedIncreaseMaxButton;

	// UMG에서 바인딩할 이동속도 가능한 모든 임시 포인트 감소 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MovementSpeed")
	TObjectPtr<UButton> MovementSpeedDecreaseMaxButton;

	// UMG에서 바인딩할 공격력 투자 포인트 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|AttackPower")
	TObjectPtr<UTextBlock> AttackPowerPointText;

	// UMG에서 바인딩할 공격력 보너스 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|AttackPower")
	TObjectPtr<UTextBlock> AttackPowerValueText;

	// UMG에서 바인딩할 공격력 증감 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|AttackPower")
	TObjectPtr<UTextBlock> AttackPowerDeltaText;

	// UMG에서 바인딩할 공격력 1포인트 증가 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|AttackPower")
	TObjectPtr<UButton> AttackPowerIncreaseButton;

	// UMG에서 바인딩할 공격력 1포인트 감소 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|AttackPower")
	TObjectPtr<UButton> AttackPowerDecreaseButton;

	// UMG에서 바인딩할 공격력 가능한 모든 포인트 증가 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|AttackPower")
	TObjectPtr<UButton> AttackPowerIncreaseMaxButton;

	// UMG에서 바인딩할 공격력 가능한 모든 임시 포인트 감소 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|AttackPower")
	TObjectPtr<UButton> AttackPowerDecreaseMaxButton;

	// UMG에서 바인딩할 최대 스태미나 투자 포인트 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MaxStamina")
	TObjectPtr<UTextBlock> MaxStaminaPointText;

	// UMG에서 바인딩할 최대 스태미나 보너스 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MaxStamina")
	TObjectPtr<UTextBlock> MaxStaminaValueText;

	// UMG에서 바인딩할 최대 스태미나 증감 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MaxStamina")
	TObjectPtr<UTextBlock> MaxStaminaDeltaText;

	// UMG에서 바인딩할 최대 스태미나 1포인트 증가 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MaxStamina")
	TObjectPtr<UButton> MaxStaminaIncreaseButton;

	// UMG에서 바인딩할 최대 스태미나 1포인트 감소 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MaxStamina")
	TObjectPtr<UButton> MaxStaminaDecreaseButton;

	// UMG에서 바인딩할 최대 스태미나 가능한 모든 포인트 증가 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MaxStamina")
	TObjectPtr<UButton> MaxStaminaIncreaseMaxButton;

	// UMG에서 바인딩할 최대 스태미나 가능한 모든 임시 포인트 감소 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|MaxStamina")
	TObjectPtr<UButton> MaxStaminaDecreaseMaxButton;

	// UMG에서 바인딩할 치명타 확률 투자 포인트 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|CriticalHitChance")
	TObjectPtr<UTextBlock> CriticalHitChancePointText;

	// UMG에서 바인딩할 치명타 확률 보너스 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|CriticalHitChance")
	TObjectPtr<UTextBlock> CriticalHitChanceValueText;

	// UMG에서 바인딩할 치명타 확률 증감 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|CriticalHitChance")
	TObjectPtr<UTextBlock> CriticalHitChanceDeltaText;

	// UMG에서 바인딩할 치명타 확률 1포인트 증가 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|CriticalHitChance")
	TObjectPtr<UButton> CriticalHitChanceIncreaseButton;

	// UMG에서 바인딩할 치명타 확률 1포인트 감소 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|CriticalHitChance")
	TObjectPtr<UButton> CriticalHitChanceDecreaseButton;

	// UMG에서 바인딩할 치명타 확률 가능한 모든 포인트 증가 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|CriticalHitChance")
	TObjectPtr<UButton> CriticalHitChanceIncreaseMaxButton;

	// UMG에서 바인딩할 치명타 확률 가능한 모든 임시 포인트 감소 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|CriticalHitChance")
	TObjectPtr<UButton> CriticalHitChanceDecreaseMaxButton;

	// UMG에서 바인딩할 치명타 피해 배율 투자 포인트 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|CriticalDamageMultiplier")
	TObjectPtr<UTextBlock> CriticalDamageMultiplierPointText;

	// UMG에서 바인딩할 치명타 피해 배율 보너스 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|CriticalDamageMultiplier")
	TObjectPtr<UTextBlock> CriticalDamageMultiplierValueText;

	// UMG에서 바인딩할 치명타 피해 배율 증감 텍스트
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|CriticalDamageMultiplier")
	TObjectPtr<UTextBlock> CriticalDamageMultiplierDeltaText;

	// UMG에서 바인딩할 치명타 피해 배율 1포인트 증가 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|CriticalDamageMultiplier")
	TObjectPtr<UButton> CriticalDamageMultiplierIncreaseButton;

	// UMG에서 바인딩할 치명타 피해 배율 1포인트 감소 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|CriticalDamageMultiplier")
	TObjectPtr<UButton> CriticalDamageMultiplierDecreaseButton;

	// UMG에서 바인딩할 치명타 피해 배율 가능한 모든 포인트 증가 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|CriticalDamageMultiplier")
	TObjectPtr<UButton> CriticalDamageMultiplierIncreaseMaxButton;

	// UMG에서 바인딩할 치명타 피해 배율 가능한 모든 임시 포인트 감소 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Growth|CriticalDamageMultiplier")
	TObjectPtr<UButton> CriticalDamageMultiplierDecreaseMaxButton;

	// 현재 표시 대상 PlayerState
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ProjectR|Growth")
	TWeakObjectPtr<APRPlayerState> BoundPlayerState;

	// 현재 표시 대상 성장 컴포넌트
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ProjectR|Growth")
	TWeakObjectPtr<UPRPlayerGrowthComponent> BoundGrowthComponent;

	// 서버에 확정되어 있는 투자 내역
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ProjectR|Growth")
	FPRTraitInvestmentInfo CommittedInvestmentInfo;

	// UI에서 확정 전까지 조정하는 임시 투자 내역
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ProjectR|Growth")
	FPRTraitInvestmentInfo PreviewInvestmentInfo;

private:
	void UnbindGrowthComponent();
	int32 GetCurrentLevel() const;
	int32 GetTotalEarnedPoint() const;
	int32 GetInvestmentPoint(const FPRTraitInvestmentInfo& InvestmentInfo, EPRTraitStatType TraitType) const;
	void SetInvestmentPoint(FPRTraitInvestmentInfo& InvestmentInfo, EPRTraitStatType TraitType, int32 Point) const;
	int32 GetTotalSpentPoint(const FPRTraitInvestmentInfo& InvestmentInfo) const;
	int32 GetTraitMaxPoint(EPRTraitStatType TraitType) const;
	float GetTraitValuePerPoint(EPRTraitStatType TraitType) const;
};
