// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Entrance 보스 입장 게이트 상호작용 액션 실행 로직 구현)
#include "PRInteraction_EntranceGate.h"

#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "ProjectR/World/PRWorldRegistry.h"

UPRInteraction_EntranceGate::UPRInteraction_EntranceGate()
{
	// 입장 게이트 전원 상호작용 기본 설정
}

void UPRInteraction_EntranceGate::HandlePartySyncConditionMet()
{
	TSoftObjectPtr<UWorld> ResolvedMapAsset;
	FGameplayTag TargetWaypoint;
	if (!ResolveTargetTravelData(ResolvedMapAsset, TargetWaypoint))
	{
		return;
	}

	// Registry에서 해석된 목적지 맵과 Waypoint 기준 이동
	StartTravelToSpawnPoint(ResolvedMapAsset, TargetWaypoint, FadeDuration);
}

bool UPRInteraction_EntranceGate::ResolveTargetTravelData(TSoftObjectPtr<UWorld>& OutResolvedMapAsset, FGameplayTag& OutTargetWaypointId) const
{
	OutResolvedMapAsset.Reset();
	OutTargetWaypointId = FGameplayTag();

	FPRWaypointKey TargetWaypointKey;
	TargetWaypointKey.WorldId = TargetWorldId;
	TargetWaypointKey.WaypointId = TargetWaypointId;
	if (!TargetWaypointKey.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("EntranceGate travel rejected: invalid target data WorldId=%s WaypointId=%s"),
			*TargetWorldId.ToString(),
			*TargetWaypointId.ToString());
		return false;
	}

	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	const UPRWorldRegistry* WorldRegistry = IsValid(Settings) ? Settings->GetWorldRegistrySync() : nullptr;
	if (!IsValid(WorldRegistry))
	{
		UE_LOG(LogTemp, Warning, TEXT("EntranceGate travel rejected: world registry is not configured"));
		return false;
	}

	const bool bRegisteredWorld = WorldRegistry->IsWorldIdRegistered(TargetWaypointKey.WorldId);
	const bool bRegisteredDevWorld = WorldRegistry->IsDevWorldIdRegistered(TargetWaypointKey.WorldId);
	if (!bRegisteredWorld && !bRegisteredDevWorld)
	{
		UE_LOG(LogTemp, Warning, TEXT("EntranceGate travel rejected: world id is not registered %s"),
			*TargetWaypointKey.WorldId.ToString());
		return false;
	}

	if (!bRegisteredWorld && !UPRWorldRegistry::IsDevTravelEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("EntranceGate travel rejected: dev world travel is disabled %s"),
			*TargetWaypointKey.WorldId.ToString());
		return false;
	}

	if (!WorldRegistry->HasWaypoint(TargetWaypointKey))
	{
		UE_LOG(LogTemp, Warning, TEXT("EntranceGate travel rejected: waypoint node not found %s"),
			*TargetWaypointKey.WaypointId.ToString());
		return false;
	}

	if (!WorldRegistry->ResolveMapAsset(TargetWaypointKey.WorldId, OutResolvedMapAsset) || OutResolvedMapAsset.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("EntranceGate travel rejected: target map is not resolved %s"),
			*TargetWaypointKey.WorldId.ToString());
		return false;
	}

	OutTargetWaypointId = TargetWaypointKey.WaypointId;
	return true;
}
