// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (월드 배치용 스폰 Point Tags 및 관련 시스템 구현)
#include "PRSpawnPointTags.h"

namespace PRSpawnPointTags
{
	// 기본 진입 SpawnPoint
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SpawnPoint_Default, "SpawnPoint.Default", "기본 진입 SpawnPoint");

	// 테스트용 보조 SpawnPoint
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SpawnPoint_Secondary, "SpawnPoint.Secondary", "테스트용 보조 SpawnPoint");
}
