// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Entrance 보스 입장 게이트 상호작용 액션 실행 로직 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PRInteraction_MapTravelBase.h"
#include "PRInteraction_EntranceGate.generated.h"

class UWorld;

// 고정 WorldId와 WaypointId로 이동하는 입장 게이트 상호작용
UCLASS()
class PROJECTR_API UPRInteraction_EntranceGate : public UPRInteraction_MapTravelBase
{
	GENERATED_BODY()

public:
	// 입장 게이트 상호작용 기본값 설정
	UPRInteraction_EntranceGate();

protected:
	/*~ UPRInteraction_PartySyncBase Interface ~*/
	// 입장 게이트 조건 충족 시 Registry 기반 목적지 이동 처리
	virtual void HandlePartySyncConditionMet() override;

	/*~ UPRInteraction_EntranceGate Interface ~*/
	// 입장 게이트 목적지 맵과 Waypoint 태그 조회
	bool ResolveTargetTravelData(TSoftObjectPtr<UWorld>& OutResolvedMapAsset, FGameplayTag& OutTargetWaypointId) const;

protected:
	// 입장 게이트 이동 대상 WorldId
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|EntranceGate", meta = (Categories = "World"))
	FGameplayTag TargetWorldId;

	// 입장 게이트 목적지 WaypointId. 목적지 월드의 SpawnPointId와 같은 태그 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|EntranceGate", meta = (Categories = "SpawnPoint"))
	FGameplayTag TargetWaypointId;
	
	// 페이드 아웃 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|EntranceGate")
	float FadeDuration = 1.6f;
	
};
