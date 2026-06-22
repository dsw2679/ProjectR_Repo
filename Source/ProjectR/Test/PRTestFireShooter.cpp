// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Test Fire Shooter 구현)
#include "PRTestFireShooter.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogPRTestFire);

APRTestFireShooter::APRTestFireShooter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	SetNetUpdateFrequency(30.f);
	SetMinNetUpdateFrequency(10.f);
}

/*~ AActor Interface ~*/

void APRTestFireShooter::BeginPlay()
{
	Super::BeginPlay();
	ScheduleServerTestStop();
	TryAutoStartFiring();
}

void APRTestFireShooter::OnRep_Owner()
{
	Super::OnRep_Owner();

	TryAutoStartFiring();
}

void APRTestFireShooter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimerHandle);
		World->GetTimerManager().ClearTimer(AutoStopTimerHandle);
	}

	// 서버 종료 시 최종 통계 출력
	if (HasAuthority())
	{
		bServerTestStopped = true;

		if (!bFinalStatsReported)
		{
			ReportServerStats();
		}

		for (const TPair<TWeakObjectPtr<APlayerController>, FPRTestFireServerStats>& Pair : ServerStatsByShooter)
		{
			const FPRTestFireServerStats& Stats = Pair.Value;
			const double AvgLatency = Stats.ReceivedCount > 0 ? Stats.TotalLatency / Stats.ReceivedCount : 0.0;
			const double AvgIntervalDelay = Stats.TotalIntervalDelaySampleCount > 0
				? Stats.TotalIntervalDelay / Stats.TotalIntervalDelaySampleCount
				: 0.0;
			UE_LOG(LogPRTestFire, Log,
				TEXT("[FINAL] Shooter=%s Recv=%d Missing=%d MaxGap=%d AvgClockLatency=%.1fms MaxClockLatency=%.1fms AvgIntervalDelay=%.1fms MaxIntervalDelay=%.1fms"),
				*GetNameSafe(Pair.Key.Get()),
				Stats.ReceivedCount, Stats.MissingCount, Stats.MaxGap,
				AvgLatency * 1000.0, Stats.MaxLatency * 1000.0,
				AvgIntervalDelay * 1000.0, Stats.IntervalDelayMax * 1000.0);
		}

		bFinalStatsReported = true;
	}

	Super::EndPlay(EndPlayReason);
}

void APRTestFireShooter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}

	if (bServerTestStopped)
	{
		return;
	}

	// 서버측 1초 단위 통계 출력
	ServerReportAccumulator += DeltaSeconds;
	if (ServerReportAccumulator >= 1.f)
	{
		ServerReportAccumulator = 0.f;
		ReportServerStats();
	}
}

/*~ APRTestFireShooter Interface ~*/

void APRTestFireShooter::SetFireTestConfig(
	float InFireRateRPM, bool bInAutoStart, bool bInUseReliable, float InTestTime)
{
	FireRateRPM = FMath::Max(0.f, InFireRateRPM);
	bAutoStart = bInAutoStart;
	bUseReliable = bInUseReliable;
	AutoStopAfterSeconds = FMath::Max(0.f, InTestTime);
	bTestConfigInitialized = true;

	if (HasAuthority())
	{
		ScheduleServerTestStop();
		ForceNetUpdate();
	}
}

void APRTestFireShooter::OnRep_TestConfig()
{
	TryAutoStartFiring();
}

void APRTestFireShooter::ScheduleServerTestStop()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoStopTimerHandle);

		if (AutoStopAfterSeconds > 0.f)
		{
			World->GetTimerManager().SetTimer(
				AutoStopTimerHandle, this, &APRTestFireShooter::HandleServerTestStop, AutoStopAfterSeconds, false);
		}
	}
}

void APRTestFireShooter::HandleServerTestStop()
{
	if (!HasAuthority() || bServerTestStopped)
	{
		return;
	}

	bServerTestStopped = true;
	StopFiring();
	ClientNotifyTestStop();
	ReportServerStats();

	UE_LOG(LogPRTestFire, Log,
		TEXT("[Server] TestStopped ServerTime=%.3f Shooter=%s TestTime=%.1f"),
		FPlatformTime::Seconds(),
		*GetNameSafe(GetOwner()),
		AutoStopAfterSeconds);
}

void APRTestFireShooter::ClientNotifyTestStop_Implementation()
{
	StopFiring();
}

void APRTestFireShooter::TryAutoStartFiring()
{
	if (!HasAuthority() && !bTestConfigInitialized)
	{
		return;
	}

	if (!bAutoStart)
	{
		return;
	}

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const APlayerController* OwnerPC = Cast<APlayerController>(GetOwner());
	if (!IsValid(OwnerPC) || !OwnerPC->IsLocalController())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (World->GetTimerManager().IsTimerActive(FireTimerHandle))
		{
			return;
		}
	}

	StartFiring();
}

void APRTestFireShooter::StartFiring()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float Interval = FireRateRPM > 0.f ? (60.f / FireRateRPM) : 0.1f;

	World->GetTimerManager().SetTimer(
		FireTimerHandle, this, &APRTestFireShooter::FireOnce, Interval, true);
}

void APRTestFireShooter::StopFiring()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimerHandle);
		World->GetTimerManager().ClearTimer(AutoStopTimerHandle);
	}
}

void APRTestFireShooter::FireOnce()
{
	FPRTestFireReport Report;
	Report.ShotId = NextShotId++;
	const double Now = FPlatformTime::Seconds();
	Report.ClientSendTime = Now;
	Report.HitLocation = GetActorLocation();
	Report.HitBone = TEXT("head");

	if (bUseReliable)
	{
		ServerReportHitReliable(Report);
	}
	else
	{
		ServerReportHitUnreliable(Report);
	}
}

void APRTestFireShooter::ServerReportHitReliable_Implementation(const FPRTestFireReport& Report)
{
	HandleServerReport(Report);
}

void APRTestFireShooter::ServerReportHitUnreliable_Implementation(const FPRTestFireReport& Report)
{
	HandleServerReport(Report);
}

void APRTestFireShooter::HandleServerReport(const FPRTestFireReport& Report)
{
	if (bServerTestStopped)
	{
		return;
	}

	APlayerController* OwnerPC = Cast<APlayerController>(GetOwner());
	if (!IsValid(OwnerPC))
	{
		UE_LOG(LogPRTestFire, Warning, TEXT("[Server] HandleServerReport OwnerPC 없음"));
		return;
	}

	TWeakObjectPtr<APlayerController> Key(OwnerPC);

	FPRTestFireServerStats& Stats = ServerStatsByShooter.FindOrAdd(Key);
	const double ServerReceiveTime = FPlatformTime::Seconds();

	Stats.ReceivedCount++;

	// ShotId 갭 계산. Reliable 인 경우 갭이 0 이어야 정상
	if (Stats.LastShotId >= 0)
	{
		const int32 Gap = static_cast<int32>(Report.ShotId) - Stats.LastShotId - 1;
		if (Gap > 0)
		{
			Stats.MissingCount += Gap;
			Stats.MaxGap = FMath::Max(Stats.MaxGap, Gap);
		}
	}
	Stats.LastShotId = static_cast<int32>(Report.ShotId);

	// 송수신 지연 측정. 시계 동기화가 완벽하지 않으므로 절대값보다 추세 관찰용
	const double Latency = ServerReceiveTime - Report.ClientSendTime;
	if (Latency > 0.0)
	{
		Stats.TotalLatency += Latency;
		Stats.MaxLatency = FMath::Max(Stats.MaxLatency, Latency);
	}

	// 연속 샷 간 간격 차이로 추가 지연 측정. 클라/서버 시계 오프셋 영향 배제
	if (Stats.LastClientSendTime >= 0.0 && Stats.LastServerReceiveTime >= 0.0)
	{
		const double ClientInterval = Report.ClientSendTime - Stats.LastClientSendTime;
		const double ServerInterval = ServerReceiveTime - Stats.LastServerReceiveTime;

		if (ClientInterval > KINDA_SMALL_NUMBER)
		{
			const double IntervalDelay = ServerInterval - ClientInterval;
			if (IntervalDelay >= 0.0)
			{
				Stats.TotalIntervalDelay += IntervalDelay;
				Stats.TotalIntervalDelaySampleCount++;
				Stats.IntervalDelaySum += IntervalDelay;
				Stats.IntervalDelaySampleCount++;
				Stats.IntervalDelayMax = FMath::Max(Stats.IntervalDelayMax, IntervalDelay);
				Stats.IntervalDelayMaxThisReport = FMath::Max(Stats.IntervalDelayMaxThisReport, IntervalDelay);
			}
		}
	}

	Stats.LastClientSendTime = Report.ClientSendTime;
	Stats.LastServerReceiveTime = ServerReceiveTime;
}

void APRTestFireShooter::ReportServerStats()
{
	const double Now = FPlatformTime::Seconds();

	for (TPair<TWeakObjectPtr<APlayerController>, FPRTestFireServerStats>& Pair : ServerStatsByShooter)
	{
		FPRTestFireServerStats& Stats = Pair.Value;
		if (Stats.ReceivedCount <= 0)
		{
			continue;
		}

		const int32 IntervalRecv = Stats.ReceivedCount - Stats.ReceivedCountAtLastReport;
		const double Elapsed = Stats.LastReportTime > 0.0 ? Now - Stats.LastReportTime : 1.0;
		const double Rate = Elapsed > 0.0 ? IntervalRecv / Elapsed : 0.0;

		const double AvgLatency = Stats.ReceivedCount > 0 ? Stats.TotalLatency / Stats.ReceivedCount : 0.0;
		const double IntervalAvgDelay = Stats.IntervalDelaySampleCount > 0
			? Stats.IntervalDelaySum / Stats.IntervalDelaySampleCount
			: -1.0;
		const double IntervalMaxDelay = Stats.IntervalDelayMaxThisReport;
		double PingMs = -1.0;
		if (APlayerController* OwnerPC = Pair.Key.Get())
		{
			if (IsValid(OwnerPC->PlayerState))
			{
				PingMs = OwnerPC->PlayerState->GetPingInMilliseconds();
			}
		}

		UE_LOG(LogPRTestFire, Log,
			TEXT("[Server] ServerTime=%.3f Shooter=%s Recv/s=%.1f Total=%d Missing=%d MaxGap=%d Ping=%s AvgClockLatency=%.1fms MaxClockLatency=%.1fms IntervalAvgDelay=%s IntervalMaxDelay=%s"),
			Now,
			*GetNameSafe(Pair.Key.Get()),
			Rate, Stats.ReceivedCount, Stats.MissingCount, Stats.MaxGap,
			PingMs >= 0.0 ? *FString::Printf(TEXT("%.1fms"), PingMs) : TEXT("N/A"),
			AvgLatency * 1000.0, Stats.MaxLatency * 1000.0,
			IntervalAvgDelay >= 0.0 ? *FString::Printf(TEXT("%.1fms"), IntervalAvgDelay * 1000.0) : TEXT("N/A"),
			IntervalMaxDelay >= 0.0 ? *FString::Printf(TEXT("%.1fms"), IntervalMaxDelay * 1000.0) : TEXT("N/A"));

		Stats.ReceivedCountAtLastReport = Stats.ReceivedCount;
		Stats.LastReportTime = Now;
		Stats.IntervalDelaySum = 0.0;
		Stats.IntervalDelaySampleCount = 0;
		Stats.IntervalDelayMaxThisReport = -1.0;
	}
}

void APRTestFireShooter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRTestFireShooter, FireRateRPM);
	DOREPLIFETIME(APRTestFireShooter, bAutoStart);
	DOREPLIFETIME(APRTestFireShooter, bUseReliable);
	DOREPLIFETIME(APRTestFireShooter, AutoStopAfterSeconds);
	DOREPLIFETIME(APRTestFireShooter, bTestConfigInitialized);
}
