// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Test Fire Shooter 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PRTestFireShooter.generated.h"

class APlayerController;

DECLARE_LOG_CATEGORY_EXTERN(LogPRTestFire, Log, All);

// 발사 RPC 페이로드. 실제 무기 시스템 페이로드의 크기/형태를 모사
USTRUCT()
struct FPRTestFireReport
{
	GENERATED_BODY()

	// 발사 시퀀스 식별자. 서버에서 유실/순서 추적 용도
	UPROPERTY()
	uint32 ShotId = 0;

	// 클라 송신 시점. 도달 지연 측정 용도
	UPROPERTY()
	double ClientSendTime = 0.0;

	// 명중 위치. 페이로드 크기 모사용 더미값
	UPROPERTY()
	FVector_NetQuantize HitLocation = FVector::ZeroVector;

	// 명중 본 이름. 페이로드 크기 모사용 더미값
	UPROPERTY()
	FName HitBone = NAME_None;
};

// 서버측 누적 통계. 1초마다 로그 출력
USTRUCT()
struct FPRTestFireServerStats
{
	GENERATED_BODY()

	// 총 수신 RPC 수
	int32 ReceivedCount = 0;

	// 마지막으로 처리한 ShotId. 다음 수신값과 비교하여 갭 계산
	int32 LastShotId = -1;

	// ShotId 시퀀스 갭 합계. 유실 또는 순서 지연 추정치
	int32 MissingCount = 0;

	// 단일 갭 최대치
	int32 MaxGap = 0;

	// 누적 송수신 지연 합계와 최대치(초)
	double TotalLatency = 0.0;
	double MaxLatency = 0.0;

	// 연속 샷 간 추가 지연 통계(초). 클라/서버 시계 오프셋 영향 배제 목적
	double TotalIntervalDelay = 0.0;
	int32 TotalIntervalDelaySampleCount = 0;
	double IntervalDelaySum = 0.0;
	double IntervalDelayMax = 0.0;
	int32 IntervalDelaySampleCount = 0;

	// 최근 1초 리포트 구간 최대 추가 지연(초)
	double IntervalDelayMaxThisReport = -1.0;

	// 연속 샷 간 지연 계산을 위한 직전 시각(초)
	double LastClientSendTime = -1.0;
	double LastServerReceiveTime = -1.0;

	// 직전 통계 출력 시점
	double LastReportTime = 0.0;

	// 직전 통계 출력 시점 기준 수신 카운트 스냅샷
	int32 ReceivedCountAtLastReport = 0;
};

// 고RPM 발사 RPC 부하 테스트용 액터
// 클라가 소유하며 설정된 RPM으로 ServerReportHit를 송신
// Reliable 또는 Unreliable 채널 선택 가능
UCLASS()
class PROJECTR_API APRTestFireShooter : public AActor
{
	GENERATED_BODY()

public:
	APRTestFireShooter();

	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void OnRep_Owner() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 클라이언트측에서 발사 시작. 오너 클라에서만 동작
	void StartFiring();

	// 클라이언트측에서 발사 중단
	void StopFiring();

	// GameMode 테스트 설정 적용
	void SetFireTestConfig(float InFireRateRPM, bool bInAutoStart, bool bInUseReliable, float InTestTime);

protected:
	// 테스트 설정 복제 완료 처리
	UFUNCTION()
	void OnRep_TestConfig();

	// 서버 테스트 종료 타이머 설정
	void ScheduleServerTestStop();

	// 서버 테스트 종료 처리
	void HandleServerTestStop();

	// 서버 종료 시 오너 클라 발사 중단 통지
	UFUNCTION(Client, Reliable)
	void ClientNotifyTestStop();

	// 로컬 소유 컨트롤러 여부 확인 후 자동 발사 시작 시도
	void TryAutoStartFiring();

	// 클라 타이머 콜백. 단일 RPC 송신
	void FireOnce();

	// Reliable 채널 RPC. bUseReliable 가 true 일 때 사용
	UFUNCTION(Server, Reliable)
	void ServerReportHitReliable(const FPRTestFireReport& Report);

	// Unreliable 채널 RPC. bUseReliable 가 false 일 때 사용
	UFUNCTION(Server, Unreliable)
	void ServerReportHitUnreliable(const FPRTestFireReport& Report);

	// 서버측 공통 처리. 통계 누적
	void HandleServerReport(const FPRTestFireReport& Report);

	// 1초 단위로 누적 통계 로그 출력
	void ReportServerStats();

protected:
	// 분당 발사 수. 600 RPM = 10 발/초, 900 RPM = 15 발/초
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_TestConfig, Category = "PR Test|Fire")
	float FireRateRPM = 900.f;

	// BeginPlay 직후 자동 발사 시작 여부
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_TestConfig, Category = "PR Test|Fire")
	bool bAutoStart = true;

	// true 면 Reliable, false 면 Unreliable RPC 사용
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_TestConfig, Category = "PR Test|Fire")
	bool bUseReliable = true;

	// 자동 발사 종료 시각(초). 0 이하면 무한 발사
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_TestConfig, Category = "PR Test|Fire")
	float AutoStopAfterSeconds = 0.f;

	// 테스트 설정 수신 여부
	UPROPERTY(ReplicatedUsing = OnRep_TestConfig)
	bool bTestConfigInitialized = false;

private:
	// 클라이언트측 다음 송신 ShotId
	uint32 NextShotId = 0;

	// 클라이언트 발사 타이머 핸들
	FTimerHandle FireTimerHandle;

	// 서버 테스트 종료 타이머 핸들
	FTimerHandle AutoStopTimerHandle;

	// 서버측 송신자별 통계. Key 는 오너 PlayerController
	TMap<TWeakObjectPtr<APlayerController>, FPRTestFireServerStats> ServerStatsByShooter;

	// 서버 통계 출력 누적 시간
	float ServerReportAccumulator = 0.f;

	// 서버 테스트 종료 상태
	bool bServerTestStopped = false;

	// 서버 최종 통계 출력 완료 상태
	bool bFinalStatsReported = false;
};
