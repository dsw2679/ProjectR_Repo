// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (BGM/사운드, 로딩 화면 및 성장/특성 밸런스 설정 정의)
// Author: 배유찬 (패스트 트래블, 핑/마커 및 VFX/어빌리티 설정 정의)
#include "PRDeveloperSettings.h"
#include "Blueprint/UserWidget.h"
#include "ProjectR/UI/FloatingText/PRFloatingTextWidget.h"
#include "ProjectR/UI/Crosshair/PRCrosshairConfig.h"
#include "ProjectR/Audio/PRBGMRegistryDataAsset.h"
#include "ProjectR/UI/PRUISoundDataAsset.h"
#include "ProjectR/World/PRWorldRegistry.h"

TSubclassOf<UUserWidget> UPRDeveloperSettings::GetCrosshairWidgetSync(EPRCrosshairType CrosshairType) const
{
	if (auto Found = CrosshairWidgets.Find(CrosshairType))
	{
		return Found->LoadSynchronous();
	}
	
	return nullptr;
}

const UPRCrosshairConfig* UPRDeveloperSettings::GetDefaultCrosshairConfigSync() const
{
	return DefaultCrosshairConfig.LoadSynchronous();
}

const UPRUISoundDataAsset* UPRDeveloperSettings::GetDefaultUISoundDataSync() const
{
	return DefaultUISoundData.LoadSynchronous();
}

const UPRBGMRegistryDataAsset* UPRDeveloperSettings::GetDefaultBGMRegistrySync() const
{
	return DefaultBGMRegistry.LoadSynchronous();
}

const UPRWorldRegistry* UPRDeveloperSettings::GetWorldRegistrySync() const
{
	return WorldRegistry.LoadSynchronous();
}

FPRFloatingTextStyle UPRDeveloperSettings::GetFloatingTextStyleSync(EPRFloatingTextType TextType) const
{
	if (const FPRFloatingTextStyleConfig* Found = FloatingTextStyles.Find(TextType))
	{
		FPRFloatingTextStyle Result;
		Result.WidgetClass = Found->WidgetClass.LoadSynchronous();
		Result.Color = Found->Color;
		Result.LayerZOrder = Found->LayerZOrder;
		return Result;
	}

	return FPRFloatingTextStyle();
}

FPRWorldMarkerVisualData UPRDeveloperSettings::GetWorldMarkerPreset(EPRWorldMarkerPreset InPresetType) const
{
	if (auto Found = WorldMarkerPresets.Find(InPresetType))
	{
		return *Found;
	}
	
	// Fallback: Default
	if (auto Found = WorldMarkerPresets.Find(EPRWorldMarkerPreset::Default))
	{
		return *Found;
	}
	
	// Fallback: Hard Coded Preset
	UE_LOG(LogTemp,Warning,TEXT("UPRDeveloperSettings: Default World Marker Preset이 설정되지 않음."));
	FPRWorldMarkerVisualData FallbackData;
	return FallbackData;
}
