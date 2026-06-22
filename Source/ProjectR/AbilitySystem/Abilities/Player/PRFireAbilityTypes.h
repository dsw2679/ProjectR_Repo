// Author: 배유찬 (플레이어 Fire 타입 어빌리티 구현)
#pragma once
#include "PRFireAbilityTypes.generated.h"

USTRUCT()
struct FPRFireShotPayload
{
	GENERATED_BODY()
	
	// 발사 시퀀스 Id, 발사 시작시 1부터 증가
	uint32 ShotID = 0;
	
	// 발사 시작 지점 (총구). ClientHit시에는 넷직렬화 x, HitResult의 TraceStart와 동일 취급
	FVector_NetQuantize ShotOrigin;
	
	// ClientHit시에는 넷직렬화 x (HitLocation-Origin으로 산출 가능)
	FVector_NetQuantize ShotDirection;
	
	// 클라측 히트 판정 정보
	TSharedPtr<FHitResult> ClientHitResult;
	
	// 발사 시점 클라이언트 시간
	float ClientTimestamp;
	
	bool IsValidShotID() const
	{
		return ShotID != 0;
	}
	
	bool HasValidHitResult() const
	{
		return ClientHitResult.IsValid() && ClientHitResult->GetActor() != nullptr;
	}
	
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPRFireShotPayload> : public TStructOpsTypeTraitsBase2<FPRFireShotPayload>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true		// TSharedPtr<FHitResult> Data 복사를 위해 필요
	};
};

USTRUCT()
struct FPRFireViewpoint
{
	GENERATED_BODY();
	
	FVector Location;
	FRotator Rotation;
};