// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (Health Bar UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRHealthBarWidget.generated.h"

class APRPlayerState;
class UAbilitySystemComponent;
class UImage;
class UMaterialInstanceDynamic;
class USizeBox;
class UWidget;
struct FOnAttributeChangeData;

// 체력 바 표시 모드
UENUM(BlueprintType)
enum class EPRHealthBarPresentationMode : uint8
{
	LocalPlayer,
	PartyMember,
};

// 플레이어 체력 HUD 바를 표시하는 부모 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRHealthBarWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRHealthBarWidget(const FObjectInitializer& ObjectInitializer);

	// 지정한 ASC를 기준으로 체력 Attribute 변경 델리게이트를 바인딩한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Health")
	void InitializeFromAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent);

	// 지정한 PlayerState의 ASC를 기준으로 체력 Attribute 변경 델리게이트를 바인딩한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Health")
	void InitializeFromPlayerState(APRPlayerState* InPlayerState);

	// 소유 플레이어의 ASC를 다시 찾아 체력 표시를 갱신한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Health")
	void RefreshHealthFromOwner();

	// 체력 바 표시 모드를 변경한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Health")
	void SetPresentationMode(EPRHealthBarPresentationMode InMode);

	// 현재 바인딩된 체력 정보 출처를 해제한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Health")
	void ClearHealthSource();

	// 현재 체력 비율을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|HUD|Health")
	float GetCurrentHealthPercent() const { return CurrentPercent; }

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 체력 레이어 비율이 바뀔 때 BP 연출 갱신을 위해 호출한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|HUD|Health")
	void BP_OnHealthLayersChanged(float InCurrentPercent, float InRecoverableEndPercent, float InDelayedPercent,
		float InCurrentHealth, float InRecoverableHealth, float InMaxHealth);

	// 위험 체력 상태가 바뀔 때 BP 연출 갱신을 위해 호출한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|HUD|Health")
	void BP_OnHealthCriticalChanged(bool bIsCritical);

	// 다운 게이지 표시 상태 변경 시 BP 연출 갱신 호출
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|HUD|Health")
	void BP_OnDownGaugeChanged(bool bInDownGaugeActive, float InRemainingPercent,
		float InRemainingSeconds, float InDurationSeconds);

private:
	void TryBindToOwnerAbilitySystem();
	void BindToAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent);
	void UnbindFromAbilitySystem();
	void RefreshHealthFromAbilitySystem();
	void RefreshDownGaugeFromPlayerState(bool bForceBroadcast);
	float ResolveServerWorldTimeSeconds() const;
	bool IsDeadHealthSource() const;
	void RefreshLayerPercents(bool bForceDelayedPercent);
	void ApplyDisplayedPercentsToFill();
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleRecoverableHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleBindRetryTimer();
	void StartDelayedLayer(float InStartPercent);
	void ApplyBarLayoutWidths();
	void CacheFillMaterials();
	void ResetFillMaterials();
	void CacheHealthFillOriginalTint();
	void ApplyDownGaugeTint();
	void ApplyLayerPercentToFill(USizeBox* FillSizeBox, UMaterialInstanceDynamic* FillMaterial, float FillPercent, float MaxFillWidth);
	float GetBackBorderWidth() const;
	float GetFillAreaWidth() const;
	float GetMaxFillWidth() const;
	USizeBox* GetCurrentFillSizeBox() const;
	USizeBox* GetRecoverableFillSizeBox() const;
	USizeBox* GetDelayedFillSizeBox() const;
	UImage* ResolveFillImage(UImage* PreferredImage, USizeBox* FillSizeBox) const;
	UImage* FindImageInWidget(UWidget* Widget) const;
	UMaterialInstanceDynamic* ResolveFillMaterial(UImage* FillImage) const;

private:
	// 전체 체력 바 프레임 기준 SizeBox
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<USizeBox> BackBorderSizeBox;

	// 실제 체력 Fill이 배치되는 기준 영역 SizeBox
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<USizeBox> FillAreaSizeBox;

	// 현재 체력 Fill 폭을 조정할 SizeBox
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<USizeBox> HealthFillSizeBox;

	// 회복 가능 체력 Fill 폭을 조정할 SizeBox
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<USizeBox> RecoverableFillSizeBox;

	// 지연 체력 Fill 폭을 조정할 SizeBox
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<USizeBox> DelayedFillSizeBox;

	// 기존 FillClipBox 이름과 호환하기 위한 현재 체력 Fill SizeBox
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<USizeBox> SizeBox_FillClip;

	// 기존 FillClipBox 이름과 호환하기 위한 현재 체력 Fill SizeBox
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<USizeBox> FillClipBox;

	// 현재 체력 Fill 머티리얼을 적용할 Image
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<UImage> HealthFillImage;

	// 회복 가능 체력 Fill 머티리얼을 적용할 Image
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<UImage> RecoverableFillImage;

	// 지연 체력 Fill 머티리얼을 적용할 Image
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<UImage> DelayedFillImage;

	// 플레이어 본인 체력 바 전체 프레임 기준 폭
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "1.0"), Category = "HUD")
	float MaxBarWidth = 500.0f;

	// 파티원 체력 바 전체 프레임 기준 폭
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "1.0"), Category = "HUD")
	float PartyMemberMaxBarWidth = 300.0f;

	// 위험 체력 상태로 취급할 현재 체력 비율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"), Category = "HUD")
	float CriticalHealthThreshold = 0.3f;

	// 지연 체력 레이어가 목표 비율까지 감소하는 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0.01"), Category = "HUD")
	float DelayedLayerDuration = 1.5f;

	// ASC가 아직 준비되지 않았을 때 재시도할 간격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0.01"), Category = "HUD")
	float BindRetryInterval = 0.1f;

	// ASC 바인딩 최대 재시도 횟수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0"), Category = "HUD")
	int32 MaxBindRetryCount = 20;

	// Fill Image의 UI 머티리얼 Percentage 파라미터를 갱신할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD|Material")
	bool bUseMaterialFill = true;

	// Fill 비율을 전달할 머티리얼 Scalar 파라미터 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD|Material")
	FName FillPercentParameterName = TEXT("Percentage");

	// 다운 게이지 활성화 중 적용할 현재 체력 Fill 색상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD|DownGauge")
	FLinearColor DownGaugeFillTint = FLinearColor(1.0f, 0.35f, 0.0f, 1.0f);

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	EPRHealthBarPresentationMode PresentationMode = EPRHealthBarPresentationMode::LocalPlayer;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float CurrentHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float CurrentMaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float CurrentRecoverableHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float CurrentPercent = 1.0f;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float RecoverableEndPercent = 1.0f;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float DelayedPercent = 1.0f;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float DownRemainingSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float DownRemainingPercent = 0.0f;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float DownDurationSeconds = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HealthFillMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RecoverableFillMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DelayedFillMaterial;

	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;
	TWeakObjectPtr<APRPlayerState> CachedPlayerState;

	FSlateColor HealthFillOriginalTint = FSlateColor(FLinearColor::White);

	FDelegateHandle HealthChangedDelegateHandle;
	FDelegateHandle MaxHealthChangedDelegateHandle;
	FDelegateHandle RecoverableHealthChangedDelegateHandle;
	FTimerHandle BindRetryTimerHandle;

	float DelayedStartPercent = 1.0f;
	float DelayedElapsed = 0.0f;
	int32 CurrentBindRetryCount = 0;
	bool bIsDelayedLayerActive = false;
	bool bIsCriticalHealth = false;
	bool bIsDownGaugeActive = false;
	bool bHasCachedHealthFillOriginalTint = false;
};
