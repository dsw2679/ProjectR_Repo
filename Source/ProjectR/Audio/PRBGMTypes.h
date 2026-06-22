// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (BGM 타입 및 구조체 정의)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PRBGMTypes.generated.h"

class USoundBase;

UENUM(BlueprintType)
enum class EPRBGMState : uint8
{
	None,
	Hub,
	Exploration,
	Combat,
	BossPhase1 UMETA(DisplayName = "Boss Phase 1"),
	BossPhase2 UMETA(DisplayName = "Boss Phase 2"),
	BossPhase3 UMETA(DisplayName = "Boss Phase 3"),
	BossPhase4 UMETA(DisplayName = "Boss Phase 4"),
	Victory,
	BossIntroCutscene UMETA(DisplayName = "Boss Intro Cutscene"),
	BossDialogue UMETA(DisplayName = "Boss Dialogue"),
	BossFightStartCutscene UMETA(DisplayName = "Boss Fight Start Cutscene")
};

UENUM(BlueprintType)
enum class EPRBGMSection : uint8
{
	Loop,
	Intro,
	Outro
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRBGMPatternCue
{
	GENERATED_BODY()

	// 특수패턴을 식별할 GameplayTag
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	FGameplayTag CueTag;

	// LoopSound를 다시 시작할 재생 위치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM", meta = (ClampMin = "0.0", Units = "s"))
	float StartTime = 0.0f;

	// 특수패턴 BGM 진입 페이드 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM", meta = (ClampMin = "0.0", Units = "s"))
	float FadeInDuration = 0.1f;

	// 기존 BGM 페이드아웃 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM", meta = (ClampMin = "0.0", Units = "s"))
	float FadeOutDuration = 0.1f;

	// 같은 사운드가 재생 중이어도 지정 위치부터 재시작할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	bool bForceRestartSameSound = true;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRBGMTrack
{
	GENERATED_BODY()

	// 재생할 BGM 사운드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	TSoftObjectPtr<USoundBase> Sound;

	// Intro 구간 사운드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	TSoftObjectPtr<USoundBase> IntroSound;

	// Loop 구간 사운드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	TSoftObjectPtr<USoundBase> LoopSound;

	// Outro 구간 사운드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	TSoftObjectPtr<USoundBase> OutroSound;

	// 새 BGM 페이드인 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM", meta = (ClampMin = "0.0", Units = "s"))
	float FadeInDuration = 1.0f;

	// 기존 BGM 페이드아웃 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM", meta = (ClampMin = "0.0", Units = "s"))
	float FadeOutDuration = 1.0f;

	// 트랙별 볼륨 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM", meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	// 재생 시작 위치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM", meta = (ClampMin = "0.0", Units = "s"))
	float StartTime = 0.0f;

	// 특수패턴 시작 시 LoopSound를 특정 위치부터 재시작하는 Cue 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	TArray<FPRBGMPatternCue> PatternCues;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRLevelBGMEntry
{
	GENERATED_BODY()

	// 월드 패키지 이름 기준 레벨 식별자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	FName LevelId = NAME_None;

	// 레벨 진입 시 기본 BGM 상태
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	EPRBGMState DefaultState = EPRBGMState::Exploration;

	// 거점 BGM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	FPRBGMTrack HubTrack;

	// 탐색 BGM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	FPRBGMTrack ExplorationTrack;

	// 일반 전투 BGM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	FPRBGMTrack CombatTrack;

	// 보스 최초 조우 컷신 BGM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	FPRBGMTrack BossIntroCutsceneTrack;

	// 보스 대화 상호작용 BGM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	FPRBGMTrack BossDialogueTrack;

	// 보스 전투 시작 컷신 BGM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	FPRBGMTrack BossFightStartCutsceneTrack;

	// 보스 1페이즈 BGM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	FPRBGMTrack BossPhase1Track;

	// 보스 2페이즈 BGM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	FPRBGMTrack BossPhase2Track;

	// 보스 3페이즈 BGM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	FPRBGMTrack BossPhase3Track;

	// 보스 4페이즈 BGM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	FPRBGMTrack BossPhase4Track;

	// 승리 연출 BGM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Audio|BGM")
	FPRBGMTrack VictoryTrack;
};
