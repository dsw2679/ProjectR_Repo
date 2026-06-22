// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Crash(지면충돌) 드롭 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_FaerinCrashSequence.h"
#include "PRGameplayAbility_FaerinCrashDropSequence.generated.h"

// Faerin Ranged 원작 TeleportDown_Crash_Drop branch 전용 CrashDrop Ability이다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaerinCrashDropSequence : public UPRGameplayAbility_FaerinCrashSequence
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinCrashDropSequence();
};
