// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (전투 Engagement 서브시스템 구현)
#include "PRCombatEngagementSubsystem.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Engine/World.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"
#include "TimerManager.h"

/*~ UWorldSubsystem Interface ~*/

void UPRCombatEngagementSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StartFlushTimer();
}

void UPRCombatEngagementSubsystem::Deinitialize()
{
	StopFlushTimer();
	PendingEngagedPlayers.Empty();
	ReporterByPlayer.Empty();

	Super::Deinitialize();
}

/*~ 교전 보고 ~*/

void UPRCombatEngagementSubsystem::ReportCombatEngagedCandidate(AActor* Reporter, AActor* Candidate)
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_Client || !IsValid(Reporter) || !IsValidEngagedCandidate(Candidate))
	{
		return;
	}

	const TWeakObjectPtr<AActor> CandidateKey(Candidate);
	PendingEngagedPlayers.Add(CandidateKey);

	if (!ReporterByPlayer.Contains(CandidateKey))
	{
		ReporterByPlayer.Add(CandidateKey, Reporter);
	}
}

/*~ 집계 처리 ~*/

void UPRCombatEngagementSubsystem::FlushCombatEngagedEvents()
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_Client)
	{
		PendingEngagedPlayers.Empty();
		ReporterByPlayer.Empty();
		return;
	}

	for (const TWeakObjectPtr<AActor>& WeakPlayer : PendingEngagedPlayers)
	{
		AActor* PlayerActor = WeakPlayer.Get();
		if (!IsValidEngagedCandidate(PlayerActor))
		{
			continue;
		}

		AActor* ReporterActor = nullptr;
		if (const TWeakObjectPtr<AActor>* ReporterPtr = ReporterByPlayer.Find(WeakPlayer))
		{
			ReporterActor = ReporterPtr->Get();
		}

		FGameplayEventData Payload;
		Payload.EventTag = PRGameplayTags::Event_Combat_Engaged;
		Payload.Instigator = ReporterActor;
		Payload.Target = PlayerActor;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			PlayerActor,
			PRGameplayTags::Event_Combat_Engaged,
			Payload);
	}

	PendingEngagedPlayers.Empty();
	ReporterByPlayer.Empty();
}

void UPRCombatEngagementSubsystem::StartFlushTimer()
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_Client)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		FlushTimerHandle,
		this,
		&UPRCombatEngagementSubsystem::FlushCombatEngagedEvents,
		FlushInterval,
		true,
		FlushInterval);
}

void UPRCombatEngagementSubsystem::StopFlushTimer()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().ClearTimer(FlushTimerHandle);
}

bool UPRCombatEngagementSubsystem::IsValidEngagedCandidate(const AActor* Candidate) const
{
	if (!IsValid(Candidate) || UPRCombatStatics::GetActorTeam(Candidate) != EPRTeam::Player)
	{
		return false;
	}

	const UAbilitySystemComponent* AbilitySystemComponent = UPRCombatStatics::FindAbilitySystemComponent(Candidate);
	if (!IsValid(AbilitySystemComponent))
	{
		return false;
	}

	return !AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Down)
		&& !AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Dead);
}
