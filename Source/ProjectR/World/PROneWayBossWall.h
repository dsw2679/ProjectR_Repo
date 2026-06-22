// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (월드 배치용 One Way Boss Wall 및 관련 시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PROneWayBossWall.generated.h"

class UBoxComponent;
class UPrimitiveComponent;

UCLASS()
class PROJECTR_API APROneWayBossWall : public AActor
{
	GENERATED_BODY()

public:
	APROneWayBossWall();

	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// 보스 처치 이벤트를 받아 장벽 차단 해제
	UFUNCTION()
	void HandleBossDefeated(FName DefeatedBossId);

	// 입장 방향 트리거 진입 시 Pawn이 벽을 통과할 수 있도록 이동 무시 설정
	UFUNCTION()
	void OnEntryTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 보스방 내부 트리거 이탈 시 입장 트리거 중첩 여부에 따른 이동 무시 해제
	UFUNCTION()
	void OnExitTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 복제된 활성 상태를 클라이언트 충돌 상태에 반영
	UFUNCTION()
	void OnRep_BarrierActive();

	// 현재 월드 진행 상태에 맞춰 장벽 상태 갱신
	void RefreshBarrierState();

	// 현재 활성 상태를 콜리전과 무시 목록에 반영
	void ApplyBarrierState();

	// Pawn에 속한 모든 PrimitiveComponent가 차단 콜리전만 무시하도록 설정
	void SetPawnIgnoreWall(AActor* Actor, bool bIgnore);

	// 추적 중인 Pawn의 이동 무시 목록을 전부 정리
	void ClearIgnoredPawns();

protected:
	// 퇴장 방향을 실제로 차단하는 블로킹 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|BossWall")
	TObjectPtr<UBoxComponent> BlockingCollision;

	// 입장 방향에서 벽 무시를 시작하는 트리거
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|BossWall")
	TObjectPtr<UBoxComponent> EntrySideTrigger;

	// 보스방 내부에서 벽 무시를 종료하는 트리거
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|BossWall")
	TObjectPtr<UBoxComponent> ExitSideTrigger;

	// 해제 조건으로 사용할 보스 식별자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|BossWall")
	FName BossId;

	// 보스 처치 후 장벽을 비활성화할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|BossWall")
	bool bDeactivateWhenBossDefeated = true;

	// 장벽 활성 상태
	UPROPERTY(ReplicatedUsing = OnRep_BarrierActive, VisibleInstanceOnly, Category = "ProjectR|BossWall")
	bool bBarrierActive = true;

private:
	// 현재 이 장벽을 이동 무시 중인 Pawn 목록
	TArray<TWeakObjectPtr<AActor>> IgnoredPawns;
};
