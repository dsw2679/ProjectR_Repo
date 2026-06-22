// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Test Load Actor 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PRTestLoadActor.generated.h"

// 발사 RPC 외 네트워크 부하를 시뮬레이션하는 더미 액터
// 주기적으로 Reliable Multicast RPC 송신 및 Replicated 프로퍼티 변경
// 실제 게임의 Montage/Cue/Attribute 트래픽 모사 용도
UCLASS()
class PROJECTR_API APRTestLoadActor : public AActor
{
	GENERATED_BODY()

public:
	APRTestLoadActor();

	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// 서버 타이머 콜백. Reliable Multicast 1회 송신
	void FireMulticast();

	// 서버 타이머 콜백. Replicated 프로퍼티 변동
	void TickReplicatedProperty();

	// 모든 클라에 전파되는 Reliable Multicast. Montage/Cue 모사
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDummyEvent(int32 EventId, const FVector_NetQuantize& Location);

public:
	// Reliable Multicast 송신 주기(초). 0 이하면 송신 안 함
	UPROPERTY(EditAnywhere, Category = "PR Test|Load")
	float MulticastIntervalSeconds = 0.5f;

	// Replicated 프로퍼티 변경 주기(초). 0 이하면 변경 안 함
	UPROPERTY(EditAnywhere, Category = "PR Test|Load")
	float PropertyTickIntervalSeconds = 0.1f;

protected:
	// 변동 대상 Replicated 프로퍼티. Attribute 변경 모사용 더미값
	UPROPERTY(Replicated)
	float ReplicatedHealth = 100.f;

	// 변동 대상 Replicated 프로퍼티. Tag/State 변경 모사용 더미값
	UPROPERTY(Replicated)
	int32 ReplicatedStateCounter = 0;

private:
	// Multicast 시퀀스 카운터
	int32 NextEventId = 0;

	// 서버 타이머 핸들
	FTimerHandle MulticastTimerHandle;
	FTimerHandle PropertyTimerHandle;
};
