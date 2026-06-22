// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (보스 전용 UI 체력 바 및 페이즈 BGM 연동 구현)
// Author: 배유찬 (보스 조우 및 월드 리셋, 약점 부위 히트 판정 구조 구축)
// Author: 손승우 (보스 AI 비헤이비어 제어 및 페어린 보스 패턴 연동 구현)
#include "PRBossBaseCharacter.h"

#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AI/Boss/PRBossPatternActor.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Game/PRPlayGameMode.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/System/PRRespawnSubsystem.h"

APRBossBaseCharacter::APRBossBaseCharacter()
{
	// 페이즈 임계값은 보스 BP/데이터에서 설정한다. C++ 기본값을 두면 몬스터별 튜닝이 하드코딩된다.
	bUseWorldHealthBar = false;
	TickOptimizationConfig.TickActivationRadius = 15000.f;
	TickOptimizationConfig.TickDeactivationRadius = 20000.f;
	TickOptimizationConfig.TickAlwaysActiveActivationRadius = 15000.f;
	TickOptimizationConfig.TickAlwaysActiveDeactivationRadius = 20000.f;
}

void APRBossBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APRBossBaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelAllBossPatternActors();
	Super::EndPlay(EndPlayReason);
}

void APRBossBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	if (!bUsePhaseAbilitySets)
	{
		return;
	}

	if (TObjectPtr<UPRAbilitySet>* FoundAbilitySet = PhaseAbilitySets.Find(CurrentPhase))
	{
		if (IsValid(*FoundAbilitySet))
		{
			// 기본 Enemy AbilitySet 위에 현재 페이즈 전용 AbilitySet을 추가로 부여한다.
			AbilitySystemComponent->ClearAbilitySetByHandles(CurrentPhaseHandles);
			AbilitySystemComponent->GiveAbilitySet(*FoundAbilitySet, CurrentPhaseHandles);
		}
	}
}

void APRBossBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRBossBaseCharacter, CurrentPhase);
}

void APRBossBaseCharacter::OnPostDamageApplied(const FPRDamageAppliedContext& Context)
{
	Super::OnPostDamageApplied(Context);

	if (Context.MaxHealth > 0.0f)
	{
		// 데미지 적용 직후 예측 체력 비율로 페이즈 전환을 검사한다.
		const float NewRatio = FMath::Clamp(Context.HealthAfterDamage / Context.MaxHealth, 0.0f, 1.0f);
		OnHealthRatioChanged(NewRatio);
	}
}

// 조우 상태를 별도로 관리하지 않는 보스의 기본 요청 처리
void APRBossBaseCharacter::RequestBossEncounterBegin()
{
	// 조우 이벤트를 직접 관리하지 않는 보스의 기본 동작 없음
}

void APRBossBaseCharacter::OnHealthRatioChanged(float NewRatio)
{
	if (!HasAuthority())
	{
		return;
	}

	// 페이즈 전환 판단은 서버 체력 비율만 신뢰하고, enum 순서를 진행 순서로 사용한다.
	EPRBossPhase CandidatePhase = CurrentPhase;
	const uint8 CurrentPhaseIndex = static_cast<uint8>(CurrentPhase);
	uint8 CandidatePhaseIndex = CurrentPhaseIndex;

	for (const TPair<EPRBossPhase, float>& ThresholdPair : PhaseThresholdRatios)
	{
		const EPRBossPhase Phase = ThresholdPair.Key;
		const uint8 PhaseIndex = static_cast<uint8>(Phase);
		const float ThresholdRatio = ThresholdPair.Value;

		if (PhaseIndex <= CurrentPhaseIndex)
		{
			continue;
		}

		if (NewRatio <= ThresholdRatio && PhaseIndex > CandidatePhaseIndex)
		{
			CandidatePhase = Phase;
			CandidatePhaseIndex = PhaseIndex;
		}
	}

	if (CandidatePhase != CurrentPhase)
	{
		BeginPhaseTransition(CandidatePhase);
	}
}

void APRBossBaseCharacter::BeginPhaseTransition(EPRBossPhase NewPhase)
{
	if (!HasAuthority()
		|| CurrentPhase == NewPhase
		|| PendingPhase == NewPhase
		|| static_cast<uint8>(NewPhase) <= static_cast<uint8>(CurrentPhase)
		|| !IsValid(AbilitySystemComponent)
		|| AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_PhaseTransitioning))
	{
		return;
	}

	PendingPhase = NewPhase;
	AbilitySystemComponent->AddLooseGameplayTag(PRGameplayTags::State_PhaseTransitioning);
	AbilitySystemComponent->AddReplicatedLooseGameplayTag(PRGameplayTags::State_PhaseTransitioning);

	FGameplayTagContainer PatternCancelTags;
	PatternCancelTags.AddTag(PRGameplayTags::Ability_Boss_Pattern);
	AbilitySystemComponent->CancelAbilities(&PatternCancelTags);
	CancelBossPatternActorsForPhaseTransition();

	FGameplayEventData Payload;
	Payload.EventTag = PRGameplayTags::Event_Ability_PhaseTransition;
	Payload.Instigator = this;
	Payload.Target = this;
	Payload.EventMagnitude = static_cast<float>(static_cast<uint8>(NewPhase));

	// 페이즈 전환 Ability는 서버가 확정한 목표 페이즈를 GameplayEvent로 받아 시작한다.
	const int32 TriggeredAbilityCount = AbilitySystemComponent->HandleGameplayEvent(
		PRGameplayTags::Event_Ability_PhaseTransition,
		&Payload);

	if (TriggeredAbilityCount <= 0)
	{
		// 전환 Ability가 아직 연결되지 않은 테스트 상태에서는 페이즈가 고착되지 않도록 즉시 확정한다.
		CommitPhaseTransition(NewPhase);
	}
}

void APRBossBaseCharacter::CommitPhaseTransition(EPRBossPhase NewPhase)
{
	if (!HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	if (CurrentPhase == NewPhase)
	{
		PendingPhase = NewPhase;
		AbilitySystemComponent->RemoveLooseGameplayTag(PRGameplayTags::State_PhaseTransitioning);
		AbilitySystemComponent->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_PhaseTransitioning);
		return;
	}

	const EPRBossPhase OldPhase = CurrentPhase;

	if (bUsePhaseAbilitySets)
	{
		// 이전 페이즈에서만 열렸던 Ability/GE를 회수하고 새 페이즈 세트를 부여한다.
		AbilitySystemComponent->ClearAbilitySetByHandles(CurrentPhaseHandles);

		if (TObjectPtr<UPRAbilitySet>* FoundAbilitySet = PhaseAbilitySets.Find(NewPhase))
		{
			if (IsValid(*FoundAbilitySet))
			{
				AbilitySystemComponent->GiveAbilitySet(*FoundAbilitySet, CurrentPhaseHandles);
			}
		}
	}

	CurrentPhase = NewPhase;
	PendingPhase = NewPhase;
	AbilitySystemComponent->RemoveLooseGameplayTag(PRGameplayTags::State_PhaseTransitioning);
	AbilitySystemComponent->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_PhaseTransitioning);

	BroadcastBossPhaseChangedEvent(OldPhase, NewPhase);
	OnPhaseChanged.Broadcast(OldPhase, NewPhase);
}

void APRBossBaseCharacter::BroadcastBossBGMPhasePreview(EPRBossPhase PreviewPhase)
{
	if (!HasAuthority())
	{
		return;
	}

	Multicast_BroadcastBossBGMPhasePreview(PreviewPhase);
}

void APRBossBaseCharacter::Multicast_BroadcastBossBGMPhasePreview_Implementation(EPRBossPhase PreviewPhase)
{
	BroadcastBossBGMPhasePreviewEvent(PreviewPhase);
}

void APRBossBaseCharacter::BroadcastBossBGMPatternCue(FGameplayTag CueTag)
{
	if (!HasAuthority() || !CueTag.IsValid())
	{
		return;
	}

	Multicast_BroadcastBossBGMPatternCue(CueTag);
}

void APRBossBaseCharacter::Multicast_BroadcastBossBGMPatternCue_Implementation(FGameplayTag CueTag)
{
	BroadcastBossBGMPatternCueEvent(CueTag);
}

void APRBossBaseCharacter::BroadcastBossDefeated()
{
	if (!HasAuthority())
	{
		return;
	}

	Multicast_BroadcastBossDefeated();
}

void APRBossBaseCharacter::Multicast_BroadcastBossDefeated_Implementation()
{
	BroadcastBossDefeatedEvent();
}

void APRBossBaseCharacter::RegisterBossPatternActor(APRBossPatternActor* Actor)
{
	if (!HasAuthority() || !IsValid(Actor))
	{
		return;
	}

	ActivePatternActors.RemoveAll([](const APRBossPatternActor* RegisteredActor)
	{
		return !IsValid(RegisteredActor);
	});

	ActivePatternActors.AddUnique(Actor);
}

void APRBossBaseCharacter::UnregisterBossPatternActor(APRBossPatternActor* Actor)
{
	if (!HasAuthority())
	{
		return;
	}

	ActivePatternActors.RemoveAll([Actor](const APRBossPatternActor* RegisteredActor)
	{
		return !IsValid(RegisteredActor) || RegisteredActor == Actor;
	});
}

void APRBossBaseCharacter::CancelAllBossPatternActors()
{
	if (!HasAuthority())
	{
		return;
	}

	TArray<TObjectPtr<APRBossPatternActor>> PatternActorsToCancel = ActivePatternActors;
	ActivePatternActors.Reset();

	for (APRBossPatternActor* PatternActor : PatternActorsToCancel)
	{
		if (IsValid(PatternActor))
		{
			PatternActor->CancelPatternActor();
		}
	}
}

void APRBossBaseCharacter::CancelBossPatternActorsForPhaseTransition()
{
	if (!HasAuthority())
	{
		return;
	}

	TArray<TObjectPtr<APRBossPatternActor>> PatternActorsToCancel;
	TArray<TObjectPtr<APRBossPatternActor>> PatternActorsToKeep;
	for (APRBossPatternActor* PatternActor : ActivePatternActors)
	{
		if (!IsValid(PatternActor))
		{
			continue;
		}

		if (PatternActor->ShouldCancelOnBossPhaseTransition())
		{
			PatternActorsToCancel.Add(PatternActor);
		}
		else
		{
			PatternActorsToKeep.Add(PatternActor);
		}
	}

	ActivePatternActors = MoveTemp(PatternActorsToKeep);

	for (APRBossPatternActor* PatternActor : PatternActorsToCancel)
	{
		if (IsValid(PatternActor))
		{
			PatternActor->CancelPatternActor();
		}
	}
}

FText APRBossBaseCharacter::GetBossDisplayName() const
{
	if (!CharacterDisplayName.IsEmpty())
	{
		return CharacterDisplayName;
	}
	return FText::FromName(CharacterID);
}

void APRBossBaseCharacter::GetPhaseThresholdRatios(TArray<float>& OutRatios) const
{
	OutRatios.Reset();

	for (const TPair<EPRBossPhase, float>& ThresholdPair : PhaseThresholdRatios)
	{
		OutRatios.Add(FMath::Clamp(ThresholdPair.Value, 0.0f, 1.0f));
	}

	OutRatios.Sort([](float Left, float Right)
	{
		return Left > Right;
	});
}

void APRBossBaseCharacter::HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool TagExists)
{
	Super::HandleGameplayTagUpdated(ChangedTag, TagExists);

	if (!TagExists)
	{
		return;
	}
	
	// 보스는 사망시 리스폰 대상에서 제거
	if (HasAuthority())
	{
		if (ChangedTag.MatchesTag(PRGameplayTags::State_Dead))
		{
			if (bIsRespawnable)
			{
				// 리스폰 시스템 등록해제
				if (UPRRespawnSubsystem* RespawnSubsystem = GetWorld()->GetSubsystem<UPRRespawnSubsystem>())
				{
					RespawnSubsystem->UnregisterRespawnableActor(this);
				}
				bIsRespawnable = false;
			}
			
			// 보스 처치 상태 보고
			if (APRPlayGameMode* PlayGameMode = GetWorld()->GetAuthGameMode<APRPlayGameMode>())
			{
				PlayGameMode->ReportBossDefeated(GetMonsterId());
			}

			BroadcastBossDefeated();
		}
	}
	
	if (ChangedTag.MatchesTag(PRGameplayTags::State_Dead)
	|| ChangedTag.MatchesTag(PRGameplayTags::State_Groggy))
	{
		CancelAllBossPatternActors();
	}
}

void APRBossBaseCharacter::BroadcastBossPhaseChangedEvent(EPRBossPhase OldPhase, EPRBossPhase NewPhase)
{
	if (OldPhase == NewPhase)
	{
		return;
	}

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

	FPRBossPhaseChangedEventPayload Payload;
	Payload.Boss = this;
	Payload.OldPhase = OldPhase;
	Payload.NewPhase = NewPhase;
	EventMgr->BroadcastTyped(PRGameplayTags::Event_Boss_PhaseChanged, Payload);
}

void APRBossBaseCharacter::BroadcastBossBGMPhasePreviewEvent(EPRBossPhase PreviewPhase)
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

	FPRBossPhaseChangedEventPayload Payload;
	Payload.Boss = this;
	Payload.OldPhase = CurrentPhase;
	Payload.NewPhase = PreviewPhase;
	EventMgr->BroadcastTyped(PRGameplayTags::Event_Boss_BGMPhasePreview, Payload);
}

void APRBossBaseCharacter::BroadcastBossBGMPatternCueEvent(FGameplayTag CueTag)
{
	if (!CueTag.IsValid())
	{
		return;
	}

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

	FPRBossBGMPatternCueEventPayload Payload;
	Payload.Boss = this;
	Payload.CueTag = CueTag;
	EventMgr->BroadcastTyped(PRGameplayTags::Event_Boss_BGMPatternCue, Payload);
}

void APRBossBaseCharacter::BroadcastBossDefeatedEvent()
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

	FPRBossEncounterEventPayload Payload;
	Payload.Boss = this;
	EventMgr->BroadcastTyped(PRGameplayTags::Event_Boss_Defeated, Payload);
}

void APRBossBaseCharacter::OnRep_CurrentPhase(EPRBossPhase OldPhase)
{
	// 클라이언트는 복제된 페이즈 변화로 연출/UI를 동기화한다.
	BroadcastBossPhaseChangedEvent(OldPhase, CurrentPhase);
	OnPhaseChanged.Broadcast(OldPhase, CurrentPhase);
}
