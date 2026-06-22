// Copyright Epic Games, Inc. All Rights Reserved.
// Author: 배유찬 (게임 모듈 초기화 및 공용 충돌 채널/스텐실 값 정의)
// Author: 손승우 (적 AI 전용 충돌 채널 정의)
// Author: 이건주 (몬스터 채널 정의)
#pragma once

namespace PRRowNames
{
	namespace Player
	{
		const FName Default = "Default";	
	}
	
	namespace Enemies
	{
		const FName Faerin = "Faerin";
		// TODO: Add Enemy Names
	}
}

namespace PREnemyHealthScaling
{
	// 추가 플레이어 1명당 BaseHP에 곱해지는 체력 증가 상수
	inline constexpr float EnemyHealthScaleConstant = 1.1f;

	// 플레이어 수 기반 적 체력 배율 계산
	inline float CalculateHealthScale(int32 PlayerCount)
	{
		const int32 AdditionalPlayerCount = PlayerCount > 1 ? PlayerCount - 1 : 0;
		return 1.0f + EnemyHealthScaleConstant * static_cast<float>(AdditionalPlayerCount);
	}
}

namespace PRAmmoPickupScaling
{
	// 추가 플레이어 1명당 기본 탄약 획득량에 곱해지는 증가 상수
	inline constexpr float AmmoPickupScaleConstant = 0.25f;

	// 플레이어 수 기반 탄약 획득량 계산
	inline int32 CalculateAmmoQuantity(int32 BaseQuantity, int32 PlayerCount)
	{
		if (BaseQuantity <= 0)
		{
			return 0;
		}

		const int32 AdditionalPlayerCount = PlayerCount > 1 ? PlayerCount - 1 : 0;
		const float QuantityScale = 1.0f + AmmoPickupScaleConstant * static_cast<float>(AdditionalPlayerCount);
		return FMath::Max(FMath::RoundToInt(static_cast<float>(BaseQuantity) * QuantityScale), BaseQuantity);
	}
}

namespace PRCollisionChannels
{
	constexpr ECollisionChannel ECC_Combat = ECC_GameTraceChannel1;
	constexpr ECollisionChannel ECC_Projectile = ECC_GameTraceChannel2;
	constexpr ECollisionChannel ECC_Ground = ECC_GameTraceChannel3;
	constexpr ECollisionChannel ECC_Interactable = ECC_GameTraceChannel4;
	constexpr ECollisionChannel ECC_PingMarker = ECC_GameTraceChannel5;
	constexpr ECollisionChannel ECC_AISight = ECC_GameTraceChannel6;
	constexpr ECollisionChannel ECC_EnemyProjectile = ECC_GameTraceChannel7;
}

namespace PRStencilValues
{
	constexpr int32 Default = 0;
	constexpr int32 Highlight= 1;
	constexpr int32 Interaction = 2;
	constexpr int32 Friendly = 3;
	constexpr int32 Hostile = 4;
}
