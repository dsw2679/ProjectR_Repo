// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (콤보 유효 시간대 조준 판정 연동)
// Author: 손승우 (해머 및 보스 콤보 시퀀스 연계 윈도우 트리거 구현)
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PRAnimNotify_EnemyComboWindow.generated.h"

// 콤보 몽타주 섹션 분기 지점용 Notify
// 실제 분기 판단은 서버 Ability의 GameplayEvent 처리 기준
UCLASS(meta = (DisplayName = "PR Enemy Combo Window"))
class PROJECTR_API UPRAnimNotify_EnemyComboWindow : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
