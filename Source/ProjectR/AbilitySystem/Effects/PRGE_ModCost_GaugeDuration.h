// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 이건주 (Mod Cost Gauge Duration 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "PRGE_ModCost_GaugeDuration.generated.h"

// 지속시간형 Mod 게이지 비용을 주기적으로 차감하고 슬롯별 게이지 잠금 태그를 유지하는 GE
UCLASS()
class PROJECTR_API UPRGE_ModCost_GaugeDuration : public UGameplayEffect
{
	GENERATED_BODY()

public:
	// 지속시간형 Mod 비용 GE 기본값을 설정한다
	UPRGE_ModCost_GaugeDuration();
};
