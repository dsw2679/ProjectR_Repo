// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Perception 설정 클래스 정의)
#include "PRPerceptionConfig.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UPRPerceptionConfig::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (SightRadius < 0.0f)
	{
		Context.AddError(FText::FromString(TEXT("SightRadius는 0 이상이어야 합니다.")));
		Result = EDataValidationResult::Invalid;
	}

	if (LoseSightRadius < SightRadius)
	{
		Context.AddError(FText::FromString(TEXT("LoseSightRadius는 SightRadius 이상이어야 합니다.")));
		Result = EDataValidationResult::Invalid;
	}

	if (PeripheralVisionAngle < 0.0f || PeripheralVisionAngle > 180.0f)
	{
		Context.AddError(FText::FromString(TEXT("PeripheralVisionAngle은 0에서 180 사이여야 합니다.")));
		Result = EDataValidationResult::Invalid;
	}

	if (HearingRange < 0.0f)
	{
		Context.AddError(FText::FromString(TEXT("HearingRange는 0 이상이어야 합니다.")));
		Result = EDataValidationResult::Invalid;
	}

	if (StimulusMaxAge < 0.0f)
	{
		Context.AddError(FText::FromString(TEXT("StimulusMaxAge는 0 이상이어야 합니다.")));
		Result = EDataValidationResult::Invalid;
	}

	if (bPropagateAlertToNearbyEnemies && AlertPropagationRadius <= 0.0f)
	{
		Context.AddError(FText::FromString(TEXT("AlertPropagationRadius는 Alert 전파를 사용할 때 0보다 커야 합니다.")));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif
