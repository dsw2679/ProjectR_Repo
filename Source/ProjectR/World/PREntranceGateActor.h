// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (월드 배치용 Entrance 보스 입장 게이트 Actor 및 관련 시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRInteractableActor.h"
#include "PREntranceGateActor.generated.h"

class UBoxComponent;
class USceneComponent;

UCLASS()
class PROJECTR_API APREntranceGateActor : public APRInteractableActor
{
	GENERATED_BODY()

public:
	// 입장 게이트 상호작용 액터 기본 구성
	APREntranceGateActor();

protected:
	// 액터 Transform 기준점으로 사용할 루트 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|EntranceGate")
	TObjectPtr<USceneComponent> SceneRoot;

	// 플레이어 상호작용 감지용 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|EntranceGate")
	TObjectPtr<UBoxComponent> InteractionCollision;
};
