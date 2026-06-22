// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (BGM 레지스트리 데이터 에셋 구현)
#include "PRBGMRegistryDataAsset.h"

bool UPRBGMRegistryDataAsset::FindLevelEntry(FName LevelId, FPRLevelBGMEntry& OutEntry) const
{
	if (LevelId.IsNone())
	{
		return false;
	}

	for (const FPRLevelBGMEntry& Entry : LevelEntries)
	{
		if (Entry.LevelId == LevelId)
		{
			OutEntry = Entry;
			return true;
		}
	}

	return false;
}

bool UPRBGMRegistryDataAsset::TryGetTrackForState(const FPRLevelBGMEntry& Entry, EPRBGMState State, FPRBGMTrack& OutTrack)
{
	switch (State)
	{
	case EPRBGMState::Hub:
		OutTrack = Entry.HubTrack;
		return true;
	case EPRBGMState::Exploration:
		OutTrack = Entry.ExplorationTrack;
		return true;
	case EPRBGMState::Combat:
		OutTrack = Entry.CombatTrack;
		return true;
	case EPRBGMState::BossIntroCutscene:
		OutTrack = Entry.BossIntroCutsceneTrack;
		return true;
	case EPRBGMState::BossDialogue:
		OutTrack = Entry.BossDialogueTrack;
		return true;
	case EPRBGMState::BossFightStartCutscene:
		OutTrack = Entry.BossFightStartCutsceneTrack;
		return true;
	case EPRBGMState::BossPhase1:
		OutTrack = Entry.BossPhase1Track;
		return true;
	case EPRBGMState::BossPhase2:
		OutTrack = Entry.BossPhase2Track;
		return true;
	case EPRBGMState::BossPhase3:
		OutTrack = Entry.BossPhase3Track;
		return true;
	case EPRBGMState::BossPhase4:
		OutTrack = Entry.BossPhase4Track;
		return true;
	case EPRBGMState::Victory:
		OutTrack = Entry.VictoryTrack;
		return true;
	case EPRBGMState::None:
	default:
		return false;
	}
}
