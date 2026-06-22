// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (월드 배치용 웨이포인트 Actor 및 관련 시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRSpawnPoint.h"
#include "ProjectR/Interaction/PRInteractionInterface.h"
#include "ProjectR/UI/WorldMarker/PRPingMarkerTargetInterface.h"
#include "PRWaypointActor.generated.h"

class USceneComponent;
class UPRInteractableComponent;

UCLASS()
class PROJECTR_API APRWaypointActor : public APRSpawnPoint, public IPRInteractionInterface, public IPRPingMarkerTargetInterface
{
	GENERATED_BODY()

public:
	APRWaypointActor();

	/*~ IPRInteractionInterface ~*/
	virtual UPRInteractableComponent* GetInteractableComponent() const override { return InteractableComponent; }

	/*~ IPRPingMarkerTargetInterface ~*/
	virtual FPRWorldMarkerVisualData GetPingMarkerVisualData_Implementation() const override;
	virtual FVector GetPingMarkerWorldLocation_Implementation() const override;

protected:
	// 액터 Transform 기준점으로 사용할 루트 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Waypoint")
	TObjectPtr<USceneComponent> SceneRoot;

	// 상호작용 액션을 제공하기 위한 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UPRInteractableComponent> InteractableComponent;

	// 월드 마커 표시 위치 보정값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|UI")
	FVector PingMarkerOffset = FVector(0.0f, 0.0f, 30.0f);

	// 기본 상호작용 마커 대신 전용 마커를 사용할지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|UI")
	bool bOverridesPingMarker = false;

	// 전용 월드 마커 시각 데이터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bOverridesPingMarker == true"), Category = "ProjectR|UI")
	FPRWorldMarkerVisualData PingMarkerVisualOverride;
};
