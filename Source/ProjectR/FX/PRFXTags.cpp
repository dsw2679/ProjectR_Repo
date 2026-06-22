// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (사격 궤적 및 일반 투사체 이펙트 태그 정의)
// Author: 손승우 (보스 패턴 관련 비주얼 이펙트 태그 정의)
#include "PRFXTags.h"

namespace PRFXTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(FX_Weapon_Trail, "FX.Weapon.Trail", "무기 발사 Trail 테스트 Cue 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(FX_Weapon_OutOfAmmo, "FX.Weapon.OutOfAmmo", "탄약 부족 Cue");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(FX_Weapon_SimpleFire, "FX.Weapon.SimpleFire", "무기 발사 Cue (Trail 없음)");
}
