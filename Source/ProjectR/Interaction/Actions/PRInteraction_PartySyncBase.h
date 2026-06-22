// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Party Sync 기본 구조 상호작용 액션 실행 로직 구현)
#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "ProjectR/Interaction/PRInteractionAction.h"
#include "PRInteraction_PartySyncBase.generated.h"

class APRPlayerController;
class APRPlayerState;
enum class EPRHUDMessageType : uint8;

// 전투 가능 플레이어 전원 상호작용 판정 기반 클래스
UCLASS(Abstract)
class PROJECTR_API UPRInteraction_PartySyncBase : public UPRInteractionAction
{
	GENERATED_BODY()

public:
	// 파티 동기화 상호작용 기본값 설정
	UPRInteraction_PartySyncBase();

	/*~ UPRInteractionAction Interface ~*/
	// 파티 동기화 참여 시작
	virtual void Execute_Implementation(AActor* Interactor) override;

	// 파티 동기화 참여 종료
	virtual void EndInteraction_Implementation(AActor* Interactor, bool bCanceled) override;

protected:
	TArray<TObjectPtr<APlayerState>> GetPlayerArray() const;
	
	// 전투 가능 플레이어 전원 상호작용 충족 처리
	virtual void HandlePartySyncConditionMet();

	// 개별 플레이어 상호작용 시작 피드백 처리
	virtual void NotifyPartySyncInteractionStarted(AActor* Interactor);

	// 개별 플레이어 상호작용 종료 피드백 처리
	virtual void NotifyPartySyncInteractionEnded(AActor* Interactor, bool bCanceled);

	// 이동 진행 또는 UI 대기 같은 완료 이후 상태의 중복 처리 차단 여부
	virtual bool IsPartySyncActionLocked() const;

	// 파티 동기화 판정 타이머 정리
	void ClearPartySyncCheckTimer();

	// 파티 동기화 대기 HUD 메시지와 갱신 예약 제거
	void ClearPartySyncWaitingMessages();

private:
	// 전투 가능 플레이어 전원 상호작용 상태 검사
	void CheckPartySyncCondition();

	// 상호작용 플레이어 수 반환
	int32 CountInteractingPlayers() const;

	// 행동 가능 플레이어 수 반환
	int32 CountFightCapablePlayers() const;

	// 파티 동기화 대기 HUD 메시지 갱신
	void RefreshPartySyncWaitingMessages();

	// 플레이어 상태 기준 상호작용 여부 확인
	bool IsPlayerInteracting(const APRPlayerState* PlayerState) const;

	// 플레이어 상태 기준 컨트롤러 조회
	APRPlayerController* ResolvePlayerController(const APRPlayerState* PlayerState) const;

protected:
	// 파티 동기화 판정 지연
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|PartySync", meta = (ClampMin = "0.0"))
	float PartySyncCheckDelay = 0.3f;

	// 파티 동기화 대기 메시지 표시 지연
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Interaction|PartySync", meta = (ClampMin = "0.1"))
	float PartySyncMessageDelay = 0.3f;

private:
	// 전원 상호작용 판정 예약 타이머
	FTimerHandle PartySyncCheckTimerHandle;

	// 대기 HUD 메시지 갱신 예약 타이머
	FTimerHandle PartySyncMessageTimerHandle;
};
