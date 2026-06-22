// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (애니메이션 State Update Motion Warp 타겟 윈도우/트리거 노티파이 구현)
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "PRAnimNotifyState_UpdateMotionWarpTarget.generated.h"

// 지정한 구간 동안 AttackTarget 워프 타겟을 계속 갱신하는 Notify State
UCLASS(meta = (DisplayName = "PR Update Motion Warp Target"))
class PROJECTR_API UPRAnimNotifyState_UpdateMotionWarpTarget : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/*~ UAnimNotifyState Interface ~*/
public:
	// 에디터에 표시할 노티파이 이름을 반환한다.
	virtual FString GetNotifyName_Implementation() const override;

	// AttackTarget 워프 타겟 갱신을 시작한다.
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	// AttackTarget 워프 타겟을 계속 갱신한다.
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	// AttackTarget 워프 타겟 갱신을 끝낸다.
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	// true면 타겟 위치까지 워프하고 false면 현재 위치 기준 회전만 사용한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|MotionWarp")
	bool bUseTargetLocation = true;
};
