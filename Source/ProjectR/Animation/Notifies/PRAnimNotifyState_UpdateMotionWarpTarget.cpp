// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (애니메이션 State Update Motion Warp 타겟 윈도우/트리거 노티파이 구현)
#include "PRAnimNotifyState_UpdateMotionWarpTarget.h"

#include "PRAnimNotifyMotionWarpTargetUtils.h"

FString UPRAnimNotifyState_UpdateMotionWarpTarget::GetNotifyName_Implementation() const
{
	return TEXT("PR Update Motion Warp Target");
}

void UPRAnimNotifyState_UpdateMotionWarpTarget::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	PRAnimNotifyMotionWarpTargetUtils::UpdateAttackTargetMotionWarp(MeshComp, bUseTargetLocation);
}

void UPRAnimNotifyState_UpdateMotionWarpTarget::NotifyTick(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	PRAnimNotifyMotionWarpTargetUtils::UpdateAttackTargetMotionWarp(MeshComp, bUseTargetLocation);
}

void UPRAnimNotifyState_UpdateMotionWarpTarget::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
}
