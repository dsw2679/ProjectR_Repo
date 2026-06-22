// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (애니메이션 페어린 보스 Character Event 윈도우/트리거 노티파이 구현)
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PRAnimNotify_FaerinCharacterEvent.generated.h"

// Faerin 원작 AnimNotify/CharacterEvent 이름을 RouterComponent로 전달한다.
UCLASS(meta = (DisplayName = "PR Faerin Character Event"))
class PROJECTR_API UPRAnimNotify_FaerinCharacterEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	/*~ UAnimNotify Interface ~*/
	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

public:
	// 원작 CharacterEvent 이름이다. 예: SummonPortal_Missile, StartLunge, StopLunge, ShowSword, ShowShards.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|AI|Boss|Faerin")
	FName EventName = NAME_None;
};
