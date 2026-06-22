// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Repeat Fire 구현)
#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "PRAT_RepeatFire.generated.h"

class UPRGA_Fire;
/**
 * Interval 마다 반복되는 델리게이트 발송,
 * OnPerform에 실제 로직 바인딩
 */
UCLASS()
class PROJECTR_API UPRAT_RepeatFire : public UAbilityTask
{
	GENERATED_BODY()

public:
	// 팩토리. bFireImmediately=true 이면 Activate 즉시 1회 수행.
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta=(HidePin="OwningAbility", DefaultToSelf="OwningAbility", BlueprintInternalUseOnly="true"))
	static UPRAT_RepeatFire* RepeatFire(UGameplayAbility* OwningAbility, float Interval, bool bFireImmediately);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void PerformFire();

public:
	// 매 Interval마다 호출. 어빌리티 측에서 FireOneShot을 바인딩.
	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnPerform;
	
private:
	float Interval = 0.1f;
	bool  bFireImmediately = true;
	FTimerHandle TimerHandle;
};
