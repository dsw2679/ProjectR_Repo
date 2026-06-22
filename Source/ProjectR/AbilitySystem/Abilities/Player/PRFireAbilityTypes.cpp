// Author: 배유찬 (플레이어 Fire 타입 어빌리티 구현)
#include "PRFireAbilityTypes.h"

bool FPRFireShotPayload::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	enum ERepFlag : uint8
	{
		REP_ShotOrigin = 1 << 0,
		REP_ShotDirection = 1 << 1,
		REP_ClientHitResult  = 1 << 2,
	};
		
	uint8 RepBits = 0;
		
	if (Ar.IsSaving())
	{
		if (!HasValidHitResult())
		{
			RepBits |= REP_ShotOrigin;
			RepBits |= REP_ShotDirection;
		}
		else
		{
			RepBits |= REP_ClientHitResult;
		}
	}
		
	Ar.SerializeBits(&RepBits,3);
		
	// 1. ShotId
	Ar << ShotID;
		
	// 2. ShotOrigin
	if (RepBits & REP_ShotOrigin)
	{
		ShotOrigin.NetSerialize(Ar,Map,bOutSuccess);
	}
	// 3. ShotDirection
	if (RepBits & REP_ShotDirection)
	{
		ShotDirection.NetSerialize(Ar,Map,bOutSuccess);
	}
	// 4. ClientHitResult
	if (RepBits & REP_ClientHitResult)
	{
		if (Ar.IsLoading())
		{
			// ClientHitResult는 이 시점에 NullPtr 가능
			if (!ClientHitResult.IsValid())
			{
				ClientHitResult = MakeShared<FHitResult>();
			}
		}
		ClientHitResult->NetSerialize(Ar, Map, bOutSuccess);
	}
	// 5. ClientTimestamp
	Ar << ClientTimestamp;
		
	bOutSuccess = true;
	return true;
}
