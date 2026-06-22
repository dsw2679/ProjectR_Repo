// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (Boss Pattern Actor 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PRBossPatternActor.generated.h"

class APRBossBaseCharacter;
class USceneComponent;

// 보스 패턴이 생성하는 지속형 Helper Actor의 공통 베이스다.
// 포털, 검 하자드, 장판처럼 본체와 분리되어 존재하는 패턴 오브젝트가 이 클래스를 공유한다.
UCLASS(Abstract, Blueprintable)
class PROJECTR_API APRBossPatternActor : public AActor
{
	GENERATED_BODY()

public:
	APRBossPatternActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버에서 보스 패턴 Actor의 소유 보스와 주 타겟을 초기화한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	virtual void InitializeBossPatternActor(APRBossBaseCharacter* InOwnerBoss, AActor* InPatternTarget);

	// 이 패턴 Actor를 생성한 보스를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss")
	APRBossBaseCharacter* GetOwnerBoss() const { return OwnerBoss; }

	// 이 패턴 Actor가 참조하는 주 타겟을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss")
	AActor* GetPatternTarget() const { return PatternTarget; }

	// 서버에서 패턴 Actor의 정상 만료를 요청한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	virtual void RequestPatternActorExpire();

	// 서버에서 패턴 Actor를 즉시 취소하고 정리한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	virtual void CancelPatternActor();

	// Phase 전환 정리 중 이 Actor를 취소할지 결정한다.
	virtual bool ShouldCancelOnBossPhaseTransition() const { return true; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 복제된 초기화 상태가 클라이언트에 도착했을 때 연출 연결점을 호출한다.
	UFUNCTION()
	void OnRep_PatternActorInitialized();

	// 패턴 Actor를 만료 처리하고 제거한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void ExpirePatternActor();

	// 패턴 Actor 초기화 직후 BP 연출을 연결하는 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss")
	void BP_OnPatternActorInitialized();

	// 패턴 Actor가 제거되기 직전 BP 연출을 연결하는 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss")
	void BP_OnPatternActorExpired();

protected:
	// 패턴 Actor의 기본 루트 컴포넌트다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	TObjectPtr<USceneComponent> SceneRoot;

	// 이 패턴 Actor를 생성한 보스다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	TObjectPtr<APRBossBaseCharacter> OwnerBoss;

	// 패턴이 바라보거나 추적할 주 타겟이다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ProjectR|AI|Boss")
	TObjectPtr<AActor> PatternTarget;

	// 0보다 크면 BeginPlay 후 지정 시간 뒤 자동 제거한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss", meta = (ClampMin = "0.0"))
	float PatternLifeSpan = 0.0f;

private:
	// OwnerBoss의 활성 패턴 목록에서 이 Actor를 안전하게 제거한다.
	void UnregisterFromOwnerBoss();

private:
	// 초기화 완료 상태다. 클라이언트가 BP 초기화 이벤트를 놓치지 않도록 복제한다.
	UPROPERTY(ReplicatedUsing = OnRep_PatternActorInitialized)
	bool bPatternActorInitialized = false;
};
