// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (월드 배치용 레지스트리 및 관련 시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "PRWorldRegistry.generated.h"

class UPRWorldDataAsset;
class UWorld;

// WorldId 기반 월드 데이터 조회와 Travel 목적지 검증을 담당하는 Registry
UCLASS(BlueprintType)
class PROJECTR_API UPRWorldRegistry : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 개발용 Waypoint Travel 허용 CVar 상태 반환
	static bool IsDevTravelEnabled();

	// 기본 월드 데이터 매핑 조회
	const TMap<FGameplayTag, TSoftObjectPtr<UPRWorldDataAsset>>& GetWorldDataById() const { return WorldDataById; }

	// 개발용 월드 데이터 매핑 조회
	const TMap<FGameplayTag, TSoftObjectPtr<UPRWorldDataAsset>>& GetDevWorldDataById() const { return DevWorldDataById; }

	// WorldId로 기본 월드 데이터 참조 조회
	bool TryGetWorldData(FGameplayTag WorldId, TSoftObjectPtr<UPRWorldDataAsset>& OutWorldDataAsset) const;

	// WorldId로 개발용 월드 데이터 참조 조회
	bool TryGetDevWorldData(FGameplayTag WorldId, TSoftObjectPtr<UPRWorldDataAsset>& OutWorldDataAsset) const;

	// 현재 월드 패키지와 매칭되는 WorldId 조회
	bool FindWorldIdByMap(UWorld* World, FGameplayTag& OutWorldId) const;

	// 기본 월드 등록 여부 조회
	bool IsWorldIdRegistered(FGameplayTag WorldId) const;

	// 개발용 월드 등록 여부 조회
	bool IsDevWorldIdRegistered(FGameplayTag WorldId) const;

	// WorldId와 WaypointId 조합의 등록 여부 조회
	bool HasWaypoint(const FPRWaypointKey& WaypointKey) const;

	// Travel 실행에 필요한 맵 에셋 조회
	bool ResolveMapAsset(FGameplayTag WorldId, TSoftObjectPtr<UWorld>& OutMapAsset) const;

	// WorldId에 해당하는 월드 데이터를 동기 로드
	UPRWorldDataAsset* LoadWorldDataSync(FGameplayTag WorldId, bool bAllowDevWorldData) const;

protected:
	// 기본 월드 데이터 매핑
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WorldRegistry")
	TMap<FGameplayTag, TSoftObjectPtr<UPRWorldDataAsset>> WorldDataById;

	// CVar가 켜졌을 때만 허용되는 개발용 월드 데이터 매핑
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WorldRegistry")
	TMap<FGameplayTag, TSoftObjectPtr<UPRWorldDataAsset>> DevWorldDataById;
};
