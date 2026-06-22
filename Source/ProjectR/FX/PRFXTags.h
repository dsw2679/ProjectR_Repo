// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (사격 궤적 및 일반 투사체 이펙트 태그 정의)
// Author: 손승우 (보스 패턴 관련 비주얼 이펙트 태그 정의)
#pragma once

#include "NativeGameplayTags.h"

// FX 시스템 전용 Native GameplayTag 선언 모음
namespace PRFXTags
{
	// 무기 발사 Trail 테스트 Cue 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(FX_Weapon_Trail);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(FX_Weapon_OutOfAmmo);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(FX_Weapon_SimpleFire);
}
