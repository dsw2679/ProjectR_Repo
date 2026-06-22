// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (플레이어 Fire Single 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRGA_Fire.h"
#include "PRGA_FireSingle.generated.h"

/**
 * 단발 사격 어빌리티. 1회 발사 후 즉시 EndAbility. 쿨다운(FireInterval) 동안 재활성화 차단
 */
UCLASS()
class PROJECTR_API UPRGA_FireSingle : public UPRGA_Fire
{
	GENERATED_BODY()

public:
	UPRGA_FireSingle();

public:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
