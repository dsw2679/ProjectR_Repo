// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "PRFaerinCloneCharacter.generated.h"

class APRFaerinCharacter;
class UMaterialInstanceDynamic;
class UMotionWarpingComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UPRFaerinWeaponVisualComponent;
class UTexture;
struct FGameplayEventData;

// Faerin 분신의 생명 주기 상태다.
UENUM(BlueprintType)
enum class EPRFaerinCloneState : uint8
{
	Inactive,
	Spawning,
	ChasingTarget,
	SpokeCombo,
	ReturningToOwner,
	Merged,
	Killed
};

// Faerin 분신 SpokeCombo의 현재 진행 단계다.
UENUM(BlueprintType)
enum class EPRFaerinCloneSpokeStage : uint8
{
	None,
	Opening,
	FollowUp,
	SlamStart,
	SlamLoop,
	SlamEnd
};

// Faerin 분신 SpokeCombo의 R/L 방향이다.
UENUM(BlueprintType)
enum class EPRFaerinCloneSpokeDirection : uint8
{
	None,
	Right,
	Left
};

// Faerin 분신 SpokeCombo 히트 윈도우 피해 타입이다.
UENUM(BlueprintType)
enum class EPRFaerinCloneSpokeDamageWindow : uint8
{
	None,
	Spoke,
	SlamEnd
};

// Faerin 분신 SpokeCombo 첫 1타 방향 선택 방식이다.
UENUM(BlueprintType)
enum class EPRFaerinCloneFirstDirectionPolicy : uint8
{
	TargetSide,
	Random,
	ForceRight,
	ForceLeft
};

// Faerin 분신 SpokeCombo 첫 1타 이후 분기 방식이다.
UENUM(BlueprintType)
enum class EPRFaerinCloneBranchPolicy : uint8
{
	OppositeOnly,
	SlamOnly,
	RandomOppositeOrSlam
};

// Faerin 분신의 런타임 전투/연출 설정이다.
USTRUCT(BlueprintType)
struct FPRFaerinCloneRuntimeConfig
{
	GENERATED_BODY()

	// 분신 최대 체력이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "1.0"))
	float CloneMaxHealth = 350.0f;

	// 분신 공격력이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0"))
	float CloneAttackPower = 18.0f;

	// 분신 방어력이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0"))
	float CloneArmor = 0.0f;

	// 분신 최대 그로기 게이지다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0"))
	float CloneMaxGroggyGauge = 100.0f;

	// 분신 이동 속도 배율이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0"))
	float CloneMovementSpeedMultiplier = 1.0f;

	// 소환 연출이 끝나기 전까지 대기할 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0.0"))
	float SpawnIntroSeconds = 0.6f;

	// 분신 스폰 위치에 재생할 Niagara다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|VFX")
	TObjectPtr<UNiagaraSystem> SpawnLocationNiagaraSystem;

	// 분신 몸에 붙여 재생할 역디졸브 Niagara다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|VFX")
	TObjectPtr<UNiagaraSystem> SpawnDissolveNiagaraSystem;

	// 분신 소환 디졸브에 사용할 텍스처다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|VFX")
	TObjectPtr<UTexture> SpawnDissolveTexture;

	// 분신 소환 디졸브 텍스처 UV 스케일이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|VFX")
	FVector2D SpawnDissolveTextureUV = FVector2D(1.0f, 1.0f);

	// 분신 소환 디졸브 총 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|VFX", meta = (ClampMin = "0.0"))
	float SpawnDissolveDuration = 0.6f;

	// 분신 소환 디졸브 시작 값이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|VFX")
	float SpawnDissolveStartValue = 1.0f;

	// 분신 소환 디졸브 종료 값이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|VFX")
	float SpawnDissolveEndValue = 0.0f;

	// 분신 머티리얼 디졸브 스칼라 파라미터 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|VFX")
	FName SpawnDissolveScalarParameterName = TEXT("DissolveAmount");

	// 분신 Niagara 디졸브 파라미터 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|VFX")
	FName SpawnNiagaraDissolveParameterName = TEXT("User.DissolveAmount");

	// 분신 소환 디졸브 보간 주기다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn|VFX", meta = (ClampMin = "0.001"))
	float SpawnDissolveTickInterval = 0.016f;

	// 분신이 파괴될 때 현재 위치에 재생할 Niagara다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death|VFX")
	TObjectPtr<UNiagaraSystem> DestroyedNiagaraSystem;

	// 위치형 Niagara 생명 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX", meta = (ClampMin = "0.0"))
	float WorldNiagaraLifeSeconds = 1.5f;

	// 위치형 Niagara 스케일이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	FVector WorldNiagaraScale = FVector(1.0f);

	// 대상 추적 이동 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase", meta = (ClampMin = "0.0"))
	float ChaseMoveSpeed = 420.0f;

	// 이 거리 안에 들어오면 SpokeCombo를 시작한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase", meta = (ClampMin = "0.0"))
	float SpokeStartRange = 600.0f;

	// 대상 추적 제한 시간이다. 초과 시 회복 없이 본체로 복귀한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase", meta = (ClampMin = "0.0"))
	float ChaseMaxSeconds = 6.0f;

	// R/L 1타와 Slam Start/Loop/End 섹션을 포함하는 SpokeCombo 몽타주다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Animation")
	TObjectPtr<UAnimMontage> SpokeComboMontage;

	// Spoke R 시작 섹션 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Animation")
	FName RightOpeningSectionName = TEXT("Spoke_R_Start");

	// Spoke L 시작 섹션 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Animation")
	FName LeftOpeningSectionName = TEXT("Spoke_L_Start");

	// Spoke R 후속타 섹션 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Animation")
	FName RightFollowUpSectionName = TEXT("Spoke_R_FollowUp");

	// Spoke L 후속타 섹션 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Animation")
	FName LeftFollowUpSectionName = TEXT("Spoke_L_FollowUp");

	// Spoke Slam 시작 섹션 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Animation")
	FName SlamStartSectionName = TEXT("SpokeSlam_Start");

	// Spoke Slam 추적 loop 섹션 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Animation")
	FName SlamLoopSectionName = TEXT("SpokeSlam_Loop");

	// Spoke Slam 마무리 공격 섹션 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Animation")
	FName SlamEndSectionName = TEXT("SpokeSlam_End");

	// SpokeCombo 몽타주 재생 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Animation", meta = (ClampMin = "0.01"))
	float MontagePlayRate = 1.0f;

	// 몽타주 정리 블렌드아웃 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Animation", meta = (ClampMin = "0.0"))
	float MontageStopBlendOutTime = 0.12f;

	// 첫 1타 방향 선택 기준이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Combo")
	EPRFaerinCloneFirstDirectionPolicy FirstDirectionPolicy = EPRFaerinCloneFirstDirectionPolicy::TargetSide;

	// 첫 1타 이후 분기 기준이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Combo")
	EPRFaerinCloneBranchPolicy BranchPolicy = EPRFaerinCloneBranchPolicy::RandomOppositeOrSlam;

	// RandomOppositeOrSlam일 때 Slam으로 이어질 확률이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Combo", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SlamBranchChance = 0.35f;

	// SpokeCombo가 노티파이 누락 등으로 끝나지 않을 때 정리할 제한 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Combo", meta = (ClampMin = "0.1"))
	float ComboFailsafeSeconds = 8.0f;

	// WeaponVisualComponent가 없을 때 사용할 검 bone/socket 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Hit")
	FName FallbackBladeBoneName = TEXT("Bone_FN_Weapon_Sword_Blade");

	// 검 판정 기준점에 더할 로컬 오프셋이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Hit")
	FVector HitTraceOffset = FVector::ZeroVector;

	// 검 sweep 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Hit", meta = (ClampMin = "0.0"))
	float HitTraceRadius = 85.0f;

	// 검 sweep 충돌 채널이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Hit")
	TEnumAsByte<ECollisionChannel> HitTraceChannel = ECC_Pawn;

	// 지정된 분신 타겟만 피해 대상으로 제한할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Hit")
	bool bOnlyDamageAssignedTarget = true;

	// Spoke R/L 1타 피해 배율이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Hit", meta = (ClampMin = "0.0"))
	float AttackDamageMultiplier = 0.85f;

	// Spoke R/L 1타 강인도 피해다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Hit", meta = (ClampMin = "0.0"))
	float AttackPoiseDamage = 8.0f;

	// Spoke Slam End 피해 배율이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Hit", meta = (ClampMin = "0.0"))
	float SlamDamageMultiplier = 1.15f;

	// Spoke Slam End 강인도 피해다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Hit", meta = (ClampMin = "0.0"))
	float SlamPoiseDamage = 15.0f;

	// 타깃과의 거리가 이 값 이하이고 최소 loop 시간이 지났으면 Slam End로 전환한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Slam", meta = (ClampMin = "0.0"))
	float SlamEndTriggerDistance = 520.0f;

	// Slam Start 직후 End로 바로 넘어가지 않도록 보장하는 최소 loop 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Slam", meta = (ClampMin = "0.0"))
	float SlamMinimumLoopSeconds = 0.25f;

	// Slam Loop 최대 지속 시간 하한이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Slam", meta = (ClampMin = "0.0"))
	float SlamLoopMaxSecondsMin = 0.65f;

	// Slam Loop 최대 지속 시간 상한이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Slam", meta = (ClampMin = "0.0"))
	float SlamLoopMaxSecondsMax = 1.15f;

	// RootMotion/MotionWarping만으로 부족할 때 코드 기반 보조 이동을 사용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Slam")
	bool bUseCodeDrivenSlamTrackingMovement = false;

	// 코드 기반 Slam 보조 이동 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Slam", meta = (ClampMin = "0.0"))
	float SlamTrackingMoveSpeed = 850.0f;

	// Slam 보조 이동 방향 계산에서 높이 차이를 제거할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpokeCombo|Slam")
	bool bConstrainSlamMovementToGround = true;

	// 공격 완료 뒤 본체 복귀 시작 전 대기 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Return", meta = (ClampMin = "0.0"))
	float ReturnDelayAfterSpokeCombo = 0.2f;

	// 본체 복귀 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Return", meta = (ClampMin = "0.0"))
	float ReturnSpeed = 1200.0f;

	// 본체 복귀 성공 판정 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Return", meta = (ClampMin = "0.0"))
	float ReturnAcceptanceRadius = 120.0f;

	// 본체 복귀 제한 시간이다. 초과 시 회복 없이 제거한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Return", meta = (ClampMin = "0.0"))
	float ReturnMaxSeconds = 3.5f;

	// 본체 복귀 기준 소켓 이름이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Return")
	FName MergeSocketName = TEXT("Bone_FN_PelvisSocket");

	// 본체 복귀 기준 위치 오프셋이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Return")
	FVector MergeLocationOffset = FVector::ZeroVector;

	// 복귀 성공 시 페어린 회복량이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Return|Heal", meta = (ClampMin = "0.0"))
	float MergeHealAmount = 250.0f;

	// 복귀 성공 회복의 최대 체력 비율 상한이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Return|Heal", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MergeHealMaxHealthRatio = 1.0f;

	// 복귀 성공 시 페어린 몸에 재생할 회복 Niagara다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Return|Heal")
	TObjectPtr<UNiagaraSystem> MergeHealNiagaraSystem;

	// 회복 Niagara 부착 소켓이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Return|Heal")
	FName MergeHealNiagaraSocketName = TEXT("Bone_FN_PelvisSocket");

	// 회복 Niagara 위치 오프셋이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Return|Heal")
	FVector MergeHealNiagaraLocationOffset = FVector::ZeroVector;

	// 회복 Niagara 회전 오프셋이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Return|Heal")
	FRotator MergeHealNiagaraRotationOffset = FRotator::ZeroRotator;

	// 회복 Niagara 스케일이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Return|Heal")
	FVector MergeHealNiagaraScale = FVector(1.0f);

	// 회복 Niagara 생명 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Return|Heal", meta = (ClampMin = "0.0"))
	float MergeHealNiagaraLifeSeconds = 1.5f;

	// SpokeCombo sweep 디버그를 그릴지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawDebug = false;
};

// Faerin 분신 전용 캐릭터다.
UCLASS(Blueprintable)
class PROJECTR_API APRFaerinCloneCharacter : public APREnemyBaseCharacter
{
	GENERATED_BODY()

public:
	APRFaerinCloneCharacter();

	/*~ AActor Interface ~*/
	// 분신 상태 머신을 서버에서 진행한다.
	virtual void Tick(float DeltaTime) override;

	// 분신 복제 상태를 설정한다.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ IPRCombatInterface ~*/
	// 분신 피격 후 체력이 0 이하가 되었는지 확인한다.
	virtual void OnPostDamageApplied(const FPRDamageAppliedContext& Context) override;

	/*~ APRFaerinCloneCharacter Interface ~*/
	// 소환 GA가 확정한 본체, 대상, 런타임 설정으로 분신을 초기화한다.
	void InitializeFaerinClone(
		APRFaerinCharacter* InOwnerFaerin,
		AActor* InAssignedTarget,
		const FPRFaerinCloneRuntimeConfig& InRuntimeConfig);

protected:
	/*~ AActor Interface ~*/
	// 남아 있는 노티파이 바인딩과 VFX 타이머를 정리한다.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/*~ APRCharacterBase Interface ~*/
	// Dead 태그가 외부에서 들어온 경우 분신 사망 흐름으로 흡수한다.
	virtual void HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool bTagExists) override;

	// 분신 소환 대기 상태를 갱신한다.
	void TickSpawning(float DeltaTime);

	// 분신 대상 추적 상태를 갱신한다.
	void TickChasingTarget(float DeltaTime);

	// 분신 SpokeCombo 상태를 갱신한다.
	void TickSpokeCombo(float DeltaTime);

	// 분신 본체 복귀 상태를 갱신한다.
	void TickReturningToOwner(float DeltaTime);

	// 지정 대상 추적 상태를 시작한다.
	void BeginChasingTarget();

	// SpokeCombo 몽타주를 시작한다.
	void BeginSpokeCombo();

	// SpokeCombo를 종료하고 본체 복귀로 전환한다.
	void CompleteSpokeCombo(bool bCompletedNormally);

	// 본체 복귀 상태를 시작한다.
	void BeginReturningToOwner(bool bShouldHealOnMerge);

	// 본체 복귀 성공 처리를 수행한다.
	void CompleteMerge();

	// 분신 사망 처리를 수행한다.
	void HandleCloneKilled();

	// 회복 없이 분신을 제거한다.
	void DestroyCloneWithoutHeal();

	// 분신 런타임 Attribute를 ASC에 주입한다.
	void InitializeCloneAbilitySystem();

	// 현재 지정 타겟이 공격 가능한지 확인한다.
	bool IsAssignedTargetAttackable() const;

	// 대상 방향으로 캐릭터 이동 입력을 추가한다.
	void MoveTowardAssignedTarget(float DeltaTime);

	// 대상 또는 본체를 향하도록 회전을 갱신한다.
	void FaceLocation(const FVector& TargetLocation, float DeltaTime);

	// 현재 타겟 기준 첫 Spoke 방향을 결정한다.
	EPRFaerinCloneSpokeDirection ResolveFirstDirection() const;

	// 현재 방향의 반대 방향을 반환한다.
	EPRFaerinCloneSpokeDirection GetOppositeDirection(EPRFaerinCloneSpokeDirection Direction) const;

	// 방향에 대응하는 시작 섹션 이름을 반환한다.
	FName GetOpeningSectionName(EPRFaerinCloneSpokeDirection Direction) const;

	// 방향에 대응하는 follow-up 섹션 이름을 반환한다.
	FName GetFollowUpSectionName(EPRFaerinCloneSpokeDirection Direction) const;

	// 지정한 섹션이 현재 몽타주에 존재하는지 확인한다.
	bool IsValidComboSection(FName SectionName) const;

	// 현재 몽타주 섹션 자동 진행을 C++ 분기 기준으로 고정한다.
	void ConfigureComboSectionFlow() const;

	// 지정한 섹션의 다음 섹션 링크를 갱신한다.
	void SetComboNextSection(FName FromSectionName, FName ToSectionName) const;

	// 현재 몽타주를 지정한 섹션으로 이동하고 클라이언트에 동기화한다.
	bool JumpToComboSection(FName SectionName);

	// 콤보 윈도우 노티파이 시점에서 다음 단계로 진행한다.
	void AdvanceCurrentComboStage();

	// 첫 1타 이후 Slam으로 분기할지 결정한다.
	bool ShouldBranchToSlam() const;

	// 반대 방향 follow-up 섹션을 시작한다.
	void BeginOppositeFollowUp();

	// Spoke Slam Start 섹션을 시작한다.
	void BeginSlamStart();

	// Spoke Slam Loop 섹션을 시작한다.
	void BeginSlamLoop();

	// Spoke Slam End 섹션을 시작한다.
	void BeginSlamEnd(bool bWasCancelled);

	// Spoke Slam Loop 최대 지속 시간을 계산한다.
	float ResolveSlamLoopMaxSeconds() const;

	// 타깃과의 2D 거리를 계산한다.
	float CalculateDistanceToTarget() const;

	// 코드 기반 Slam 보조 이동을 실행한다.
	bool MoveSlamTowardTarget(float DeltaTime);

	// Motion Warping Notify가 사용할 AttackTarget 워프 타깃을 갱신한다.
	bool RefreshAttackTargetMotionWarp(bool bUseTargetLocation) const;

	// 몽타주 노티파이 이벤트 수신을 시작한다.
	void BindSpokeComboEvents();

	// 몽타주 노티파이 이벤트 수신을 정리한다.
	void UnbindSpokeComboEvents();

	// 콤보 윈도우 노티파이 이벤트를 처리한다.
	void HandleComboWindowGameplayEvent(const FGameplayEventData* Payload);

	// 히트 윈도우 시작 노티파이 이벤트를 처리한다.
	void HandleMeleeHitWindowBeginGameplayEvent(const FGameplayEventData* Payload);

	// 히트 윈도우 tick 노티파이 이벤트를 처리한다.
	void HandleMeleeHitWindowTickGameplayEvent(const FGameplayEventData* Payload);

	// 히트 윈도우 종료 노티파이 이벤트를 처리한다.
	void HandleMeleeHitWindowEndGameplayEvent(const FGameplayEventData* Payload);

	// 현재 노티파이 기반 피해 판정을 시작한다.
	void BeginNotifiedDamageWindow();

	// 현재 노티파이 기반 피해 판정을 갱신한다.
	void TickNotifiedDamageWindow();

	// 현재 노티파이 기반 피해 판정을 닫는다.
	void EndNotifiedDamageWindow();

	// 현재 검 판정 기준점을 계산한다.
	bool GetCurrentBladeTracePoint(FVector& OutTracePoint) const;

	// 이전 검 위치와 현재 검 위치 사이를 sweep하여 피해를 적용한다.
	void ApplyDamageTrace(const FVector& TraceStart, const FVector& TraceEnd, float InDamageMultiplier, float InPoiseDamage);

	// 분신 SpokeCombo 피해를 적용할 수 있는 대상인지 확인한다.
	bool ShouldDamageActor(AActor* CandidateActor) const;

	// 적 공격력 기반 피해 GE를 대상에게 적용한다.
	void ApplyCloneAttackPowerDamage(AActor* TargetActor, float DamageMultiplier, float PoiseDamage, const FHitResult* HitResult);

	// SpokeCombo 몽타주를 정리한다.
	void StopSpokeComboMontage() const;

	// 분신 이동을 즉시 정리한다.
	void StopCloneMovement();

	// 월드 위치 Niagara를 모든 클라이언트에서 재생한다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnCloneWorldNiagara(
		UNiagaraSystem* NiagaraSystem,
		FVector_NetQuantize Location,
		FRotator Rotation,
		FVector Scale,
		float LifeSeconds);

	// 분신 SpokeCombo 몽타주 시작을 모든 클라이언트에 동기화한다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayCloneSpokeMontage(UAnimMontage* Montage, float PlayRate, FName StartSectionName);

	// 분신 SpokeCombo 섹션 점프를 모든 클라이언트에 동기화한다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_JumpCloneSpokeMontageSection(FName SectionName);

	// 분신 소환 디졸브를 모든 클라이언트에서 재생한다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayCloneSpawnDissolveVisual(
		UNiagaraSystem* DissolveNiagaraSystem,
		float DissolveDuration,
		float DissolveStartValue,
		float DissolveEndValue,
		FName DissolveScalarParameterName,
		FName NiagaraDissolveParameterName,
		UTexture* DissolveTexture,
		FVector2D DissolveTextureUV,
		float DissolveTickInterval);

	// 로컬 인스턴스에서 분신 소환 디졸브를 시작한다.
	void StartCloneSpawnDissolveVisual(
		UNiagaraSystem* DissolveNiagaraSystem,
		float DissolveDuration,
		float DissolveStartValue,
		float DissolveEndValue,
		FName DissolveScalarParameterName,
		FName NiagaraDissolveParameterName,
		UTexture* DissolveTexture,
		const FVector2D& DissolveTextureUV,
		float DissolveTickInterval);

	// 로컬 인스턴스에서 분신 소환 디졸브를 갱신한다.
	void TickCloneSpawnDissolveVisual();

	// 로컬 인스턴스에서 분신 소환 디졸브를 완료한다.
	void CompleteCloneSpawnDissolveVisual();

	// 분신 메시와 Niagara에 디졸브 값을 적용한다.
	void ApplyCloneSpawnDissolveVisualValue(float DissolveValue);

	// 분신 소환 디졸브 연출을 즉시 정리한다.
	void ClearCloneSpawnDissolveVisual();

protected:
	// 공격 루트모션 방향 보정을 위한 Motion Warping 컴포넌트다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Animation")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

private:
	UPROPERTY(Replicated)
	EPRFaerinCloneState CloneState = EPRFaerinCloneState::Inactive;

	UPROPERTY(Transient)
	TObjectPtr<APRFaerinCharacter> OwnerFaerin;

	UPROPERTY(Transient)
	TObjectPtr<AActor> AssignedTarget;

	UPROPERTY(Transient)
	TObjectPtr<UPRFaerinWeaponVisualComponent> WeaponVisualComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> CloneDissolveDynamicMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveCloneDissolveNiagara;

	UPROPERTY(Transient)
	FPRFaerinCloneRuntimeConfig RuntimeConfig;

	FTimerHandle CloneDissolveTickTimerHandle;
	FDelegateHandle ComboWindowEventHandle;
	FDelegateHandle MeleeHitWindowBeginEventHandle;
	FDelegateHandle MeleeHitWindowTickEventHandle;
	FDelegateHandle MeleeHitWindowEndEventHandle;
	TSet<TWeakObjectPtr<AActor>> DamagedActors;
	FVector PreviousBladeTracePoint = FVector::ZeroVector;
	EPRFaerinCloneSpokeStage ActiveSpokeStage = EPRFaerinCloneSpokeStage::None;
	EPRFaerinCloneSpokeDirection ActiveSpokeDirection = EPRFaerinCloneSpokeDirection::None;
	EPRFaerinCloneSpokeDamageWindow ActiveDamageWindow = EPRFaerinCloneSpokeDamageWindow::None;
	float StateElapsedSeconds = 0.0f;
	float SpokeComboElapsedSeconds = 0.0f;
	float SlamLoopElapsedSeconds = 0.0f;
	float SlamLoopMaxSeconds = 0.0f;
	float CloneDissolveElapsedTime = 0.0f;
	float CloneDissolveDuration = 0.0f;
	float CloneDissolveStartValue = 1.0f;
	float CloneDissolveEndValue = 0.0f;
	float CloneDissolveTickInterval = 0.016f;
	FName CloneDissolveScalarParameterName = TEXT("DissolveAmount");
	FName CloneNiagaraDissolveParameterName = TEXT("User.DissolveAmount");
	bool bHasPreviousBladeTracePoint = false;
	bool bPendingSlamEndCancelled = false;
	bool bCanApplyHealOnMerge = false;
	bool bCloneResolved = false;
};
