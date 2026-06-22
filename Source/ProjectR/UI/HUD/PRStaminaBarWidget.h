// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (Stamina Bar UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRStaminaBarWidget.generated.h"

class UAbilitySystemComponent;
class USizeBox;
struct FOnAttributeChangeData;

// 플레이어 스태미나 HUD 바 표시를 담당하는 부모 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRStaminaBarWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRStaminaBarWidget(const FObjectInitializer& ObjectInitializer);

	// 지정한 ASC를 기준으로 스태미나 변경 델리게이트를 바인딩한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Stamina")
	void InitializeFromAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent);

	// 소유 플레이어의 ASC에서 현재 스태미나와 최대 스태미나를 다시 읽어 UI에 반영한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Stamina")
	void RefreshStaminaFromOwner();

	// 스태미나 비율을 직접 적용한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|HUD|Stamina")
	void SetStaminaPercent(float NewPercent);

	// 현재 UI에 반영된 스태미나 비율을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|HUD|Stamina")
	float GetStaminaPercent() const { return CurrentPercent; }

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 스태미나 비율이 바뀐 뒤 BP 연출을 추가할 수 있도록 호출된다
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|HUD|Stamina")
	void BP_OnStaminaPercentChanged(float NewPercent, float InCurrentStamina, float InMaxStamina);

private:
	// 소유 플레이어의 ASC를 찾아 델리게이트 바인딩을 시도한다
	void TryBindToOwnerAbilitySystem();

	// 지정한 ASC의 스태미나 변경 델리게이트에 바인딩한다
	void BindToAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent);

	// 현재 ASC에 등록한 델리게이트를 해제한다
	void UnbindFromAbilitySystem();

	// 캐시된 ASC에서 스태미나 값을 읽어 UI를 갱신한다
	void RefreshStaminaFromAbilitySystem();

	// 현재 표시 비율을 기준으로 Fill 영역 너비를 갱신한다
	void ApplyDisplayedPercentToFill();

	// 스태미나 값 변경을 처리한다
	void HandleStaminaChanged(const FOnAttributeChangeData& ChangeData);

	// 최대 스태미나 값 변경을 처리한다
	void HandleMaxStaminaChanged(const FOnAttributeChangeData& ChangeData);

	// ASC 준비가 늦을 때 재시도한다
	void HandleBindRetryTimer();

	// BP에서 바인딩된 Fill SizeBox를 반환한다
	USizeBox* GetFillSizeBox() const;

private:
	// Fill 영역 너비를 직접 조정할 SizeBox. BP 위젯에서 이 이름으로 생성하면 자동 바인딩된다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<USizeBox> StaminaFillSizeBox;

	// 기존 설명에서 사용한 이름과 호환하기 위한 선택 바인딩이다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<USizeBox> SizeBox_FillClip;

	// FillClipBox 이름을 사용한 BP와 호환하기 위한 선택 바인딩이다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"), Category = "HUD")
	TObjectPtr<USizeBox> FillClipBox;

	// 스태미나 100퍼센트일 때 Fill 영역의 기준 너비
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "1.0"), Category = "HUD")
	float MaxBarWidth = 330.0f;

	// ASC가 아직 준비되지 않았을 때 재시도할 간격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0.01"), Category = "HUD")
	float BindRetryInterval = 0.1f;

	// ASC 바인딩 최대 재시도 횟수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0"), Category = "HUD")
	int32 MaxBindRetryCount = 20;

	// UI에 반영된 현재 스태미나 비율
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float CurrentPercent = 1.0f;

	// 보간이 도달해야 하는 목표 스태미나 비율
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float TargetPercent = 1.0f;

	// 스태미나 바 너비 변화를 부드럽게 보간할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD|Interpolation")
	bool bInterpolateStaminaChange = true;

	// 스태미나 바가 목표 비율을 따라가는 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0.1"), Category = "HUD|Interpolation")
	float StaminaInterpSpeed = 12.0f;

	// 이 값보다 차이가 작으면 목표 비율로 스냅한다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0.0"), Category = "HUD|Interpolation")
	float StaminaSnapTolerance = 0.001f;

	// 마지막으로 읽은 현재 스태미나
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float CurrentStamina = 0.0f;

	// 마지막으로 읽은 최대 스태미나
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "HUD")
	float CurrentMaxStamina = 0.0f;

	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;

	FDelegateHandle StaminaChangedDelegateHandle;

	FDelegateHandle MaxStaminaChangedDelegateHandle;

	FTimerHandle BindRetryTimerHandle;

	int32 CurrentBindRetryCount = 0;

	bool bHasInitializedPercent = false;
};
