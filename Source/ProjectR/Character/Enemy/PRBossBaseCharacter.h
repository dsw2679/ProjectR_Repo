// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (보스 전용 UI 체력 바 및 페이즈 BGM 연동 구현)
// Author: 배유찬 (보스 조우 및 월드 리셋, 약점 부위 히트 판정 구조 구축)
// Author: 손승우 (보스 AI 비헤이비어 제어 및 페어린 보스 패턴 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/System/PREventTypes.h"
#include "PRBossBaseCharacter.generated.h"

class UPRAbilitySet;
class APRBossPatternActor;
class APRBossBaseCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPRPhaseChangedSignature, EPRBossPhase, OldPhase, EPRBossPhase, NewPhase);

/**
 * 보스 조우 이벤트 페이로드.
 * Event.Boss.Encounter.Begin 발송 시 동반된다.
 */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRBossEncounterEventPayload : public FPREventPayload
{
	GENERATED_BODY()

	// 조우 대상 보스 인스턴스
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<APRBossBaseCharacter> Boss = nullptr;
};

/**
 * 보스 페이즈 변경 이벤트 페이로드.
 * Event.Boss.PhaseChanged 발송 시 동반
 */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRBossPhaseChangedEventPayload : public FPREventPayload
{
	GENERATED_BODY()

	// 페이즈가 변경된 보스 인스턴스
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<APRBossBaseCharacter> Boss = nullptr;

	// 변경 전 페이즈
	UPROPERTY(BlueprintReadWrite)
	EPRBossPhase OldPhase = EPRBossPhase::Phase1;

	// 변경 후 페이즈
	UPROPERTY(BlueprintReadWrite)
	EPRBossPhase NewPhase = EPRBossPhase::Phase1;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRBossBGMPatternCueEventPayload : public FPREventPayload
{
	GENERATED_BODY()

	// BGM Cue를 발생시킨 보스 인스턴스
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<APRBossBaseCharacter> Boss = nullptr;

	// DataAsset PatternCues와 매칭할 Cue 태그
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag CueTag;
};

// 보스 몬스터 공통 베이스다.
// 일반 적 베이스 위에 페이즈 상태, 페이즈별 AbilitySet, 체력 비율 기반 전환을 얹는다.
UCLASS(Abstract)
class PROJECTR_API APRBossBaseCharacter : public APREnemyBaseCharacter
{
	GENERATED_BODY()

public:
	APRBossBaseCharacter();

	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ APawn Interface ~*/
	virtual void PossessedBy(AController* NewController) override;

	/*~ APRCharacterBase Interface ~*/
	virtual EPRCharacterRole GetCharacterRole() const override { return EPRCharacterRole::Boss; }

	/*~ IPRCombatInterface ~*/
	virtual void OnPostDamageApplied(const FPRDamageAppliedContext& Context) override;

	// 플레이어 전투 교전 상태 진입에 따른 보스 HUD 조우 시작 요청
	virtual void RequestBossEncounterBegin();

	EPRBossPhase GetCurrentPhase() const { return CurrentPhase; }

	// 체력 비율이 바뀔 때 호출해 다음 페이즈 조건을 검사한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void OnHealthRatioChanged(float NewRatio);

	// 페이즈 전환 연출/Ability를 시작하기 위한 진입점이다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void BeginPhaseTransition(EPRBossPhase NewPhase);

	// 전환 연출이 끝난 뒤 실제 페이즈 값을 확정한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void CommitPhaseTransition(EPRBossPhase NewPhase);

	// 실제 페이즈 확정 없이 BGM 전환에 사용할 목표 페이즈를 예고
	void BroadcastBossBGMPhasePreview(EPRBossPhase PreviewPhase);

	// 각 머신의 EventManager로 BGM 페이즈 예고 이벤트를 발행
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastBossBGMPhasePreview(EPRBossPhase PreviewPhase);

	// BGM 특수패턴 Cue를 각 머신의 EventManager로 발행
	void BroadcastBossBGMPatternCue(FGameplayTag CueTag);

	// 각 머신의 EventManager로 BGM 특수패턴 Cue 이벤트를 발행
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastBossBGMPatternCue(FGameplayTag CueTag);

	// 서버 기준 보스 처치 확정 이벤트 멀티캐스트
	void BroadcastBossDefeated();

	// 각 머신 EventManager 보스 처치 확정 이벤트 발행
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastBossDefeated();

	// 보스가 생성한 지속형 패턴 Actor를 활성 목록에 등록한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void RegisterBossPatternActor(APRBossPatternActor* Actor);

	// 지속형 패턴 Actor가 만료/파괴될 때 활성 목록에서 제거한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void UnregisterBossPatternActor(APRBossPatternActor* Actor);

	// 페이즈 전환/사망/그로기 등 보스 상태 전환 시 살아 있는 패턴 Actor를 모두 취소한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void CancelAllBossPatternActors();

	// Phase 전환 중에도 유지해야 하는 지속형 패턴 Actor를 제외하고 취소한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void CancelBossPatternActorsForPhaseTransition();

	// 보스 HUD에 표시할 이름을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss")
	FText GetBossDisplayName() const;

	// 보스 HUD 페이즈 마커에 사용할 HP 임계값 비율 목록을 반환한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss")
	void GetPhaseThresholdRatios(TArray<float>& OutRatios) const;

public:
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|AI|Boss")
	FPRPhaseChangedSignature OnPhaseChanged;

protected:
	/*~ APRCharacterBase Interface ~*/
	virtual void HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool TagExists) override;

	// EventManager로 보스 페이즈 변경을 발행
	void BroadcastBossPhaseChangedEvent(EPRBossPhase OldPhase, EPRBossPhase NewPhase);

	// EventManager로 보스 BGM 페이즈 예고를 발행
	void BroadcastBossBGMPhasePreviewEvent(EPRBossPhase PreviewPhase);

	// EventManager로 보스 BGM 특수패턴 Cue 발행
	void BroadcastBossBGMPatternCueEvent(FGameplayTag CueTag);

	// EventManager로 보스 처치 확정 이벤트 발행
	void BroadcastBossDefeatedEvent();

	UFUNCTION()
	void OnRep_CurrentPhase(EPRBossPhase OldPhase);

protected:
	// 현재 확정된 보스 페이즈다. 클라이언트 연출 동기화를 위해 복제한다.
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase, VisibleInstanceOnly, Category = "ProjectR|AI|Boss")
	EPRBossPhase CurrentPhase = EPRBossPhase::Phase1;

	// 페이즈 전환 시 PhaseAbilitySets를 grant/remove할지 여부다. 끄면 패턴 페이즈 제어는 PatternData가 전담한다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Boss|Ability")
	bool bUsePhaseAbilitySets = true;

	// 페이즈마다 추가로 부여할 AbilitySet이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Boss|Ability", meta = (EditCondition = "bUsePhaseAbilitySets"))
	TMap<EPRBossPhase, TObjectPtr<UPRAbilitySet>> PhaseAbilitySets;

	// 체력 비율이 이 값 이하가 되면 해당 페이즈 전환 후보가 된다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Boss")
	TMap<EPRBossPhase, float> PhaseThresholdRatios;

	// 현재 페이즈에서 부여한 Ability/GE 핸들이다.
	UPROPERTY()
	FPRAbilitySetHandles CurrentPhaseHandles;

	// 전환 연출 중 목표 페이즈를 보관한다.
	EPRBossPhase PendingPhase = EPRBossPhase::Phase1;

	// Portal/Godfall/Shift처럼 Ability 종료 후에도 남을 수 있는 Helper Actor 목록이다.
	UPROPERTY(Transient)
	TArray<TObjectPtr<APRBossPatternActor>> ActivePatternActors;
};
