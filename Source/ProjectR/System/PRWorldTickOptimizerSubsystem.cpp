// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (World Tick Optimizer 서브시스템 구현)
#include "PRWorldTickOptimizerSubsystem.h"

#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PRWorldTickOptimizerReporterComponent.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRWorldTickOptimizer, Log, All);

namespace
{
	static TAutoConsoleVariable<int32> CVarPRWorldTickOptimizerEnabled(
		TEXT("pr.WorldTickOptimizer.Enabled"),
		1,
		TEXT("월드 Tick 최적화 활성 여부.\n0: 비활성\n1: 활성"),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarPRWorldTickOptimizerShowActors(
		TEXT("pr.WorldTickOptimizer.ShowActors"),
		0,
		TEXT("월드 Tick 최적화 대상 액터 상태 박스 표시 여부.\n0: 비활성\n1: 활성"),
		ECVF_Default);

	// 액터 상태 디버그 박스 표시 여부 확인
	bool IsActorDebugDrawEnabled()
	{
		return CVarPRWorldTickOptimizerShowActors.GetValueOnGameThread() > 0;
	}

	// AlwaysTick 반경 설정 유효성 확인
	bool IsAlwaysTickConfigValid(const FPRTickOptimizationConfig& Config)
	{
		return Config.TickAlwaysActiveActivationRadius > 0.0f
			&& Config.TickAlwaysActiveDeactivationRadius >= Config.TickAlwaysActiveActivationRadius
			&& Config.TickActivationRadius > Config.TickAlwaysActiveActivationRadius
			&& Config.TickDeactivationRadius > Config.TickAlwaysActiveDeactivationRadius;
	}

	// 최적화 대상 액터의 Tick 활성 상태 시각화
	void DrawActorStateBoxes(const UWorld* World, const TArray<FPRTickOptimizationEntry>& ActiveEntries, float DrawDuration)
	{
		if (!IsValid(World) || !IsActorDebugDrawEnabled())
		{
			return;
		}

		const FVector DebugBoxExtent(50.0f, 50.0f, 50.0f);
		const float ClampedDrawDuration = FMath::Max(DrawDuration, 0.0f);
		for (const FPRTickOptimizationEntry& Entry : ActiveEntries)
		{
			const AActor* TargetActor = Entry.TargetActor.Get();
			if (!IsValid(TargetActor))
			{
				continue;
			}

			FColor DebugColor = FColor::Red;
			if (Entry.IsTickActive())
			{
				DebugColor = Entry.DebugState == EPRTickOptimizationDebugState::ActiveByClientRender
					? FColor::Yellow
					: FColor::Green;
			}

			DrawDebugBox(
				World,
				TargetActor->GetActorLocation(),
				DebugBoxExtent,
				DebugColor,
				false,
				ClampedDrawDuration,
				SDPG_Foreground,
				2.0f);
		}
	}
}

/*~ UWorldSubsystem Interface ~*/

void UPRWorldTickOptimizerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StartEvaluationTimer();
}

void UPRWorldTickOptimizerSubsystem::Deinitialize()
{
	StopEvaluationTimer();
	ActiveEntries.Empty();
	PendingRegisterTargets.Empty();
	PendingUnregisterTargets.Empty();
	CachedSources.Empty();
	ClientRenderStates.Empty();
	LastAnyClientRenderedTimeSeconds.Empty();

	Super::Deinitialize();
}

/*~ 등록 API ~*/

void UPRWorldTickOptimizerSubsystem::RegisterTarget(AActor* TargetActor)
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(TargetActor))
	{
		return;
	}

	const TWeakObjectPtr<AActor> TargetKey(TargetActor);
	PendingUnregisterTargets.Remove(TargetKey);
	PendingRegisterTargets.Add(TargetKey);
}

void UPRWorldTickOptimizerSubsystem::UnregisterTarget(AActor* TargetActor)
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(TargetActor))
	{
		return;
	}

	const TWeakObjectPtr<AActor> TargetKey(TargetActor);
	PendingRegisterTargets.Remove(TargetKey);
	PendingUnregisterTargets.Add(TargetKey);
	RemoveServerRenderStateForTarget(TargetActor);
	RemoveLocalRenderStateForTarget(TargetActor);
}

// 대상 액터의 강제 활성 고정
void UPRWorldTickOptimizerSubsystem::ForceActivateTarget(AActor* TargetActor)
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_Client || !IsValid(TargetActor))
	{
		return;
	}

	FlushPendingChanges();

	FPRTickOptimizationEntry* Entry = FindEntry(TargetActor);
	if (Entry == nullptr)
	{
		FPRTickOptimizationEntry NewEntry;
		if (!BuildEntry(TargetActor, NewEntry))
		{
			return;
		}

		ActiveEntries.Add(MoveTemp(NewEntry));
		Entry = &ActiveEntries.Last();
	}

	Entry->bForceActive = true;
	Entry->DebugState = EPRTickOptimizationDebugState::ActiveByAlwaysTick;
	ApplyEntryActiveState(*Entry, true);
}

// 대상 액터의 강제 활성 고정 해제
void UPRWorldTickOptimizerSubsystem::ClearForceActivateTarget(AActor* TargetActor)
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_Client || !IsValid(TargetActor))
	{
		return;
	}

	FlushPendingChanges();

	FPRTickOptimizationEntry* Entry = FindEntry(TargetActor);
	if (Entry == nullptr)
	{
		return;
	}

	Entry->bForceActive = false;
}

void UPRWorldTickOptimizerSubsystem::ForceEvaluate()
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FlushPendingChanges();
	if (World->GetNetMode() == NM_Client)
	{
		// 클라는 렌더 상태만 보고
		EvaluateClientRenderStates();
		return;
	}

	if (!IsOptimizationEnabled())
	{
		RestoreAllTargetsActive();
		DrawActorStateBoxes(World, ActiveEntries, EvaluationInterval);
		CachedSources.Reset();
		return;
	}

	if (World->GetNetMode() != NM_DedicatedServer)
	{
		// 리슨 서버도 렌더 상태 갱신
		EvaluateClientRenderStates();
	}

	// Tick 활성화 여부 종합
	BuildSources();
	CompactServerRenderStates();
	EvaluateTargets();
	DrawActorStateBoxes(World, ActiveEntries, EvaluationInterval);
}

bool UPRWorldTickOptimizerSubsystem::IsOptimizationEnabled()
{
	return CVarPRWorldTickOptimizerEnabled.GetValueOnAnyThread() > 0;
}

void UPRWorldTickOptimizerSubsystem::UpdateClientRenderStates(APlayerController* ReportingPlayerController, const TArray<AActor*>& RenderedTargets, const TArray<AActor*>& NotRenderedTargets)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_Client || !IsValid(ReportingPlayerController))
	{
		return;
	}

	FlushPendingChanges();

	const double CurrentTimeSeconds = World->GetTimeSeconds();
	const TWeakObjectPtr<APlayerController> ClientKey(ReportingPlayerController);
	FPRTickOptimizationClientRenderState& ClientState = ClientRenderStates.FindOrAdd(ClientKey);
	ClientState.PlayerController = ReportingPlayerController;
	ClientState.LastReportTimeSeconds = CurrentTimeSeconds;

	for (AActor* TargetActor : RenderedTargets)
	{
		if (!IsValid(TargetActor) || FindEntry(TargetActor) == nullptr)
		{
			continue;
		}

		const TWeakObjectPtr<AActor> TargetKey(TargetActor);
		ClientState.TargetRenderStates.Add(TargetKey, true);
		LastAnyClientRenderedTimeSeconds.Add(TargetKey, CurrentTimeSeconds);
	}

	for (AActor* TargetActor : NotRenderedTargets)
	{
		if (!IsValid(TargetActor) || FindEntry(TargetActor) == nullptr)
		{
			continue;
		}

		ClientState.TargetRenderStates.Add(TWeakObjectPtr<AActor>(TargetActor), false);
	}
}

/*~ 타이머 관리 ~*/

void UPRWorldTickOptimizerSubsystem::StartEvaluationTimer()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float TimerInterval = World->GetNetMode() == NM_Client
		? ClientRenderEvaluationInterval
		: EvaluationInterval;

	World->GetTimerManager().SetTimer(
		EvaluationTimerHandle,
		this,
		&UPRWorldTickOptimizerSubsystem::ForceEvaluate,
		TimerInterval,
		true,
		TimerInterval);
}

void UPRWorldTickOptimizerSubsystem::StopEvaluationTimer()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().ClearTimer(EvaluationTimerHandle);
}

/*~ 등록 반영 ~*/

void UPRWorldTickOptimizerSubsystem::FlushPendingChanges()
{
	CompactEntries();

	for (const TWeakObjectPtr<AActor>& WeakTarget : PendingRegisterTargets)
	{
		AActor* TargetActor = WeakTarget.Get();
		if (!IsValid(TargetActor))
		{
			continue;
		}

		const bool bAlreadyRegistered = ActiveEntries.ContainsByPredicate(
			[TargetActor](const FPRTickOptimizationEntry& Entry)
			{
				return Entry.TargetActor.Get() == TargetActor;
			});
		if (bAlreadyRegistered)
		{
			continue;
		}

		FPRTickOptimizationEntry NewEntry;
		if (BuildEntry(TargetActor, NewEntry))
		{
			ActiveEntries.Add(MoveTemp(NewEntry));
		}
	}

	PendingRegisterTargets.Empty();
	PendingUnregisterTargets.Empty();
	NextEvaluationIndex = ActiveEntries.IsValidIndex(NextEvaluationIndex) ? NextEvaluationIndex : 0;
}

void UPRWorldTickOptimizerSubsystem::CompactEntries()
{
	ActiveEntries.RemoveAllSwap(
		[this](const FPRTickOptimizationEntry& Entry)
		{
			AActor* TargetActor = Entry.TargetActor.Get();
			return !IsValid(TargetActor) || PendingUnregisterTargets.Contains(TWeakObjectPtr<AActor>(TargetActor));
		},
		EAllowShrinking::No);
}

bool UPRWorldTickOptimizerSubsystem::BuildEntry(AActor* TargetActor, FPRTickOptimizationEntry& OutEntry) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	IPRTickOptimizable* TargetInterface = Cast<IPRTickOptimizable>(TargetActor);
	if (TargetInterface == nullptr)
	{
		UE_LOG(
			LogPRWorldTickOptimizer,
			Warning,
			TEXT("[WorldTickOptimizer] TickOptimizable 미구현 Target=%s"),
			*GetNameSafe(TargetActor));
		return false;
	}

	const FPRTickOptimizationConfig Config = TargetInterface->GetTickConfig();
	if (Config.TickActivationRadius <= 0.0f || Config.TickDeactivationRadius < Config.TickActivationRadius)
	{
		UE_LOG(
			LogPRWorldTickOptimizer,
			Warning,
			TEXT("[WorldTickOptimizer] Tick 반경 설정 오류 Target=%s Activation=%.1f Deactivation=%.1f"),
			*GetNameSafe(TargetActor),
			Config.TickActivationRadius,
			Config.TickDeactivationRadius);
		return false;
	}

	if (!IsAlwaysTickConfigValid(Config))
	{
		UE_LOG(
			LogPRWorldTickOptimizer,
			Warning,
			TEXT("[WorldTickOptimizer] Tick 내부 반경 설정 오류 Target=%s InnerActivation=%.1f InnerDeactivation=%.1f OuterActivation=%.1f OuterDeactivation=%.1f"),
			*GetNameSafe(TargetActor),
			Config.TickAlwaysActiveActivationRadius,
			Config.TickAlwaysActiveDeactivationRadius,
			Config.TickActivationRadius,
			Config.TickDeactivationRadius);
	}

	OutEntry.TargetActor = TargetActor;
	OutEntry.TargetInterface.SetObject(TargetActor);
	OutEntry.TargetInterface.SetInterface(TargetInterface);
	OutEntry.Config = Config;
	OutEntry.bIsTickActive = Config.bStartTickActive || TargetInterface->IsTickActive();
	if (const UWorld* World = GetWorld())
	{
		OutEntry.RegisteredWorldTimeSeconds = World->GetTimeSeconds();
	}
	return true;
}

/*~ 거리 평가 ~*/

void UPRWorldTickOptimizerSubsystem::BuildSources()
{
	CachedSources.Reset();

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		const APlayerController* PlayerController = Iterator->Get();
		if (!IsValid(PlayerController))
		{
			continue;
		}

		APawn* PlayerPawn = PlayerController->GetPawn();
		if (!IsValid(PlayerPawn) || UPRCombatStatics::GetActorTeam(PlayerPawn) != EPRTeam::Player)
		{
			continue;
		}

		const UAbilitySystemComponent* AbilitySystemComponent = UPRCombatStatics::FindAbilitySystemComponent(PlayerPawn);
		if (IsValid(AbilitySystemComponent) && AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Dead))
		{
			continue;
		}

		FPRTickOptimizationSource Source;
		Source.Location = PlayerPawn->GetActorLocation();
		Source.SourceActor = PlayerPawn;
		CachedSources.Add(Source);
	}
}

void UPRWorldTickOptimizerSubsystem::EvaluateTargets()
{
	if (ActiveEntries.IsEmpty())
	{
		NextEvaluationIndex = 0;
		return;
	}

	if (MaxTargetsPerEvaluation <= 0 || MaxTargetsPerEvaluation >= ActiveEntries.Num())
	{
		for (FPRTickOptimizationEntry& Entry : ActiveEntries)
		{
			EvaluateTarget(Entry);
		}

		NextEvaluationIndex = 0;
		return;
	}

	const int32 EvaluationCount = FMath::Min(MaxTargetsPerEvaluation, ActiveEntries.Num());
	for (int32 Count = 0; Count < EvaluationCount; ++Count)
	{
		if (!ActiveEntries.IsValidIndex(NextEvaluationIndex))
		{
			NextEvaluationIndex = 0;
		}

		EvaluateTarget(ActiveEntries[NextEvaluationIndex]);
		NextEvaluationIndex = (NextEvaluationIndex + 1) % ActiveEntries.Num();
	}
}

void UPRWorldTickOptimizerSubsystem::RestoreAllTargetsActive()
{
	for (FPRTickOptimizationEntry& Entry : ActiveEntries)
	{
		AActor* TargetActor = Entry.TargetActor.Get();
		IPRTickOptimizable* TargetInterface = Entry.TargetInterface.GetInterface();
		if (!IsValid(TargetActor) || TargetInterface == nullptr)
		{
			continue;
		}

		if (!Entry.bIsTickActive)
		{
			Entry.bIsTickActive = true;
			TargetInterface->SetTickActive(true);
		}

		Entry.DebugState = EPRTickOptimizationDebugState::ActiveByAlwaysTick;
	}

	NextEvaluationIndex = 0;
}

// 대상 항목의 Tick 활성 상태 적용
void UPRWorldTickOptimizerSubsystem::ApplyEntryActiveState(FPRTickOptimizationEntry& Entry, bool bTickActive)
{
	AActor* TargetActor = Entry.TargetActor.Get();
	IPRTickOptimizable* TargetInterface = Entry.TargetInterface.GetInterface();
	if (!IsValid(TargetActor) || TargetInterface == nullptr)
	{
		return;
	}

	if (Entry.bIsTickActive != bTickActive)
	{
		Entry.bIsTickActive = bTickActive;
		TargetInterface->SetTickActive(bTickActive);
	}
}

void UPRWorldTickOptimizerSubsystem::EvaluateClientRenderStates()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	APlayerController* LocalPlayerController = nullptr;
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (IsValid(PlayerController) && PlayerController->IsLocalController())
		{
			LocalPlayerController = PlayerController;
			break;
		}
	}

	if (!IsValid(LocalPlayerController))
	{
		return;
	}

	UPRWorldTickOptimizerReporterComponent* ReporterComponent = LocalPlayerController->FindComponentByClass<UPRWorldTickOptimizerReporterComponent>();
	if (!IsValid(ReporterComponent))
	{
		return;
	}

	const double CurrentTimeSeconds = World->GetTimeSeconds();
	for (FPRTickOptimizationEntry& Entry : ActiveEntries)
	{
		AActor* TargetActor = Entry.TargetActor.Get();
		IPRTickOptimizable* TargetInterface = Entry.TargetInterface.GetInterface();
		if (!IsValid(TargetActor) || TargetInterface == nullptr)
		{
			continue;
		}

		if (Entry.Config.bAllowTargetRuntimeEvaluationGate && !TargetInterface->CanOptimizeTick())
		{
			ReporterComponent->RemoveTarget(TargetActor);
			continue;
		}

		const bool bRecentlyRendered = TargetActor->WasRecentlyRendered(RenderedToleranceSeconds);
		ReporterComponent->PushRenderState(TargetActor, bRecentlyRendered, CurrentTimeSeconds);
	}
}

void UPRWorldTickOptimizerSubsystem::EvaluateTarget(FPRTickOptimizationEntry& Entry)
{
	AActor* TargetActor = Entry.TargetActor.Get();
	IPRTickOptimizable* TargetInterface = Entry.TargetInterface.GetInterface();
	if (!IsValid(TargetActor) || TargetInterface == nullptr)
	{
		return;
	}

	if (Entry.Config.bAllowTargetRuntimeEvaluationGate && !TargetInterface->CanOptimizeTick())
	{
		return;
	}

	if (Entry.bForceActive)
	{
		// 강제 활성 대상의 거리 기반 비활성화 차단
		Entry.DebugState = EPRTickOptimizationDebugState::ActiveByAlwaysTick;
		ApplyEntryActiveState(Entry, true);
		return;
	}

	const FVector TargetLocation = TargetInterface->GetTickLocation();
	bool bShouldTickBeActive = false;
	if (IsAlwaysTickConfigValid(Entry.Config))
	{
		const float AlwaysTickRadius = Entry.bIsTickActive
			? Entry.Config.TickAlwaysActiveDeactivationRadius
			: Entry.Config.TickAlwaysActiveActivationRadius;

		if (HasSourceInsideRadius(TargetLocation, AlwaysTickRadius))
		{
			bShouldTickBeActive = true;
			Entry.DebugState = EPRTickOptimizationDebugState::ActiveByAlwaysTick;
		}
		else if (IsInsideClientRenderRelevantRadius(Entry, TargetLocation) && HasClientRenderActivation(Entry))
		{
			bShouldTickBeActive = true;
			Entry.DebugState = EPRTickOptimizationDebugState::ActiveByClientRender;
		}
		else
		{
			bShouldTickBeActive = false;
			Entry.DebugState = EPRTickOptimizationDebugState::OutsideRadius;
		}
	}
	else
	{
		bShouldTickBeActive = Entry.bIsTickActive
			? HasSourceInsideRadius(TargetLocation, Entry.Config.TickDeactivationRadius)
			: HasSourceInsideRadius(TargetLocation, Entry.Config.TickActivationRadius);
		Entry.DebugState = bShouldTickBeActive
			? EPRTickOptimizationDebugState::ActiveByAlwaysTick
			: EPRTickOptimizationDebugState::OutsideRadius;
	}

	ApplyEntryActiveState(Entry, bShouldTickBeActive);
}

// 활성 목록의 대상 항목 검색
FPRTickOptimizationEntry* UPRWorldTickOptimizerSubsystem::FindEntry(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return nullptr;
	}

	return ActiveEntries.FindByPredicate(
		[TargetActor](const FPRTickOptimizationEntry& Entry)
		{
			return Entry.TargetActor.Get() == TargetActor;
		});
}

bool UPRWorldTickOptimizerSubsystem::HasSourceInsideRadius(const FVector& TargetLocation, float Radius) const
{
	const float RadiusSquared = FMath::Square(Radius);
	for (const FPRTickOptimizationSource& Source : CachedSources)
	{
		if (FVector::DistSquared(TargetLocation, Source.Location) <= RadiusSquared)
		{
			return true;
		}
	}

	return false;
}

bool UPRWorldTickOptimizerSubsystem::HasClientRenderActivation(const FPRTickOptimizationEntry& Entry) const
{
	const UWorld* World = GetWorld();
	AActor* TargetActor = Entry.TargetActor.Get();
	if (!IsValid(World) || !IsValid(TargetActor))
	{
		return false;
	}

	const double CurrentTimeSeconds = World->GetTimeSeconds();
	const TWeakObjectPtr<AActor> TargetKey(TargetActor);
	if (const double* LastRenderedTimeSeconds = LastAnyClientRenderedTimeSeconds.Find(TargetKey))
	{
		if (CurrentTimeSeconds - *LastRenderedTimeSeconds <= ClientRenderGraceTime)
		{
			return true;
		}
	}

	bool bHasUnknownClientState = false;
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (!IsValid(PlayerController))
		{
			continue;
		}

		const FPRTickOptimizationClientRenderState* ClientState = ClientRenderStates.Find(TWeakObjectPtr<APlayerController>(PlayerController));
		if (ClientState == nullptr)
		{
			bHasUnknownClientState = true;
			continue;
		}

		const bool* bClientRendered = ClientState->TargetRenderStates.Find(TargetKey);
		if (bClientRendered == nullptr)
		{
			bHasUnknownClientState = true;
			continue;
		}

		if (*bClientRendered)
		{
			return true;
		}
	}

	return bHasUnknownClientState
		&& CurrentTimeSeconds - Entry.RegisteredWorldTimeSeconds <= UnknownClientGraceTime;
}

bool UPRWorldTickOptimizerSubsystem::IsInsideClientRenderRelevantRadius(const FPRTickOptimizationEntry& Entry, const FVector& TargetLocation) const
{
	return HasSourceInsideRadius(TargetLocation, Entry.Config.TickDeactivationRadius);
}

void UPRWorldTickOptimizerSubsystem::RemoveServerRenderStateForTarget(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	const TWeakObjectPtr<AActor> TargetKey(TargetActor);
	LastAnyClientRenderedTimeSeconds.Remove(TargetKey);
	for (TPair<TWeakObjectPtr<APlayerController>, FPRTickOptimizationClientRenderState>& ClientStatePair : ClientRenderStates)
	{
		ClientStatePair.Value.TargetRenderStates.Remove(TargetKey);
	}
}

void UPRWorldTickOptimizerSubsystem::CompactServerRenderStates()
{
	for (auto ClientIterator = ClientRenderStates.CreateIterator(); ClientIterator; ++ClientIterator)
	{
		if (!ClientIterator.Key().IsValid())
		{
			ClientIterator.RemoveCurrent();
			continue;
		}

		for (auto TargetIterator = ClientIterator.Value().TargetRenderStates.CreateIterator(); TargetIterator; ++TargetIterator)
		{
			if (!TargetIterator.Key().IsValid())
			{
				TargetIterator.RemoveCurrent();
			}
		}
	}

	for (auto RenderTimeIterator = LastAnyClientRenderedTimeSeconds.CreateIterator(); RenderTimeIterator; ++RenderTimeIterator)
	{
		if (!RenderTimeIterator.Key().IsValid())
		{
			RenderTimeIterator.RemoveCurrent();
		}
	}
}

void UPRWorldTickOptimizerSubsystem::RemoveLocalRenderStateForTarget(AActor* TargetActor) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(TargetActor) || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
		{
			continue;
		}

		if (UPRWorldTickOptimizerReporterComponent* ReporterComponent = PlayerController->FindComponentByClass<UPRWorldTickOptimizerReporterComponent>())
		{
			ReporterComponent->RemoveTarget(TargetActor);
		}
	}
}
