// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (World Tick Optimizer 클라이언트 렌더 상태 보고 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRWorldTickOptimizerReporterComponent.generated.h"

class AActor;

// 클라이언트가 서버에 마지막으로 보고한 렌더 상태
UENUM()
enum class EPRClientRenderReportState : uint8
{
	Unknown,
	NotRendered,
	Rendered
};

// 클라이언트 로컬 렌더 관측과 보고 상태 캐시
USTRUCT()
struct FPRClientLocalRenderReportState
{
	GENERATED_BODY()

	// 서버에 마지막으로 보고한 렌더 상태
	EPRClientRenderReportState LastReportedState = EPRClientRenderReportState::Unknown;

	// 마지막 로컬 평가에서 관측한 렌더 상태
	bool bCurrentRendered = false;

	// 마지막으로 렌더 중이라고 관측한 로컬 시간
	double LastRenderedTimeSeconds = 0.0;

	// 마지막으로 렌더 상태를 검사한 로컬 시간
	double LastObservedTimeSeconds = 0.0;
};

// PlayerController 소유 Server RPC로 렌더 상태 변경분을 보고하는 컴포넌트
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRWorldTickOptimizerReporterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRWorldTickOptimizerReporterComponent();

	/*~ UActorComponent Interface ~*/
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// 로컬 렌더 상태 관측 결과를 보고 큐에 반영
	void PushRenderState(AActor* TargetActor, bool bRendered, double CurrentTimeSeconds);

	// 대상 액터의 로컬 캐시와 전송 대기 상태 제거
	void RemoveTarget(AActor* TargetActor);

	// 무효 대상의 로컬 캐시와 전송 대기 상태 제거
	void CompactInvalidTargets();

protected:
	// 보고 큐의 일부를 Reliable RPC로 서버에 전송
	void FlushPendingRenderReports();

	// 로컬 소유 PlayerController에서 동작 중인지 확인
	bool IsLocalReportingComponent() const;

	// 클라이언트 렌더 상태 변경분 서버 보고
	UFUNCTION(Server, Reliable)
	void ServerReportRenderState(const TArray<AActor*>& RenderedTargets, const TArray<AActor*>& NotRenderedTargets);

protected:
	// 서버 보고 전환을 지연할 클라이언트 미렌더 유지 시간
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|TickOptimization", meta = (ClampMin = "0.0"))
	float ClientNotRenderedGraceTime = 0.5f;

	// Tick마다 서버에 전송할 최대 변경 대상 수
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|TickOptimization", meta = (ClampMin = "1"))
	int32 MaxRenderReportsPerFlush = 20;

	// 대상별 마지막 보고 상태와 로컬 관측 시간
	TMap<TWeakObjectPtr<AActor>, FPRClientLocalRenderReportState> LocalRenderReportStates;

	// 서버 전송 대기 중인 대상별 최종 렌더 상태
	TMap<TWeakObjectPtr<AActor>, bool> PendingRenderStateReports;
};
