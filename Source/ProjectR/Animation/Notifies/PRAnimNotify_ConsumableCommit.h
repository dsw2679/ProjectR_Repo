// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (아이템 복용 중 행동 차단 상태 해제 연동)
// Author: 손승우 (적 AI 소모품 효과 연동)
// Author: 이건주 (소모품 사용 애니메이션 완료 시점 회복 효과 트리거 구현)
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PRAnimNotify_ConsumableCommit.generated.h"

// 소비 아이템 사용 몽타주의 효과 적용 프레임에 배치하는 Notify다
UCLASS(meta = (DisplayName = "PR Consumable Commit"))
class PROJECTR_API UPRAnimNotify_ConsumableCommit : public UAnimNotify
{
	GENERATED_BODY()

public:
	/*~ UAnimNotify Interface ~*/
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
