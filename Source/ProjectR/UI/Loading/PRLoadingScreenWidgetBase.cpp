// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Loading Screen 기본 구조 UI 위젯 구현)
#include "PRLoadingScreenWidgetBase.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Image.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"

void UPRLoadingScreenWidgetBase::SetLoadingDestination(const FString& MapPackageName)
{
	DestinationMapPackageName = MapPackageName;
	DestinationMapShortName = ResolveShortMapName(MapPackageName);

	const FName PackageName(*DestinationMapPackageName);
	const FPRLoadingScreenImageEntry* ImageEntry = FindImageEntry(PackageName, DestinationMapShortName);
	ApplyImageBrushes(ImageEntry);
	OnLoadingDestinationChanged(DestinationMapPackageName, DestinationMapShortName);
}

void UPRLoadingScreenWidgetBase::PlayFadeInAnimation()
{
	if (IsValid(FadeIn))
	{
		PlayAnimation(FadeIn, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
	}
}

void UPRLoadingScreenWidgetBase::PlayFadeOutAnimation()
{
	if (IsValid(FadeOut))
	{
		PlayAnimation(FadeOut, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
	}
}

const FPRLoadingScreenImageEntry* UPRLoadingScreenWidgetBase::FindImageEntry(FName MapPackageName, FName MapShortName) const
{
	for (const FPRLoadingScreenImageEntry& ImageEntry : MapImageEntries)
	{
		if (ImageEntry.MapName == MapPackageName || ImageEntry.MapName == MapShortName)
		{
			return &ImageEntry;
		}
	}

	return nullptr;
}

void UPRLoadingScreenWidgetBase::ApplyImageBrushes(const FPRLoadingScreenImageEntry* ImageEntry)
{
	const FSlateBrush& PrimaryBrush = ImageEntry ? ImageEntry->PrimaryImageBrush : DefaultPrimaryImageBrush;
	const FSlateBrush& SecondaryBrush = ImageEntry
		? ImageEntry->ViewportSecondaryImageBrush
		: DefaultViewportSecondaryImageBrush;

	if (IsValid(PrimaryImage))
	{
		PrimaryImage->SetBrush(PrimaryBrush);
	}

	if (IsValid(SecondaryImage))
	{
		SecondaryImage->SetBrush(SecondaryBrush);
	}
}

FName UPRLoadingScreenWidgetBase::ResolveShortMapName(const FString& MapPackageName) const
{
	FString NormalizedMapName = UWorld::RemovePIEPrefix(MapPackageName);
	int32 ObjectNameDelimiterIndex = INDEX_NONE;
	if (NormalizedMapName.FindChar(TEXT('.'), ObjectNameDelimiterIndex))
	{
		NormalizedMapName.LeftInline(ObjectNameDelimiterIndex);
	}

	return FName(*FPackageName::GetShortName(NormalizedMapName));
}
