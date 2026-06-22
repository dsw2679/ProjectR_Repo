// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (전투 Engagement 서브시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PRCombatEngagementSubsystem.generated.h"

class AActor;

// 적 전투 후보 보고를 플레이어별 전투 교전 이벤트로 집계하는 서버 전용 서브시스템
UCLASS()
class PROJECTR_API UPRCombatEngagementSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/*~ UWorldSubsystem Interface ~*/
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	// 적이 교전 후보로 유지 중인 플레이어를 보고
	void ReportCombatEngagedCandidate(AActor* Reporter, AActor* Candidate);

protected:
	// 집계된 플레이어에게 전투 교전 이벤트를 전송
	void FlushCombatEngagedEvents();

	// 서버 월드에서만 집계 타이머 시작
	void StartFlushTimer();

	// 집계 타이머 정리
	void StopFlushTimer();

	// 교전 이벤트를 받을 수 있는 플레이어 후보 검증
	bool IsValidEngagedCandidate(const AActor* Candidate) const;

private:
	TSet<TWeakObjectPtr<AActor>> PendingEngagedPlayers;

	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AActor>> ReporterByPlayer;

	FTimerHandle FlushTimerHandle;

	float FlushInterval = 1.0f;
};
