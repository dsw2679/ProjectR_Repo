// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 플레이어 인력(Shift) Player Close 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_FaerinShiftSequence.h"
#include "PRGameplayAbility_FaerinShiftPlayerCloseSequence.generated.h"

// Faerin 원작 ShiftPlayerClose 패턴의 기본값을 고정하는 전용 Ability다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_FaerinShiftPlayerCloseSequence : public UPRGameplayAbility_FaerinShiftSequence
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinShiftPlayerCloseSequence();
};
