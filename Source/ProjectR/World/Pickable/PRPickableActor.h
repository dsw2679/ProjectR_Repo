// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (월드 배치용 획득 가능 Actor 및 관련 시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/WorldMarker/PRPingMarkerTargetInterface.h"
#include "ProjectR/World/PRInteractableActor.h"
#include "PRPickableActor.generated.h"

class USphereComponent;

UCLASS()
class PROJECTR_API APRPickableActor : public APRInteractableActor
{
	GENERATED_BODY()

public:
	APRPickableActor();

	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// ===== Components =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|UI")
	TObjectPtr<USphereComponent> InteractionCollision;

	// 월드 오브젝트 리스폰 시 파괴할 일회성 액터 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Respawn")
	bool bDisposable = false;
};
