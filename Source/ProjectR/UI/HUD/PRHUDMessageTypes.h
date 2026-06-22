// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (UI HUD Message 타입 및 구조체 정의)
#pragma once

#include "CoreMinimal.h"
#include "PRHUDMessageTypes.generated.h"

UENUM(BlueprintType)
enum class EPRHUDMessageType : uint8
{
	// HUD 안내 메시지 숨김
	None,

	// 상호작용 중인 플레이어가 다른 플레이어를 기다리는 상태
	WaitingForOtherPlayers,

	// 상호작용하지 않은 플레이어에게 다른 플레이어 대기 상태를 알리는 상태
	OtherPlayersWaiting,

	// 맵 이동 확정 이후 실제 이동을 기다리는 상태
	MapTravelInProgress,
};
