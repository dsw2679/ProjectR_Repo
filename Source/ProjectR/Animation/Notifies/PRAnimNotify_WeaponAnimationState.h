// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (애니메이션 Weapon Animation State 윈도우/트리거 노티파이 구현)
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ProjectR/ItemSystem/Types/PRWeaponAnimationTypes.h"
#include "PRAnimNotify_WeaponAnimationState.generated.h"

// 캐릭터 몽타주의 특정 프레임에 활성 슬롯 무기 메시 애니메이션 상태를 요청하는 Notify다.
// 몽타주 자체가 모든 머신에서 복제 재생되므로 각 머신에서 로컬로 무기 애니메이션이 갱신된다.
// TODO: 무기 애니메이션 재생이 현재 ABP의 state machine을 사용하는데, Montage 재생으로 방식이 어떨지?
UCLASS(meta = (DisplayName = "PR Weapon Animation State"))
class PROJECTR_API UPRAnimNotify_WeaponAnimationState : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	// 노티파이 시점에 활성 슬롯 무기 메시에 요청할 애니메이션 상태
	UPROPERTY(EditAnywhere, Category = "ProjectR|Animation")
	EPRWeaponAnimationState AnimationState = EPRWeaponAnimationState::Idle;
};
