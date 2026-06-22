// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (애니메이션 Refresh Motion Warp 타겟 윈도우/트리거 노티파이 구현)
#include "PRAnimNotify_RefreshMotionWarpTarget.h"

#include "PRAnimNotifyMotionWarpTargetUtils.h"

FString UPRAnimNotify_RefreshMotionWarpTarget::GetNotifyName_Implementation() const
{
	return TEXT("PR Refresh Motion Warp Target");
}

void UPRAnimNotify_RefreshMotionWarpTarget::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	PRAnimNotifyMotionWarpTargetUtils::UpdateAttackTargetMotionWarp(MeshComp, bUseTargetLocation);
}
