// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (월드 배치용 스폰 Point 및 관련 시스템 구현)
#include "PRSpawnPoint.h"

#include "GameFramework/PlayerStart.h"
#include "PRSpawnPointTags.h"

APRSpawnPoint::APRSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
}

FGameplayTag APRSpawnPoint::GetSpawnPointId() const
{
	return SpawnPointId.IsValid() ? SpawnPointId : PRSpawnPointTags::SpawnPoint_Default;
}

bool APRSpawnPoint::MatchesSpawnPointId(FGameplayTag InSpawnPointId) const
{
	// 빈 태그 기본값 보정
	const FGameplayTag TargetSpawnPointId = InSpawnPointId.IsValid() ? InSpawnPointId : PRSpawnPointTags::SpawnPoint_Default;
	return GetSpawnPointId().MatchesTagExact(TargetSpawnPointId);
}

APlayerStart* APRSpawnPoint::GetLinkedPlayerStart(int32 PlayerIndex) const
{
	if (LinkedPlayerStarts.IsValidIndex(PlayerIndex) && IsValid(LinkedPlayerStarts[PlayerIndex].Get()))
	{
		return LinkedPlayerStarts[PlayerIndex].Get();
	}

	if (PlayerIndex >= 0 && !LinkedPlayerStarts.IsEmpty())
	{
		const int32 FallbackIndex = PlayerIndex % LinkedPlayerStarts.Num();
		if (LinkedPlayerStarts.IsValidIndex(FallbackIndex) && IsValid(LinkedPlayerStarts[FallbackIndex].Get()))
		{
			return LinkedPlayerStarts[FallbackIndex].Get();
		}
	}

	for (const TObjectPtr<APlayerStart>& LinkedPlayerStart : LinkedPlayerStarts)
	{
		if (IsValid(LinkedPlayerStart.Get()))
		{
			// 첫 유효 PlayerStart 대체
			return LinkedPlayerStart.Get();
		}
	}

	return nullptr;
}

FTransform APRSpawnPoint::GetSpawnTransform(int32 PlayerIndex) const
{
	if (APlayerStart* LinkedPlayerStart = GetLinkedPlayerStart(PlayerIndex))
	{
		return LinkedPlayerStart->GetActorTransform();
	}

	// PlayerStart 미연결 대체
	UE_LOG(LogTemp, Warning, TEXT("SpawnPoint %s has no linked PlayerStart. Using spawn point transform."), *GetName());
	return GetActorTransform();
}
