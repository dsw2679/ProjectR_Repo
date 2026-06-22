// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Tag Utils 시퀀스 어빌리티 구현)
#pragma once

#include "GameplayTagContainer.h"
#include "ProjectR/PRGameplayTags.h"

namespace PRBossAbility
{
	inline FGameplayTagContainer MakePatternAssetTags(const FGameplayTag& BossSpecificPatternTag)
	{
		// 보스 패턴 Ability 일괄 취소를 위해 공통 루트 태그와 보스 전용 태그를 함께 부여한다.
		FGameplayTagContainer AssetTags;
		AssetTags.AddTag(PRGameplayTags::Ability_Boss_Pattern);

		if (BossSpecificPatternTag.IsValid())
		{
			AssetTags.AddTag(BossSpecificPatternTag);
		}

		return AssetTags;
	}

	inline FGameplayTagContainer MakePhaseTransitionAssetTags(const FGameplayTag& BossSpecificPhaseTransitionTag = FGameplayTag())
	{
		// 페이즈 전환 Ability는 패턴 취소 대상에서 빠지도록 별도 루트 태그를 사용한다.
		FGameplayTagContainer AssetTags;
		AssetTags.AddTag(PRGameplayTags::Ability_Boss_PhaseTransition);

		if (BossSpecificPhaseTransitionTag.IsValid())
		{
			AssetTags.AddTag(BossSpecificPhaseTransitionTag);
		}

		return AssetTags;
	}
}
