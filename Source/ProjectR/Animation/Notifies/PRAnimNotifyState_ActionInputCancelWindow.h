// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "PRAnimNotifyState_ActionInputCancelWindow.generated.h"

/**
 * 몽타주 기반 액션의 입력 스킵 가능 구간을 여는 Notify State다.
 * 실제 입력 소비와 액션 종료 판단은 현재 등록된 ActionInputConsumer가 처리한다.
 */
UCLASS(meta = (DisplayName = "PR Action Input Cancel Window"))
class PROJECTR_API UPRAnimNotifyState_ActionInputCancelWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/*~ UAnimNotifyState Interface ~*/
	/** 에디터와 몽타주 트랙에 표시할 Notify State 이름을 반환한다 */
	virtual FString GetNotifyName_Implementation() const override;

	/** 입력 스킵 가능 구간을 시작한다 */
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	/** 입력 스킵 가능 구간을 종료한다 */
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	/** 입력 스킵이 발생했을 때 현재 액션 몽타주를 블렌드 아웃할 시간이다 */
	UPROPERTY(EditAnywhere, Category = "ProjectR|Action")
	float InputCancelBlendOutTime = 0.18f;
};
