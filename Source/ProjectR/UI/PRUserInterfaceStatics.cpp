// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (크로스헤어 및 사격 UI 정적 호출 구현)
// Author: 이건주 (무기 상태 및 드론 전용 UI 정적 호출 구현)
#include "PRUserInterfaceStatics.h"

#include "ProjectR/System/PRDeveloperSettings.h"

TSubclassOf<UUserWidget> UPRUserInterfaceStatics::GetCrosshairWidgetClass(const EPRCrosshairType CrosshairType)
{
	if (const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>())
	{
		return Settings->GetCrosshairWidgetSync(CrosshairType);
	}
	return nullptr;
}

FText UPRUserInterfaceStatics::ConvertFloatToText(float Value, int32 MaximumFractionalDigits)
{
	FNumberFormattingOptions FormattingOptions;
	FormattingOptions.MaximumFractionalDigits = FMath::Max(MaximumFractionalDigits, 0);
	FormattingOptions.MinimumFractionalDigits = 0;
	return FText::AsNumber(Value, &FormattingOptions);
}

float UPRUserInterfaceStatics::ConvertMinMaxToPercent(float CurrentValue, float MaxValue, float MinValue)
{
	if (MaxValue <= MinValue)
	{
		return 0.0f;
	}

	return FMath::Clamp((CurrentValue - MinValue) / (MaxValue - MinValue), 0.0f, 1.0f);
}
