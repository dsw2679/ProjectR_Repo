// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (월드 배치용 데이터 에셋 및 관련 시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/WaypointTravel/PRWaypointTravelTypes.h"
#include "PRWorldDataAsset.generated.h"

class UPRWaypointTravelMapWidget;

// Travel UI에 표시할 맵과 웨이포인트 목적지 목록
UCLASS(BlueprintType)
class PROJECTR_API UPRWorldDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 지정한 웨이포인트 ID를 가진 목적지 노드 존재 여부 반환
	bool HasWaypointNode(FGameplayTag WaypointId) const
	{
		return WaypointNodes.ContainsByPredicate([WaypointId](const FPRWaypointTravelNodeDefinition& Node)
		{
			return Node.WaypointId.MatchesTagExact(WaypointId);
		});
	}

	// 지정한 웨이포인트 ID를 가진 목적지 노드 조회
	bool FindWaypointNode(FGameplayTag WaypointId, FPRWaypointTravelNodeDefinition& OutNode) const
	{
		const FPRWaypointTravelNodeDefinition* FoundNode = WaypointNodes.FindByPredicate([WaypointId](const FPRWaypointTravelNodeDefinition& Node)
		{
			return Node.WaypointId.MatchesTagExact(WaypointId);
		});
		if (!FoundNode)
		{
			return false;
		}

		OutNode = *FoundNode;
		return true;
	}

public:
	// Registry의 Key와 일치해야 하는 논리 월드 식별자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WaypointTravel", meta = (Categories = "World"))
	FGameplayTag WorldId;

	// ServerTravel 대상 맵
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	TSoftObjectPtr<UWorld> MapAsset;

	// UI에 표시할 맵 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	FText DisplayName;

	// UI 썸네일
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	TSoftObjectPtr<UTexture2D> ThumbnailTexture;

	// 월드별 지도 화면으로 생성할 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	TSubclassOf<UPRWaypointTravelMapWidget> MapWidgetClass;

	// 이 맵에서 선택 가능한 웨이포인트 노드 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	TArray<FPRWaypointTravelNodeDefinition> WaypointNodes;
};
