// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (기본 투사체 동기화 및 예측 바운스 연산 타입 정의)
// Author: 손승우 (보스 전용 투사체 타입 정의)
// Author: 이건주 (드론/힐링샷 및 Penitent 투사체 타입 정의)
#include "PRProjectileTypes.h"

#include "GameFramework/Actor.h"

bool FPRProjectileRepHomingSchedule::IsEnabled() const
{
	return HomingTargetActor != nullptr && HomingAcceleration > 0.0f;
}

void FPRProjectileRepHomingSchedule::Reset()
{
	HomingTargetActor = nullptr;
	HomingAcceleration = 0.0f;
	StartDelay = 0.0f;
	Duration = 0.0f;
	Revision = 0;
}

bool PRShouldPlayProjectileDestroyEffect(EPRProjectileDestroyReason DestroyReason)
{
	// 파괴 연출 대상
	return DestroyReason == EPRProjectileDestroyReason::Impact
		|| DestroyReason == EPRProjectileDestroyReason::DamageDepleted
		|| DestroyReason == EPRProjectileDestroyReason::ReplicatedDetonation;
}

bool FPRProjectileRepMovement::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	uint8 EventByte = static_cast<uint8>(Event);
	// 이동 이벤트 6종 인코딩
	Ar.SerializeBits(&EventByte, 3);
	Event = static_cast<EPRRepMovementEvent>(EventByte);

	bool bLocationSuccess = true;
	bool bVelocitySuccess = true;
	bool bRotationSuccess = true;
	Location.NetSerialize(Ar, Map, bLocationSuccess);
	Velocity.NetSerialize(Ar, Map, bVelocitySuccess);
	Rotation.NetSerialize(Ar, Map, bRotationSuccess);
	// Bounce 이벤트 비교 식별자
	Ar.SerializeBits(&BounceIndex, 8);

	uint8 bHasHomingSchedule = Event == EPRRepMovementEvent::Spawn && HomingSchedule.IsEnabled() ? 1 : 0;
	Ar.SerializeBits(&bHasHomingSchedule, 1);
	bool bHomingTargetMapped = true;
	if (bHasHomingSchedule != 0)
	{
		UObject* HomingTargetObject = HomingSchedule.HomingTargetActor.Get();
		bHomingTargetMapped = Map == nullptr
			|| Map->SerializeObject(Ar, AActor::StaticClass(), HomingTargetObject);
		if (Ar.IsLoading())
		{
			HomingSchedule.HomingTargetActor = Cast<AActor>(HomingTargetObject);
		}

		Ar << HomingSchedule.HomingAcceleration;
		Ar << HomingSchedule.StartDelay;
		Ar << HomingSchedule.Duration;
		Ar.SerializeBits(&HomingSchedule.Revision, 8);
	}
	else if (Ar.IsLoading())
	{
		HomingSchedule.Reset();
	}

	bOutSuccess = bLocationSuccess && bVelocitySuccess && bRotationSuccess && bHomingTargetMapped;
	return true;
}
