// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (UI 웨이포인트 Travel 타입 및 구조체 정의)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "PRWaypointTravelTypes.generated.h"

// Travel UI에서 하나의 버튼이 가리키는 목적지 웨이포인트
USTRUCT(BlueprintType)
struct PROJECTR_API FPRWaypointTravelNodeDefinition
{
	GENERATED_BODY()

	// 목적지 맵의 SpawnPoint와 매칭할 웨이포인트 식별자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WaypointTravel", meta = (Categories = "SpawnPoint"))
	FGameplayTag WaypointId;

	// UI에 표시할 웨이포인트 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	FText DisplayName;

	// UI에 표시할 썸네일
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	TSoftObjectPtr<UTexture2D> ThumbnailTexture;
};

// Travel UI의 월드 목록 표시에 사용하는 선택지
USTRUCT(BlueprintType)
struct PROJECTR_API FPRWaypointTravelWorldOption
{
	GENERATED_BODY()

	// 월드 선택과 Registry 조회에 사용할 논리 월드 식별자
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel", meta = (Categories = "World"))
	FGameplayTag WorldId;

	// UI에 표시할 월드 이름
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	FText WorldDisplayName;

	// 월드 썸네일
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	TSoftObjectPtr<UTexture2D> ThumbnailTexture;

	// 해당 월드 안에 활성화된 Waypoint가 하나 이상 있는지 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	bool bHasUnlockedWaypoint = false;

	// 개발 모드에서만 노출된 월드 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	bool bDevOnly = false;
};

// Travel UI와 서버 요청 사이에서 전달하는 목적지 노드
USTRUCT(BlueprintType)
struct PROJECTR_API FPRWaypointTravelNodeOption
{
	GENERATED_BODY()

	// 목적지 월드와 Waypoint를 함께 나타내는 키
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	FPRWaypointKey WaypointKey;

	// UI에 표시할 맵 이름
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	FText WorldDisplayName;

	// UI에 표시할 맵 이름
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	TSoftObjectPtr<UTexture2D> ThumbnailTexture;

	// UI에 표시할 웨이포인트 이름
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	FText WaypointDisplayName;

	// 저장 진행 상태 기준으로 활성화된 Waypoint 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	bool bUnlocked = false;

	// 개발 모드 예외로 Travel 가능한 Waypoint 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WaypointTravel")
	bool bDevTravelEnabled = false;
};
