// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 포털 Attached 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossPortalSequence.h"
#include "PRGameplayAbility_FaerinPortalAttachedSequence.generated.h"

// Faerin 2페이즈 부착형 좌/우 포탈 패턴의 C++ 부모 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaerinPortalAttachedSequence : public UPRGameplayAbility_BossPortalSequence
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinPortalAttachedSequence();
};

