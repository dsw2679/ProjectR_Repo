// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (애니메이션 Refresh Motion Warp 타겟 윈도우/트리거 노티파이 구현)
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PRAnimNotify_RefreshMotionWarpTarget.generated.h"

// 한 번만 AttackTarget 워프 타겟을 갱신하는 Notify
UCLASS(meta = (DisplayName = "PR Refresh Motion Warp Target"))
class PROJECTR_API UPRAnimNotify_RefreshMotionWarpTarget : public UAnimNotify
{
	GENERATED_BODY()

public:
	/*~ UAnimNotify Interface ~*/
public:
	// 에디터에 표시할 노티파이 이름을 반환한다.
	virtual FString GetNotifyName_Implementation() const override;

	// AttackTarget 워프 타겟을 한 번 갱신한다.
	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	// true면 타겟 위치까지 워프하고 false면 현재 위치 기준 회전만 사용한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|MotionWarp")
	bool bUseTargetLocation = true;
};
