// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (UI Enemy World Health Bar 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "PREnemyWorldHealthBarComponent.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

// 일반 몬스터 머리 위 HP 바 표시와 숨김 타이머를 관리하는 컴포넌트
UCLASS(ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPREnemyWorldHealthBarComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UPREnemyWorldHealthBarComponent();

	/*~ UActorComponent Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// 지정한 ASC를 기준으로 HP 표시를 초기화한다.
	void InitializeFromAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent);

	// 일정 시간 동안 HP 바를 표시한다.
	void ShowForDuration();

	// HP 바를 즉시 숨긴다.
	void HideHealthBar();

	void SuppressVisibility(bool bSuppressed) {bSuppressedVisibility = bSuppressed;}

private:
	void BindToAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent);
	void UnbindFromAbilitySystem();
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleHideTimer();
	void UpdateHealthBarVisibility(bool bInVisible);
	void InitializeUserWidget();

private:
	// 마지막 피격 후 표시 유지 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0.0"), Category = "ProjectR|UI")
	float VisibleDurationAfterHit = 3.0f;

	// 머리 위 표시 위치 보정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "ProjectR|UI")
	FVector HeadOffset = FVector(0.0f, 0.0f, 120.0f);

	// 풀피 상태에서 기본 숨김 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "ProjectR|UI")
	bool bHideAtFullHealth = true;

	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;
	FDelegateHandle HealthChangedDelegateHandle;
	FDelegateHandle MaxHealthChangedDelegateHandle;
	FTimerHandle HideTimerHandle;

	float CurrentHealth = 0.0f;
	float CurrentMaxHealth = 0.0f;
	float LastCurrentHealth = 0.0f;
	float LastMaxHealth = 0.0f;
	
	bool bSuppressedVisibility = false;
};
