// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (BGM 서브시스템 구현)
#include "PRBGMSubsystem.h"

#include "AbilitySystemComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Audio/PRBGMRegistryDataAsset.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace
{
	EPRBGMState MapBossPhaseToBGMState(EPRBossPhase BossPhase)
	{
		switch (BossPhase)
		{
		case EPRBossPhase::Phase1:
			return EPRBGMState::BossPhase1;
		case EPRBossPhase::Phase2:
			return EPRBGMState::BossPhase2;
		case EPRBossPhase::Phase3:
			return EPRBGMState::BossPhase3;
		case EPRBossPhase::Phase4:
			return EPRBGMState::BossPhase4;
		default:
			return EPRBGMState::BossPhase1;
		}
	}
}

/*~ UWorldSubsystem Interface ~*/

void UPRBGMSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	BindBossBGMEvents();
}

void UPRBGMSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	BindBossBGMEvents();
	StartLevelBGM();
	BindLocalPlayerCombatTag();
}

void UPRBGMSubsystem::Deinitialize()
{
	UnbindLocalPlayerCombatTag();
	UnbindBossBGMEvents();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CombatBGMReleaseTimerHandle);
	}
	StopLocalPlayerCombatTagBindRetry();

	StopBGM(0.0f);
	for (TWeakObjectPtr<UAudioComponent>& WeakAudioComponent : FadingOutAudioComponents)
	{
		if (UAudioComponent* AudioComponent = WeakAudioComponent.Get())
		{
			AudioComponent->Stop();
			AudioComponent->DestroyComponent();
		}
	}
	FadingOutAudioComponents.Empty();

	Super::Deinitialize();
}

/*~ BGM 제어 ~*/

void UPRBGMSubsystem::StartLevelBGM()
{
	if (!TryCacheCurrentLevelEntry())
	{
		return;
	}

	SetBGMState(CurrentLevelEntry.DefaultState);
}

void UPRBGMSubsystem::StopBGM(float FadeOutDuration)
{
	if (IsValid(CurrentAudioComponent))
	{
		ClearCurrentAudioFinishedBinding(CurrentAudioComponent);
		FadeOutAndDestroyAudioComponent(CurrentAudioComponent, FadeOutDuration);
	}

	CurrentAudioComponent = nullptr;
	CurrentSound = nullptr;
	CurrentState = EPRBGMState::None;
	CurrentSection = EPRBGMSection::Loop;
}

void UPRBGMSubsystem::SetBGMState(EPRBGMState NewState)
{
	PlayBGMStateSection(NewState, EPRBGMSection::Loop, false);
}

void UPRBGMSubsystem::PlayBGMStateSection(EPRBGMState NewState, EPRBGMSection Section, bool bForceRestartSameSound)
{
	if (NewState == EPRBGMState::None)
	{
		StopBGM(0.0f);
		return;
	}

	if (CurrentState == EPRBGMState::Victory && NewState != EPRBGMState::Victory)
	{
		return;
	}

	if (IsBossPhaseState(CurrentState) && !IsBossPhaseState(NewState) && NewState != EPRBGMState::Victory)
	{
		return;
	}

	if (!bHasCurrentLevelEntry && !TryCacheCurrentLevelEntry())
	{
		return;
	}

	FPRBGMTrack Track;
	if (!UPRBGMRegistryDataAsset::TryGetTrackForState(CurrentLevelEntry, NewState, Track))
	{
		return;
	}

	USoundBase* Sound = LoadTrackSound(Track, Section);
	if (!IsValid(Sound))
	{
		return;
	}

	if (CurrentState == NewState && CurrentSection == Section && IsValid(CurrentAudioComponent) && !bForceRestartSameSound)
	{
		return;
	}

	if (CurrentSound.Get() == Sound && CurrentSection == Section && IsValid(CurrentAudioComponent) && !bForceRestartSameSound)
	{
		CurrentState = NewState;
		CurrentSection = Section;
		return;
	}

	PlayTrack(NewState, Track, Section, Track.StartTime, Track.FadeInDuration, Track.FadeOutDuration, bForceRestartSameSound);
}

void UPRBGMSubsystem::PlayPatternCue(FGameplayTag CueTag)
{
	if (!CueTag.IsValid())
	{
		return;
	}

	if (!bHasCurrentLevelEntry && !TryCacheCurrentLevelEntry())
	{
		return;
	}

	FPRBGMTrack Track;
	if (!UPRBGMRegistryDataAsset::TryGetTrackForState(CurrentLevelEntry, CurrentState, Track))
	{
		return;
	}

	for (const FPRBGMPatternCue& PatternCue : Track.PatternCues)
	{
		if (!PatternCue.CueTag.MatchesTagExact(CueTag))
		{
			continue;
		}

		PlayTrack(
			CurrentState,
			Track,
			EPRBGMSection::Loop,
			PatternCue.StartTime,
			PatternCue.FadeInDuration,
			PatternCue.FadeOutDuration,
			PatternCue.bForceRestartSameSound);
		return;
	}
}

void UPRBGMSubsystem::RestoreDefaultLevelBGM()
{
	if (IsBossPhaseState(CurrentState) || CurrentState == EPRBGMState::Victory)
	{
		return;
	}

	if (!bHasCurrentLevelEntry && !TryCacheCurrentLevelEntry())
	{
		return;
	}

	SetBGMState(CurrentLevelEntry.DefaultState);
}

void UPRBGMSubsystem::ResetToLevelBGM(float FadeOutDuration)
{
	StopBGM(FMath::Max(FadeOutDuration, 0.0f));
	StartLevelBGM();
}

/*~ 레벨 데이터 ~*/

FName UPRBGMSubsystem::ResolveCurrentLevelId() const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return NAME_None;
	}

	const FString CleanMapName = UWorld::RemovePIEPrefix(World->GetMapName());
	return FName(*CleanMapName);
}

bool UPRBGMSubsystem::TryCacheCurrentLevelEntry()
{
	CurrentLevelId = ResolveCurrentLevelId();
	bHasCurrentLevelEntry = false;

	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (!IsValid(Settings))
	{
		return false;
	}

	const UPRBGMRegistryDataAsset* Registry = Settings->GetDefaultBGMRegistrySync();
	if (!IsValid(Registry))
	{
		return false;
	}

	if (!Registry->FindLevelEntry(CurrentLevelId, CurrentLevelEntry))
	{
		return false;
	}

	bHasCurrentLevelEntry = true;
	return true;
}

USoundBase* UPRBGMSubsystem::LoadTrackSound(const FPRBGMTrack& Track, EPRBGMSection Section) const
{
	switch (Section)
	{
	case EPRBGMSection::Intro:
		if (!Track.IntroSound.IsNull())
		{
			return Track.IntroSound.LoadSynchronous();
		}
		break;
	case EPRBGMSection::Outro:
		if (!Track.OutroSound.IsNull())
		{
			return Track.OutroSound.LoadSynchronous();
		}
		return Track.Sound.LoadSynchronous();
	case EPRBGMSection::Loop:
	default:
		break;
	}

	if (!Track.LoopSound.IsNull())
	{
		return Track.LoopSound.LoadSynchronous();
	}

	return Track.Sound.LoadSynchronous();
}

/*~ 오디오 재생 ~*/

bool UPRBGMSubsystem::PlayTrack(EPRBGMState NewState, const FPRBGMTrack& Track, EPRBGMSection Section, float StartTime, float FadeInDuration, float FadeOutDuration, bool bForceRestartSameSound)
{
	USoundBase* Sound = LoadTrackSound(Track, Section);
	if (!IsValid(Sound))
	{
		return false;
	}

	if (CurrentSound.Get() == Sound && CurrentSection == Section && IsValid(CurrentAudioComponent) && !bForceRestartSameSound)
	{
		CurrentState = NewState;
		CurrentSection = Section;
		return true;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	UAudioComponent* PreviousAudioComponent = CurrentAudioComponent;
	ClearCurrentAudioFinishedBinding(PreviousAudioComponent);
	CurrentAudioComponent = UGameplayStatics::SpawnSound2D(
		World,
		Sound,
		FMath::Max(Track.VolumeMultiplier, 0.0f),
		1.0f,
		FMath::Max(StartTime, 0.0f),
		nullptr,
		false,
		false);

	if (!IsValid(CurrentAudioComponent))
	{
		CurrentAudioComponent = PreviousAudioComponent;
		return false;
	}

	CurrentAudioComponent->FadeIn(
		FMath::Max(FadeInDuration, 0.0f),
		FMath::Max(Track.VolumeMultiplier, 0.0f),
		FMath::Max(StartTime, 0.0f));

	if (IsValid(PreviousAudioComponent))
	{
		FadeOutAndDestroyAudioComponent(PreviousAudioComponent, FadeOutDuration);
	}

	CurrentSound = Sound;
	CurrentState = NewState;
	CurrentSection = Section;
	RefreshCurrentAudioFinishedBinding();
	CleanupFadingAudioComponents();
	return true;
}

void UPRBGMSubsystem::RefreshCurrentAudioFinishedBinding()
{
	if (!IsValid(CurrentAudioComponent))
	{
		return;
	}

	CurrentAudioComponent->OnAudioFinished.RemoveDynamic(this, &ThisClass::HandleCurrentAudioFinished);
	CurrentAudioComponent->OnAudioFinished.AddDynamic(this, &ThisClass::HandleCurrentAudioFinished);
}

void UPRBGMSubsystem::ClearCurrentAudioFinishedBinding(UAudioComponent* AudioComponent)
{
	if (!IsValid(AudioComponent))
	{
		return;
	}

	AudioComponent->OnAudioFinished.RemoveDynamic(this, &ThisClass::HandleCurrentAudioFinished);
}

void UPRBGMSubsystem::HandleCurrentAudioFinished()
{
	if (CurrentSection == EPRBGMSection::Intro)
	{
		PlayBGMStateSection(CurrentState, EPRBGMSection::Loop, true);
		return;
	}

	if (CurrentState == EPRBGMState::Victory && CurrentSection == EPRBGMSection::Outro)
	{
		ResetToLevelBGM(0.0f);
	}
}

void UPRBGMSubsystem::FadeOutAndDestroyAudioComponent(UAudioComponent* AudioComponent, float FadeOutDuration)
{
	if (!IsValid(AudioComponent))
	{
		return;
	}

	UWorld* World = GetWorld();
	const float ClampedFadeOutDuration = FMath::Max(FadeOutDuration, 0.0f);
	if (!IsValid(World) || ClampedFadeOutDuration <= UE_SMALL_NUMBER)
	{
		AudioComponent->Stop();
		AudioComponent->DestroyComponent();
		return;
	}

	FadingOutAudioComponents.Add(AudioComponent);
	AudioComponent->FadeOut(ClampedFadeOutDuration, 0.0f);

	FTimerHandle DestroyTimerHandle;
	TWeakObjectPtr<UAudioComponent> WeakAudioComponent(AudioComponent);
	World->GetTimerManager().SetTimer(
		DestroyTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, WeakAudioComponent]()
		{
			if (UAudioComponent* FadingAudioComponent = WeakAudioComponent.Get())
			{
				FadingAudioComponent->Stop();
				FadingAudioComponent->DestroyComponent();
			}

			CleanupFadingAudioComponents();
		}),
		ClampedFadeOutDuration,
		false);
}

void UPRBGMSubsystem::CleanupFadingAudioComponents()
{
	FadingOutAudioComponents.RemoveAll([](const TWeakObjectPtr<UAudioComponent>& WeakAudioComponent)
	{
		return !WeakAudioComponent.IsValid();
	});
}

/*~ 전투 상태 연동 ~*/

void UPRBGMSubsystem::BindLocalPlayerCombatTag()
{
	UPRAbilitySystemComponent* AbilitySystemComponent = ResolveLocalPlayerAbilitySystem();
	if (!IsValid(AbilitySystemComponent))
	{
		StartLocalPlayerCombatTagBindRetry();
		return;
	}

	if (BoundAbilitySystemComponent.Get() == AbilitySystemComponent && CombatTagDelegateHandle.IsValid())
	{
		return;
	}

	UnbindLocalPlayerCombatTag();
	BoundAbilitySystemComponent = AbilitySystemComponent;
	CombatTagDelegateHandle = AbilitySystemComponent->OnGameplayTagUpdated.AddUObject(
		this,
		&ThisClass::HandleLocalPlayerGameplayTagUpdated);
	StopLocalPlayerCombatTagBindRetry();

	if (AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Combating))
	{
		SetBGMState(EPRBGMState::Combat);
	}
}

void UPRBGMSubsystem::StartLocalPlayerCombatTagBindRetry()
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetTimerManager().IsTimerActive(LocalPlayerBindRetryTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		LocalPlayerBindRetryTimerHandle,
		this,
		&UPRBGMSubsystem::BindLocalPlayerCombatTag,
		FMath::Max(LocalPlayerBindRetryInterval, 0.1f),
		true);
}

void UPRBGMSubsystem::StopLocalPlayerCombatTagBindRetry()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().ClearTimer(LocalPlayerBindRetryTimerHandle);
}

void UPRBGMSubsystem::UnbindLocalPlayerCombatTag()
{
	UPRAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	if (IsValid(AbilitySystemComponent) && CombatTagDelegateHandle.IsValid())
	{
		AbilitySystemComponent->OnGameplayTagUpdated.Remove(CombatTagDelegateHandle);
	}

	CombatTagDelegateHandle.Reset();
	BoundAbilitySystemComponent = nullptr;
}

UPRAbilitySystemComponent* UPRBGMSubsystem::ResolveLocalPlayerAbilitySystem() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PlayerController = It->Get();
		if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
		{
			continue;
		}

		const APRPlayerState* PRPlayerState = PlayerController->GetPlayerState<APRPlayerState>();
		if (!IsValid(PRPlayerState))
		{
			continue;
		}

		return Cast<UPRAbilitySystemComponent>(PRPlayerState->GetAbilitySystemComponent());
	}

	return nullptr;
}

void UPRBGMSubsystem::HandleLocalPlayerGameplayTagUpdated(const FGameplayTag& Tag, bool bTagExists)
{
	if (!Tag.MatchesTagExact(PRGameplayTags::State_Combating))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().ClearTimer(CombatBGMReleaseTimerHandle);

	if (bTagExists)
	{
		SetBGMState(EPRBGMState::Combat);
		return;
	}

	World->GetTimerManager().SetTimer(
		CombatBGMReleaseTimerHandle,
		this,
		&UPRBGMSubsystem::HandleCombatBGMReleaseDelayElapsed,
		FMath::Max(CombatBGMReleaseDelay, 0.0f),
		false);
}

void UPRBGMSubsystem::HandleCombatBGMReleaseDelayElapsed()
{
	UPRAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	if (IsValid(AbilitySystemComponent) && AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Combating))
	{
		return;
	}

	RestoreDefaultLevelBGM();
}

void UPRBGMSubsystem::BindBossBGMEvents()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>();
	if (!IsValid(EventMgr))
	{
		return;
	}

	if (!BossEncounterBeginDelegateHandle.IsValid())
	{
		BossEncounterBeginDelegateHandle = EventMgr->Listen(
			PRGameplayTags::Event_Boss_Encounter_Begin,
			FPREventMulticast::FDelegate::CreateUObject(this, &ThisClass::HandleBossEncounterBeginEvent));
	}

	if (!BossDefeatedDelegateHandle.IsValid())
	{
		BossDefeatedDelegateHandle = EventMgr->Listen(
			PRGameplayTags::Event_Boss_Defeated,
			FPREventMulticast::FDelegate::CreateUObject(this, &ThisClass::HandleBossDefeatedEvent));
	}

	if (!BossPhaseChangedDelegateHandle.IsValid())
	{
		BossPhaseChangedDelegateHandle = EventMgr->Listen(
			PRGameplayTags::Event_Boss_PhaseChanged,
			FPREventMulticast::FDelegate::CreateUObject(this, &ThisClass::HandleBossPhaseChangedEvent));
	}

	if (!BossBGMPhasePreviewDelegateHandle.IsValid())
	{
		BossBGMPhasePreviewDelegateHandle = EventMgr->Listen(
			PRGameplayTags::Event_Boss_BGMPhasePreview,
			FPREventMulticast::FDelegate::CreateUObject(this, &ThisClass::HandleBossPhaseChangedEvent));
	}

	if (!BossBGMPatternCueDelegateHandle.IsValid())
	{
		BossBGMPatternCueDelegateHandle = EventMgr->Listen(
			PRGameplayTags::Event_Boss_BGMPatternCue,
			FPREventMulticast::FDelegate::CreateUObject(this, &ThisClass::HandleBossBGMPatternCueEvent));
	}
}

void UPRBGMSubsystem::UnbindBossBGMEvents()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		BossEncounterBeginDelegateHandle.Reset();
		BossDefeatedDelegateHandle.Reset();
		BossPhaseChangedDelegateHandle.Reset();
		BossBGMPhasePreviewDelegateHandle.Reset();
		BossBGMPatternCueDelegateHandle.Reset();
		return;
	}

	UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>();
	if (!IsValid(EventMgr))
	{
		BossEncounterBeginDelegateHandle.Reset();
		BossDefeatedDelegateHandle.Reset();
		BossPhaseChangedDelegateHandle.Reset();
		BossBGMPhasePreviewDelegateHandle.Reset();
		BossBGMPatternCueDelegateHandle.Reset();
		return;
	}

	EventMgr->Unlisten(PRGameplayTags::Event_Boss_Encounter_Begin, BossEncounterBeginDelegateHandle);
	EventMgr->Unlisten(PRGameplayTags::Event_Boss_Defeated, BossDefeatedDelegateHandle);
	EventMgr->Unlisten(PRGameplayTags::Event_Boss_PhaseChanged, BossPhaseChangedDelegateHandle);
	EventMgr->Unlisten(PRGameplayTags::Event_Boss_BGMPhasePreview, BossBGMPhasePreviewDelegateHandle);
	EventMgr->Unlisten(PRGameplayTags::Event_Boss_BGMPatternCue, BossBGMPatternCueDelegateHandle);
}

void UPRBGMSubsystem::HandleBossEncounterBeginEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FPRBossEncounterEventPayload* EncounterPayload = Payload.GetPtr<FPRBossEncounterEventPayload>();
	if (!EncounterPayload || !IsValid(EncounterPayload->Boss))
	{
		return;
	}

	PlayBGMStateSection(MapBossPhaseToBGMState(EncounterPayload->Boss->GetCurrentPhase()), EPRBGMSection::Loop, false);
}

void UPRBGMSubsystem::HandleBossDefeatedEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FPRBossEncounterEventPayload* DefeatedPayload = Payload.GetPtr<FPRBossEncounterEventPayload>();
	if (!DefeatedPayload || !IsValid(DefeatedPayload->Boss))
	{
		return;
	}

	const EPRBGMState PreviousState = CurrentState;
	PlayBGMStateSection(EPRBGMState::Victory, EPRBGMSection::Outro, false);

	if (IsBossPhaseState(PreviousState) && IsBossPhaseState(CurrentState))
	{
		StopBGM(1.0f);
	}
}

void UPRBGMSubsystem::HandleBossPhaseChangedEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FPRBossPhaseChangedEventPayload* PhasePayload = Payload.GetPtr<FPRBossPhaseChangedEventPayload>();
	if (!PhasePayload || !IsValid(PhasePayload->Boss))
	{
		return;
	}

	SetBGMState(MapBossPhaseToBGMState(PhasePayload->NewPhase));
}

void UPRBGMSubsystem::HandleBossBGMPatternCueEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FPRBossBGMPatternCueEventPayload* CuePayload = Payload.GetPtr<FPRBossBGMPatternCueEventPayload>();
	if (!CuePayload || !IsValid(CuePayload->Boss))
	{
		return;
	}

	PlayPatternCue(CuePayload->CueTag);
}

bool UPRBGMSubsystem::IsBossPhaseState(EPRBGMState State) const
{
	return State == EPRBGMState::BossPhase1
		|| State == EPRBGMState::BossPhase2
		|| State == EPRBGMState::BossPhase3
		|| State == EPRBGMState::BossPhase4;
}
