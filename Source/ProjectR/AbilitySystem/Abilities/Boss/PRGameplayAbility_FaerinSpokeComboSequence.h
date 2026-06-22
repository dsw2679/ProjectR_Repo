// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Spoke 콤보 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossPatternBase.h"
#include "PRGameplayAbility_FaerinSpokeComboSequence.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UPRFaerinWeaponVisualComponent;

// Faerin Spoke 콤보의 현재 진행 단계를 나타낸다.
UENUM()
enum class EPRFaerinSpokeComboStage : uint8
{
	None,
	Opening,
	FollowUp,
	SlamStart,
	SlamLoop,
	SlamEnd
};

// Faerin Spoke 콤보의 R/L 방향을 나타낸다.
UENUM()
enum class EPRFaerinSpokeComboDirection : uint8
{
	None,
	Right,
	Left
};

// Faerin Spoke 콤보 히트 윈도우가 현재 어떤 피해 값을 사용할지 구분한다.
UENUM()
enum class EPRFaerinSpokeComboDamageWindow : uint8
{
	None,
	Spoke,
	SlamEnd
};

// Faerin Spoke 콤보 첫 1타 방향 선택 방식이다.
UENUM()
enum class EPRFaerinSpokeComboFirstDirectionPolicy : uint8
{
	TargetSide,
	Random,
	ForceRight,
	ForceLeft
};

// Faerin Spoke 1타 이후 어떤 분기로 이어질지 결정하는 방식이다.
UENUM()
enum class EPRFaerinSpokeComboBranchPolicy : uint8
{
	OppositeOnly,
	SlamOnly,
	RandomOppositeOrSlam
};

// Faerin Phase1 Spoke R/L 1타, 반대 방향 follow-up, Spoke Slam을 단일 몽타주/Ability 흐름으로 실행한다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_FaerinSpokeComboSequence : public UPRGameplayAbility_BossPatternBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_FaerinSpokeComboSequence();

	/*~ UGameplayAbility Interface ~*/
public:
	// ShiftPlayerClose 이후 강제 연계 상태에서만 SpokeCombo 쿨다운 검사를 통과시킨다.
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// 현재 타깃 기준으로 첫 Spoke 방향을 고르고 단일 Spoke 콤보 몽타주를 시작한다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 몽타주, 이동, 타이머, 검 hitbox 상태를 정리한다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// Spoke 콤보 실행에 필요한 보스, 타깃, 무기 시각 컴포넌트를 준비한다.
	bool InitializeSpokeCombo();

	// 현재 설정에 따라 첫 Spoke 방향을 결정한다.
	EPRFaerinSpokeComboDirection ResolveFirstDirection() const;

	// 현재 방향의 반대 방향을 반환한다.
	EPRFaerinSpokeComboDirection GetOppositeDirection(EPRFaerinSpokeComboDirection Direction) const;

	// 방향에 대응하는 시작 섹션 이름을 반환한다.
	FName GetOpeningSectionName(EPRFaerinSpokeComboDirection Direction) const;

	// 방향에 대응하는 follow-up 섹션 이름을 반환한다.
	FName GetFollowUpSectionName(EPRFaerinSpokeComboDirection Direction) const;

	// 지정한 섹션이 몽타주에 존재하는지 확인한다.
	bool IsValidComboSection(FName SectionName) const;

	// Spoke 콤보 몽타주를 지정한 시작 섹션에서 재생한다.
	bool PlayComboMontage(FName StartSectionName);

	// SpokeCombo 몽타주의 섹션 자동 진행을 C++ 분기 기준으로 고정한다.
	void ConfigureComboSectionFlow() const;

	// 지정한 섹션의 다음 섹션 링크를 갱신한다.
	void SetComboNextSection(FName FromSectionName, FName ToSectionName) const;

	// 현재 몽타주를 지정한 섹션으로 이동한다.
	bool JumpToComboSection(FName SectionName);

	// 콤보 윈도우 노티파이 이벤트를 받아 비루프 섹션 전환 시점을 제어한다.
	void BindComboWindowEvent();

	// 콤보 윈도우 노티파이 이벤트 대기를 종료한다.
	void EndComboWindowEventTask();

	// 콤보 윈도우 노티파이 이벤트가 들어왔을 때 현재 단계의 다음 분기를 처리한다.
	UFUNCTION()
	void HandleComboWindowGameplayEvent(FGameplayEventData Payload);

	// 현재 섹션의 콤보 윈도우 시점에서 다음 콤보 단계를 결정한다.
	void AdvanceCurrentComboStage();

	// 첫 1타 이후 Spoke Slam으로 분기할지 결정한다.
	bool ShouldBranchToSlam() const;

	// 반대 방향 follow-up을 시작한다.
	void BeginOppositeFollowUp();

	// Spoke Slam Start 섹션을 시작한다.
	void BeginSlamStart();

	// Spoke Slam Loop 섹션을 시작한다.
	void BeginSlamLoop();

	// Spoke Slam Loop를 주기적으로 갱신하고 End 전환 조건을 확인한다.
	void TickSlamLoop();

	// Spoke Slam End 섹션을 시작한다.
	void BeginSlamEnd(bool bWasCancelled);

	// Spoke Slam Loop의 최대 지속 시간을 계산한다.
	float ResolveSlamLoopMaxSeconds() const;

	// 현재 타깃과의 2D 거리를 계산한다.
	float CalculateDistanceToTarget() const;

	// Slam Loop에서 코드 기반 보조 이동을 사용할 때 타깃 방향으로 이동한다.
	bool MoveSlamTowardTarget(float DeltaTime);

	// Motion Warping Notify가 사용할 AttackTarget 워프 타깃을 갱신한다.
	bool RefreshAttackTargetMotionWarp(bool bUseTargetLocation) const;

	// Ability 진행 중 AttackTarget 워프 타깃을 주기적으로 갱신한다.

	// 현재 Spoke 1타의 코드 기반 피해 판정 시작 타이머를 건다.
	void BindMeleeHitWindowEvents();

	// 현재 Spoke 1타의 검 sweep 피해 판정을 시작한다.
	void EndMeleeHitWindowEventTasks();

	// 현재 Spoke 1타의 검 sweep 피해 판정을 갱신한다.
	UFUNCTION()
	void HandleMeleeHitWindowBeginGameplayEvent(FGameplayEventData Payload);

	// 현재 Spoke 1타의 검 sweep 피해 판정을 닫는다.
	UFUNCTION()
	void HandleMeleeHitWindowTickGameplayEvent(FGameplayEventData Payload);

	// Slam End 구간 피해 판정 시작 타이머를 건다.
	UFUNCTION()
	void HandleMeleeHitWindowEndGameplayEvent(FGameplayEventData Payload);

	// Slam End 구간 검 sweep 피해 판정을 시작한다.
	void BeginNotifiedDamageWindow();

	// Slam End 구간 검 sweep 피해 판정을 갱신한다.
	void TickNotifiedDamageWindow();

	// Slam End 구간 검 sweep 피해 판정을 닫는다.
	void EndNotifiedDamageWindow();

	// 현재 검 판정 기준점을 계산한다.
	bool GetCurrentBladeTracePoint(FVector& OutTracePoint) const;

	// 이전 검 위치와 현재 검 위치 사이를 sweep하여 피해를 적용한다.
	void ApplyDamageTrace(const FVector& TraceStart, const FVector& TraceEnd, float InDamageMultiplier, float InPoiseDamage);

	// Spoke 콤보 피해를 적용할 수 있는 대상인지 확인한다.
	bool ShouldDamageActor(AActor* CandidateActor) const;

	// Spoke 콤보에서 사용하는 모든 타이머를 정리한다.
	void ClearSpokeComboTimers();

	// Ability 종료 시 남아 있는 SpokeCombo 몽타주를 정리한다.
	void StopSpokeComboMontage() const;

	// 이동과 검 hitbox를 멈춘다.
	void StopSpokeComboMovement() const;

	// Ability를 정상 또는 취소 상태로 종료한다.
	void FinishSpokeCombo(bool bWasCancelled);

	UFUNCTION()
	void HandleComboMontageCompleted();

	UFUNCTION()
	void HandleComboMontageBlendOut();

	UFUNCTION()
	void HandleComboMontageInterrupted();

protected:
	// R/L 1타와 Slam Start/Loop/End 섹션을 모두 포함하는 단일 Spoke 콤보 몽타주다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Animation")
	TObjectPtr<UAnimMontage> SpokeComboMontage;

	// Spoke R 시작 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Animation")
	FName RightOpeningSectionName = TEXT("Spoke_R_Start");

	// Spoke L 시작 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Animation")
	FName LeftOpeningSectionName = TEXT("Spoke_L_Start");

	// Spoke R 후속타 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Animation")
	FName RightFollowUpSectionName = TEXT("Spoke_R_FollowUp");

	// Spoke L 후속타 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Animation")
	FName LeftFollowUpSectionName = TEXT("Spoke_L_FollowUp");

	// Spoke Slam 시작 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Animation")
	FName SlamStartSectionName = TEXT("SpokeSlam_Start");

	// Spoke Slam 추적 loop 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Animation")
	FName SlamLoopSectionName = TEXT("SpokeSlam_Loop");

	// Spoke Slam 마무리 공격 섹션 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Animation")
	FName SlamEndSectionName = TEXT("SpokeSlam_End");

	// Spoke 콤보 몽타주 재생 속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Animation", meta = (ClampMin = "0.01"))
	float MontagePlayRate = 1.0f;

	// Ability가 중간 종료될 때 SpokeCombo 몽타주를 정리하는 블렌드아웃 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Animation", meta = (ClampMin = "0.0"))
	float MontageStopBlendOutTime = 0.12f;

	// 첫 1타 방향 선택 기준이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Combo")
	EPRFaerinSpokeComboFirstDirectionPolicy FirstDirectionPolicy = EPRFaerinSpokeComboFirstDirectionPolicy::TargetSide;

	// 첫 1타 이후 분기 기준이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Combo")
	EPRFaerinSpokeComboBranchPolicy BranchPolicy = EPRFaerinSpokeComboBranchPolicy::RandomOppositeOrSlam;

	// RandomOppositeOrSlam일 때 Slam으로 이어질 확률이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Combo", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SlamBranchChance = 0.35f;

	// Ability 진행 중 AttackTarget 워프 타깃을 갱신하는 주기다.

	// 몽타주 notify가 없는 초기 검증 단계에서 코드 타이머 기반 Spoke 1타 피해 판정을 사용할지 결정한다.
	// 각 Spoke 1타 시작 후 피해 판정이 열리기까지의 지연 시간이다.
	// 각 Spoke 1타 피해 판정 유지 시간이다.
	// Slam End 구간 피해 판정을 사용할지 결정한다.
	// Slam End 진입 후 피해 판정이 열리기까지의 지연 시간이다.
	// Slam End 피해 판정 유지 시간이다.
	// 검 sweep 피해 판정 갱신 주기다.
	// WeaponVisualComponent가 없을 때 사용할 검 bone/socket 이름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Hit")
	FName FallbackBladeBoneName = TEXT("Bone_FN_Weapon_Sword_Blade");

	// 검 판정 기준점에 더할 로컬 오프셋이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Hit")
	FVector HitTraceOffset = FVector::ZeroVector;

	// 검 sweep 반경이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Hit", meta = (ClampMin = "0.0"))
	float HitTraceRadius = 85.0f;

	// 검 sweep 충돌 채널이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Hit")
	TEnumAsByte<ECollisionChannel> HitTraceChannel = ECC_Pawn;

	// 현재 ThreatTarget만 피해 대상으로 제한할지 결정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Hit")
	bool bOnlyDamageThreatTarget = true;

	// Spoke R/L 1타 피해 배율이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Hit", meta = (ClampMin = "0.0"))
	float AttackDamageMultiplier = 0.85f;

	// Spoke R/L 1타 강인도 피해다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Hit", meta = (ClampMin = "0.0"))
	float AttackPoiseDamage = 8.0f;

	// Spoke Slam End 피해 배율이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Hit", meta = (ClampMin = "0.0"))
	float SlamDamageMultiplier = 1.15f;

	// Spoke Slam End 강인도 피해다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Hit", meta = (ClampMin = "0.0"))
	float SlamPoiseDamage = 15.0f;

	// Slam Loop 거리 갱신 주기다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Slam", meta = (ClampMin = "0.005"))
	float SlamTrackingTickInterval = 0.016f;

	// 타깃과의 거리가 이 값 이하이고 최소 loop 시간이 지났으면 Slam End로 전환한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Slam", meta = (ClampMin = "0.0"))
	float SlamEndTriggerDistance = 520.0f;

	// Slam Start 직후 End로 바로 넘어가지 않도록 보장하는 최소 loop 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Slam", meta = (ClampMin = "0.0"))
	float SlamMinimumLoopSeconds = 0.25f;

	// Slam Loop 최대 지속 시간 하한이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Slam", meta = (ClampMin = "0.0"))
	float SlamLoopMaxSecondsMin = 0.65f;

	// Slam Loop 최대 지속 시간 상한이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Slam", meta = (ClampMin = "0.0"))
	float SlamLoopMaxSecondsMax = 1.15f;

	// RootMotion/MotionWarping만으로 부족할 때 코드 기반 보조 이동을 사용할지 결정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Slam")
	bool bUseCodeDrivenSlamTrackingMovement = false;

	// 코드 기반 Slam 보조 이동 속도다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Slam", meta = (ClampMin = "0.0"))
	float SlamTrackingMoveSpeed = 850.0f;

	// Slam 보조 이동 방향 계산에서 높이 차이를 제거할지 결정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Slam")
	bool bConstrainSlamMovementToGround = true;

	// Spoke 콤보 sweep 디버그를 그릴지 결정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|SpokeCombo|Debug")
	bool bDrawDebug = false;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<AActor> ActiveTarget;

	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinWeaponVisualComponent> WeaponVisualComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveMeleeHitWindowBeginEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveMeleeHitWindowTickEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveMeleeHitWindowEndEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveComboWindowEventTask;

	FTimerHandle SlamLoopTrackingTimerHandle;
	TSet<TWeakObjectPtr<AActor>> DamagedActors;
	FVector PreviousBladeTracePoint = FVector::ZeroVector;
	EPRFaerinSpokeComboStage ActiveStage = EPRFaerinSpokeComboStage::None;
	EPRFaerinSpokeComboDirection ActiveDirection = EPRFaerinSpokeComboDirection::None;
	EPRFaerinSpokeComboDamageWindow ActiveDamageWindow = EPRFaerinSpokeComboDamageWindow::None;
	float SlamLoopElapsedSeconds = 0.0f;
	float SlamLoopMaxSeconds = 0.0f;
	float LastSlamTrackingUpdateTime = 0.0f;
	bool bHasPreviousBladeTracePoint = false;
	bool bSpokeComboFinished = false;
	bool bPendingEndCancelled = false;
};
