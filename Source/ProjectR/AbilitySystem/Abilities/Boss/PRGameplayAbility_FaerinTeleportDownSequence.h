// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 순간이동 다운 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_FaerinStagedMontagePattern.h"
#include "PRGameplayAbility_FaerinTeleportDownSequence.generated.h"

// Faerin 원작 TeleportDown 계열을 StagedMontagePattern 설정으로 실행하는 패턴 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaerinTeleportDownSequence : public UPRGameplayAbility_FaerinStagedMontagePattern
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinTeleportDownSequence();
};
