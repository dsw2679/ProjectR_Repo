// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (BGM 레지스트리 데이터 에셋 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectR/Audio/PRBGMTypes.h"
#include "PRBGMRegistryDataAsset.generated.h"

// 레벨별 BGM Entry를 보관하는 Registry 에셋
UCLASS(BlueprintType)
class PROJECTR_API UPRBGMRegistryDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 레벨 식별자에 대응하는 BGM Entry 조회
	bool FindLevelEntry(FName LevelId, FPRLevelBGMEntry& OutEntry) const;

	// 상태에 대응하는 BGM Track 조회
	static bool TryGetTrackForState(const FPRLevelBGMEntry& Entry, EPRBGMState State, FPRBGMTrack& OutTrack);

public:
	// 레벨별 BGM Entry 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	TArray<FPRLevelBGMEntry> LevelEntries;
};
