// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (애니메이션 State 소모품 픽업 Mesh 윈도우/트리거 노티파이 구현)
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "PRAnimNotifyState_ConsumablePickupMesh.generated.h"

class UPRConsumableDataAsset;
class USkeletalMeshComponent;

// 소비 아이템 PickupMesh 표시 Notify State
UCLASS(meta = (DisplayName = "PR Consumable Pickup Mesh"))
class PROJECTR_API UPRAnimNotifyState_ConsumablePickupMesh : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/*~ UAnimNotifyState Interface ~*/
	// 에디터 타임라인 표시 이름
	virtual FString GetNotifyName_Implementation() const override;

	// PickupMesh 표시 요청
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	// PickupMesh 제거 요청
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, Category = "ProjectR|Consumable")
	TObjectPtr<UPRConsumableDataAsset> ConsumableData = nullptr; // 소비 아이템 데이터

	// PickupMesh 부착 소켓
	UPROPERTY(EditAnywhere, Category = "ProjectR|Consumable|Mesh")
	FName AttachSocketName = NAME_None;
};
