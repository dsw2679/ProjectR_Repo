// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (World Tick Optimizer 클라이언트 렌더 상태 보고 컴포넌트 구현)
#include "PRWorldTickOptimizerReporterComponent.h"

#include "GameFramework/PlayerController.h"
#include "ProjectR/System/PRWorldTickOptimizerSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRWorldTickOptimizerReporter, Log, All);

UPRWorldTickOptimizerReporterComponent::UPRWorldTickOptimizerReporterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.25f;
	SetIsReplicatedByDefault(true);
}

void UPRWorldTickOptimizerReporterComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(IsLocalReportingComponent());
}

void UPRWorldTickOptimizerReporterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsLocalReportingComponent())
	{
		SetComponentTickEnabled(false);
		return;
	}

	FlushPendingRenderReports();
}

void UPRWorldTickOptimizerReporterComponent::PushRenderState(AActor* TargetActor, bool bRendered, double CurrentTimeSeconds)
{
	if (!IsValid(TargetActor))
	{
		CompactInvalidTargets();
		return;
	}

	const TWeakObjectPtr<AActor> TargetKey(TargetActor);
	FPRClientLocalRenderReportState& LocalState = LocalRenderReportStates.FindOrAdd(TargetKey);
	LocalState.bCurrentRendered = bRendered;
	LocalState.LastObservedTimeSeconds = CurrentTimeSeconds;

	if (bRendered)
	{
		LocalState.LastRenderedTimeSeconds = CurrentTimeSeconds;
	}

	if (LocalState.LastReportedState == EPRClientRenderReportState::Unknown)
	{
		PendingRenderStateReports.Add(TargetKey, bRendered);
		return;
	}

	if (bRendered)
	{
		if (LocalState.LastReportedState != EPRClientRenderReportState::Rendered)
		{
			PendingRenderStateReports.Add(TargetKey, true);
		}
		else
		{
			PendingRenderStateReports.Remove(TargetKey);
		}
		return;
	}

	if (LocalState.LastReportedState == EPRClientRenderReportState::Rendered)
	{
		const double NotRenderedDuration = CurrentTimeSeconds - LocalState.LastRenderedTimeSeconds;
		if (NotRenderedDuration >= ClientNotRenderedGraceTime)
		{
			PendingRenderStateReports.Add(TargetKey, false);
		}
		return;
	}

	PendingRenderStateReports.Remove(TargetKey);
}

void UPRWorldTickOptimizerReporterComponent::RemoveTarget(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	const TWeakObjectPtr<AActor> TargetKey(TargetActor);
	LocalRenderReportStates.Remove(TargetKey);
	PendingRenderStateReports.Remove(TargetKey);
}

void UPRWorldTickOptimizerReporterComponent::CompactInvalidTargets()
{
	for (auto CacheIterator = LocalRenderReportStates.CreateIterator(); CacheIterator; ++CacheIterator)
	{
		if (!CacheIterator.Key().IsValid())
		{
			CacheIterator.RemoveCurrent();
		}
	}

	for (auto PendingIterator = PendingRenderStateReports.CreateIterator(); PendingIterator; ++PendingIterator)
	{
		if (!PendingIterator.Key().IsValid())
		{
			PendingIterator.RemoveCurrent();
		}
	}
}

void UPRWorldTickOptimizerReporterComponent::FlushPendingRenderReports()
{
	CompactInvalidTargets();

	if (PendingRenderStateReports.IsEmpty())
	{
		return;
	}

	TArray<AActor*> RenderedTargets;
	TArray<AActor*> NotRenderedTargets;
	TArray<TWeakObjectPtr<AActor>> FlushedTargets;

	const int32 MaxFlushCount = FMath::Max(MaxRenderReportsPerFlush, 1);
	for (const TPair<TWeakObjectPtr<AActor>, bool>& PendingReport : PendingRenderStateReports)
	{
		if (FlushedTargets.Num() >= MaxFlushCount)
		{
			break;
		}

		AActor* TargetActor = PendingReport.Key.Get();
		if (!IsValid(TargetActor))
		{
			FlushedTargets.Add(PendingReport.Key);
			continue;
		}

		if (PendingReport.Value)
		{
			RenderedTargets.Add(TargetActor);
		}
		else
		{
			NotRenderedTargets.Add(TargetActor);
		}

		FlushedTargets.Add(PendingReport.Key);
	}

	if (!RenderedTargets.IsEmpty() || !NotRenderedTargets.IsEmpty())
	{
		ServerReportRenderState(RenderedTargets, NotRenderedTargets);
	}

	for (const TWeakObjectPtr<AActor>& TargetKey : FlushedTargets)
	{
		const bool* PendingValue = PendingRenderStateReports.Find(TargetKey);
		FPRClientLocalRenderReportState* LocalState = LocalRenderReportStates.Find(TargetKey);
		if (PendingValue != nullptr && LocalState != nullptr)
		{
			LocalState->LastReportedState = *PendingValue
				? EPRClientRenderReportState::Rendered
				: EPRClientRenderReportState::NotRendered;
		}

		PendingRenderStateReports.Remove(TargetKey);
	}
}

bool UPRWorldTickOptimizerReporterComponent::IsLocalReportingComponent() const
{
	const APlayerController* OwnerController = Cast<APlayerController>(GetOwner());
	return IsValid(OwnerController) && OwnerController->IsLocalController();
}

void UPRWorldTickOptimizerReporterComponent::ServerReportRenderState_Implementation(const TArray<AActor*>& RenderedTargets, const TArray<AActor*>& NotRenderedTargets)
{
	APlayerController* OwnerController = Cast<APlayerController>(GetOwner());
	UWorld* World = GetWorld();
	if (!IsValid(OwnerController) || !IsValid(World))
	{
		return;
	}

	if (UPRWorldTickOptimizerSubsystem* TickOptimizer = World->GetSubsystem<UPRWorldTickOptimizerSubsystem>())
	{
		TickOptimizer->UpdateClientRenderStates(OwnerController, RenderedTargets, NotRenderedTargets);
	}
}
