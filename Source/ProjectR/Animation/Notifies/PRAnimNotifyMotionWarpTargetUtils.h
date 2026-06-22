// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (애니메이션 Motion Warp 타겟 Utils 윈도우/트리거 노티파이 구현)
#pragma once

#include "CoreMinimal.h"

class USkeletalMeshComponent;

namespace PRAnimNotifyMotionWarpTargetUtils
{
	// AttackTarget 워프 타겟을 현재 타겟 기준으로 갱신한다.
	bool UpdateAttackTargetMotionWarp(USkeletalMeshComponent* MeshComp, bool bUseTargetLocation);
}
