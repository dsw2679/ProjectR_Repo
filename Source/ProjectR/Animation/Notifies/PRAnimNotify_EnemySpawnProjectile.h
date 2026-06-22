// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (조준 방어 상태 연동 투사체 판정)
// Author: 손승우 (로열 아처 및 보스 원거리 투사체 스폰 트리거 구현)
// Author: 이건주 (Penitent 사격 시 투사체 발사 트리거 구현)
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PRAnimNotify_EnemySpawnProjectile.generated.h"

// 공격 몽타주의 투사체 발사 프레임에 배치하는 Notify
// Ability 인스턴스를 직접 찾지 않고 GameplayEvent만 보내 투사체 스폰 흐름을 GAS 쪽에 맡김
UCLASS(meta = (DisplayName = "PR Enemy Spawn Projectile"))
class PROJECTR_API UPRAnimNotify_EnemySpawnProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
