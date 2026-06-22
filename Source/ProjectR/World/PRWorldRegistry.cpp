// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (월드 배치용 레지스트리 및 관련 시스템 구현)
#include "PRWorldRegistry.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "ProjectR/World/PRWorldDataAsset.h"

namespace
{
	TAutoConsoleVariable<int32> CVarPRWaypointTravelDevMode(
		TEXT("pr.WaypointTravel.DevMode"),
		0,
		TEXT("Enables Waypoint Travel access to development-only worlds."),
		ECVF_Default);

	FString NormalizePIEPackageShortName(FString ShortName)
	{
		static const FString PIEPrefix = TEXT("UEDPIE_");
		if (!ShortName.StartsWith(PIEPrefix))
		{
			return ShortName;
		}

		for (int32 Index = PIEPrefix.Len(); Index < ShortName.Len(); ++Index)
		{
			if (ShortName[Index] == TCHAR('_'))
			{
				return ShortName.RightChop(Index + 1);
			}
		}

		return ShortName;
	}

	bool DoesWorldDataMatchWorld(const UPRWorldDataAsset* WorldDataAsset, const UWorld* World)
	{
		if (!IsValid(WorldDataAsset) || !IsValid(World) || WorldDataAsset->MapAsset.IsNull())
		{
			return false;
		}

		const FString TargetPackageName = WorldDataAsset->MapAsset.ToSoftObjectPath().GetLongPackageName();
		const FString CurrentPackageName = World->GetPackage()->GetName();
		if (TargetPackageName == CurrentPackageName)
		{
			return true;
		}

		const FString TargetShortName = FPackageName::GetShortName(TargetPackageName);
		const FString CurrentShortName = NormalizePIEPackageShortName(FPackageName::GetShortName(CurrentPackageName));
		return TargetShortName == CurrentShortName;
	}

	bool IsWorldDataKeyValid(FGameplayTag WorldId, const UPRWorldDataAsset* WorldDataAsset)
	{
		return WorldId.IsValid()
			&& IsValid(WorldDataAsset)
			&& WorldDataAsset->WorldId.MatchesTagExact(WorldId);
	}
}

bool UPRWorldRegistry::IsDevTravelEnabled()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return CVarPRWaypointTravelDevMode.GetValueOnGameThread() != 0;
#endif
}

bool UPRWorldRegistry::TryGetWorldData(FGameplayTag WorldId, TSoftObjectPtr<UPRWorldDataAsset>& OutWorldDataAsset) const
{
	OutWorldDataAsset.Reset();
	if (!WorldId.IsValid())
	{
		return false;
	}

	const TSoftObjectPtr<UPRWorldDataAsset>* FoundWorldData = WorldDataById.Find(WorldId);
	if (!FoundWorldData)
	{
		return false;
	}

	OutWorldDataAsset = *FoundWorldData;
	return !OutWorldDataAsset.IsNull();
}

bool UPRWorldRegistry::TryGetDevWorldData(FGameplayTag WorldId, TSoftObjectPtr<UPRWorldDataAsset>& OutWorldDataAsset) const
{
	OutWorldDataAsset.Reset();
	if (!WorldId.IsValid())
	{
		return false;
	}

	const TSoftObjectPtr<UPRWorldDataAsset>* FoundWorldData = DevWorldDataById.Find(WorldId);
	if (!FoundWorldData)
	{
		return false;
	}

	OutWorldDataAsset = *FoundWorldData;
	return !OutWorldDataAsset.IsNull();
}

bool UPRWorldRegistry::FindWorldIdByMap(UWorld* World, FGameplayTag& OutWorldId) const
{
	OutWorldId = FGameplayTag();
	if (!IsValid(World))
	{
		return false;
	}

	for (const TPair<FGameplayTag, TSoftObjectPtr<UPRWorldDataAsset>>& Pair : WorldDataById)
	{
		UPRWorldDataAsset* WorldDataAsset = Pair.Value.LoadSynchronous();
		if (!IsWorldDataKeyValid(Pair.Key, WorldDataAsset))
		{
			continue;
		}

		if (DoesWorldDataMatchWorld(WorldDataAsset, World))
		{
			OutWorldId = Pair.Key;
			return true;
		}
	}

	if (!IsDevTravelEnabled())
	{
		return false;
	}

	for (const TPair<FGameplayTag, TSoftObjectPtr<UPRWorldDataAsset>>& Pair : DevWorldDataById)
	{
		UPRWorldDataAsset* WorldDataAsset = Pair.Value.LoadSynchronous();
		if (!IsWorldDataKeyValid(Pair.Key, WorldDataAsset))
		{
			continue;
		}

		if (DoesWorldDataMatchWorld(WorldDataAsset, World))
		{
			OutWorldId = Pair.Key;
			return true;
		}
	}

	return false;
}

bool UPRWorldRegistry::IsWorldIdRegistered(FGameplayTag WorldId) const
{
	return WorldId.IsValid() && WorldDataById.Contains(WorldId);
}

bool UPRWorldRegistry::IsDevWorldIdRegistered(FGameplayTag WorldId) const
{
	return WorldId.IsValid() && DevWorldDataById.Contains(WorldId);
}

bool UPRWorldRegistry::HasWaypoint(const FPRWaypointKey& WaypointKey) const
{
	UPRWorldDataAsset* WorldDataAsset = LoadWorldDataSync(WaypointKey.WorldId, IsDevTravelEnabled());
	if (!IsWorldDataKeyValid(WaypointKey.WorldId, WorldDataAsset))
	{
		return false;
	}

	return WorldDataAsset->HasWaypointNode(WaypointKey.WaypointId);
}

bool UPRWorldRegistry::ResolveMapAsset(FGameplayTag WorldId, TSoftObjectPtr<UWorld>& OutMapAsset) const
{
	OutMapAsset.Reset();

	UPRWorldDataAsset* WorldDataAsset = LoadWorldDataSync(WorldId, IsDevTravelEnabled());
	if (!IsWorldDataKeyValid(WorldId, WorldDataAsset) || WorldDataAsset->MapAsset.IsNull())
	{
		return false;
	}

	OutMapAsset = WorldDataAsset->MapAsset;
	return true;
}

UPRWorldDataAsset* UPRWorldRegistry::LoadWorldDataSync(FGameplayTag WorldId, bool bAllowDevWorldData) const
{
	TSoftObjectPtr<UPRWorldDataAsset> WorldDataAssetPtr;
	if (TryGetWorldData(WorldId, WorldDataAssetPtr))
	{
		return WorldDataAssetPtr.LoadSynchronous();
	}

	if (bAllowDevWorldData && TryGetDevWorldData(WorldId, WorldDataAssetPtr))
	{
		return WorldDataAssetPtr.LoadSynchronous();
	}

	return nullptr;
}
