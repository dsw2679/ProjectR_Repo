// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (애니메이션 State Enemy 근접 Hit 윈도우 윈도우/트리거 노티파이 구현)
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "PRAnimNotifyState_EnemyMeleeHitWindow.generated.h"

// 지정한 구간 동안 근접 판정 윈도우를 열어두는 Notify State
UCLASS(meta = (DisplayName = "PR Enemy Melee Hit Window"))
class PROJECTR_API UPRAnimNotifyState_EnemyMeleeHitWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/*~ UAnimNotifyState Interface ~*/
public:
	// 에디터에 표시할 노티파이 이름을 반환한다.
	virtual FString GetNotifyName_Implementation() const override;

	// 근접 판정 구간 시작 이벤트를 보낸다.
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	// 근접 판정 구간 갱신 이벤트를 보낸다.
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	// 근접 판정 구간 종료 이벤트를 보낸다.
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
