// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (월드 배치용 스폰 Point Tags 및 관련 시스템 구현)
#pragma once

#include "NativeGameplayTags.h"

// SpawnPoint 식별용 Native GameplayTag 모음
namespace PRSpawnPointTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SpawnPoint_Default); // 기본 진입 SpawnPoint
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SpawnPoint_Secondary); // 테스트용 보조 SpawnPoint
}
