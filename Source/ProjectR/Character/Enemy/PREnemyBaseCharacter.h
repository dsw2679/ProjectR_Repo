// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (적 체력 UI 및 아이템 drop 연동 구현)
// Author: 배유찬 (어그로 마커(핑) 연동, 대미지 팝업 및 ASC 태그 이벤트 바인딩 구현)
// Author: 손승우 (적 AI 상태 제어, 사망 디졸브 연출 및 아머드 솔저 적 캐릭터 구현)
// Author: 이건주 (Penitent 및 드론/배리어 패턴 지원 적 캐릭터 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/AbilitySystem/Data/PRStatRows.h"
#include "ProjectR/Character/PRCharacterBase.h"
#include "ProjectR/Character/Enemy/PREnemyInterface.h"
#include "ProjectR/System/PRTickOptimizable.h"
#include "ProjectR/UI/WorldMarker/PRPingMarkerTargetInterface.h"
#include "PREnemyBaseCharacter.generated.h"

class UBehaviorTree;
class UAnimMontage;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UNiagaraSystem;
class UTexture;
class UPRAbilitySystemComponent;
class UPRAttributeSet_Common;
class UPRAttributeSet_Enemy;
class UPREnemyWorldHealthBarComponent;
class UPRPatternDataAsset;
class UPRPerceptionConfig;
class UPRCombatMoveDataAsset;
class UPREnemyCombatEventRelayComponent;
class UPREnemyThreatComponent;
class UBlackboardComponent;
struct FPREnemyMovePresentationConfig;

// 모든 일반 몬스터가 공유하는 베이스 캐릭터다.
// ASC/AttributeSet/Threat/BT/Perception을 소유하고, 서버 Possess 시 AbilitySet과 AI를 초기화한다.
UCLASS(Abstract)
class PROJECTR_API APREnemyBaseCharacter : public APRCharacterBase, public IPREnemyInterface, public IPRPingMarkerTargetInterface, public IPRTickOptimizable
{
	GENERATED_BODY()

public:
	APREnemyBaseCharacter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual UPRAbilitySystemComponent* GetPRAbilitySystemComponent() const override;
	virtual EPRCharacterRole GetCharacterRole() const { return EPRCharacterRole::Enemy; }

	/*~ IPRCombatInterface ~*/
	virtual EPRTeam GetTeam() const override { return EPRTeam::Enemy; }
	virtual FPRDamageRegionInfo GetDamageRegionInfo(FName BoneName) const override;
	virtual void OnPostDamageApplied(const FPRDamageAppliedContext& Context) override;
	
	/*~ IPRPingMarkerTargetInterface ~*/
	virtual FPRWorldMarkerVisualData GetPingMarkerVisualData_Implementation() const override;
	virtual FVector GetPingMarkerWorldLocation_Implementation() const override;
	virtual bool ShouldPingMarkerVisible_Implementation() const override;

	/*~ IPRTickOptimizable ~*/
	// 월드 Tick 최적화 평가에 사용할 설정값 반환
	virtual FPRTickOptimizationConfig GetTickConfig() const override;

	// 월드 Tick 최적화 거리 평가에 사용할 기준 위치 반환
	virtual FVector GetTickLocation() const override;

	// 사망처럼 다른 시스템이 상태를 소유한 경우 거리 평가 제외 여부 반환
	virtual bool CanOptimizeTick() const override;

	// 현재 AI Tick 비용 활성 상태 반환
	virtual bool IsTickActive() const override;

	// 월드 Tick 최적화 시스템이 결정한 AI Tick 비용 상태 적용
	virtual void SetTickActive(bool bActive) override;

public:
	virtual UPRAbilitySystemComponent* GetEnemyAbilitySystemComponent() const override;
	virtual UPREnemyThreatComponent* GetEnemyThreatComponent() const override;
	virtual UPRPatternDataAsset* GetPatternDataAsset() const override;
	virtual UPRCombatMoveDataAsset* GetCombatDataAsset() const override;
	virtual UPRPerceptionConfig* GetPerceptionConfig() const override;
	virtual UBehaviorTree* GetBehaviorTreeAsset() const override;
	virtual FVector GetHomeLocation() const override;
	virtual void InitializeEnemyBlackboard(UBlackboardComponent* BlackboardComponent) const override;

	// EQS 기반 전투 이동 표현 문맥을 갱신한다.
	virtual void ApplyCombatMovePresentationContext(const FPREnemyMovePresentationConfig& PresentationConfig) override;

	// EQS 기반 전투 이동 표현 문맥을 초기화한다.
	virtual void ClearCombatMovePresentationContext() override;

	// 전투 이동 중 타겟 포커스 유지 문맥 여부를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|Animation")
	bool ShouldMaintainCombatMoveFocus() const { return bMaintainCombatMoveFocus; }

	UFUNCTION(BlueprintPure, Category = "ProjectR|Animation")
	bool ShouldUseCombatMovePose() const { return bUseCombatMovePose; }

	// 전투 이동 중 AimOffset 사용 문맥 여부를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|Animation")
	bool ShouldUseCombatAimOffset() const { return bUseCombatAimOffset; }

	UFUNCTION(BlueprintPure, Category = "ProjectR|Animation")
	bool ShouldUseCombatStrafeState() const { return bUseCombatStrafeState; }

	// 특수 배치 포즈를 ABP 상태머신에서 우선 사용할지 반환한다.
	virtual bool ShouldUsePerchIdlePose() const { return false; }

	const UPRAttributeSet_Common* GetCommonSet() const { return CommonSet; }
	const UPRAttributeSet_Enemy* GetEnemySet() const { return EnemySet; }
	UPREnemyWorldHealthBarComponent* GetEnemyWorldHealthBarComponent() const { return EnemyWorldHealthBarComponent; }

	// 드롭 테이블 조회에 사용할 몬스터 식별자를 반환한다
	FName GetMonsterId() const { return CharacterID; }

	// 일반 그로기 상태 진입을 허용하는 몬스터인지 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Groggy")
	bool CanEnterGroggyState() const { return bCanEnterGroggyState; }

	// 서버 사망 Ability에서 모든 클라이언트에 사망 Dissolve 시각 연출을 요청한다.
	void RequestDeathDissolveVisual(
		UAnimMontage* InDeathMontage,
		float InMontagePlayRate,
		bool bInDissolveDelayAfterMontageEnd,
		float InDissolveDelay,
		float InDissolveDuration,
		float InDissolveStartValue,
		float InDissolveEndValue,
		FName InDissolveScalarParameterName,
		UNiagaraSystem* InDissolveNiagaraSystem,
		FName InNiagaraDissolveParameterName,
		UTexture* InDissolveTexture,
		FVector2D InDissolveTextureUV,
		float InDissolveTickInterval);

protected:
	/*~ APRCharacterBase Interface ~*/
	virtual void HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool TagExists) override;

	// 서버에서 ASC ActorInfo, 초기 스탯, AbilitySet을 준비한다.
	void InitializeEnemyAbilitySystem();

	// 현재 플레이어 수 기준 최대 체력과 현재 체력 갱신
	void ApplyHealthScale();

	// GameState 플레이어 수 변경 이벤트 구독
	void BindPlayerCountChanged();

	// GameState 플레이어 수 변경 이벤트 구독 해제
	void UnbindPlayerCountChanged();

	// GameState 플레이어 수 변경 이벤트 수신 처리
	void HandlePlayerCountChanged();

	// AIController가 Blackboard에 쓰기 전에 복귀 기준 위치를 확정한다.
	void InitializeHomeLocation();

	UFUNCTION()
	void HandleDeath(AActor* InstigatorActor);

	virtual void HandleDeadTagChanged(bool bEntered);
	void HandleGroggyTagChanged(bool bEntered);

	// 전투 이동 프레젠테이션 적용 전 기존 이동 설정을 캐시한다.
	void CacheMovementPresentationDefaults();

	// 전투 이동 프레젠테이션 설정을 CharacterMovement에 적용한다.
	void ApplyMovementPresentationConfig(const FPREnemyMovePresentationConfig& PresentationConfig);

	// 캐시해 둔 기존 이동 설정을 복구한다.
	void RestoreMovementPresentationDefaults();

	// 월드 HP 바 컴포넌트를 현재 ASC에 연결한다.
	void InitializeEnemyWorldHealthBar();

	// 월드 Tick 최적화 활성 상태에 따른 AIController와 관련 컴포넌트 제어
	void ApplyTickOptimizationState(bool bActive);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_RequestDeathDissolveVisual(
		UAnimMontage* InDeathMontage,
		float InMontagePlayRate,
		bool bInDissolveDelayAfterMontageEnd,
		float InDissolveDelay,
		float InDissolveDuration,
		float InDissolveStartValue,
		float InDissolveEndValue,
		FName InDissolveScalarParameterName,
		UNiagaraSystem* InDissolveNiagaraSystem,
		FName InNiagaraDissolveParameterName,
		UTexture* InDissolveTexture,
		FVector2D InDissolveTextureUV,
		float InDissolveTickInterval);

	// 클라이언트 메시를 사망 몽타주 마지막 포즈에 고정하고 Dissolve 시작을 예약한다.
	void BeginDeathDissolveVisual();

	// GAS 몽타주 복제가 늦거나 누락된 클라이언트에서 사망 몽타주 재생을 보정한다.
	void PlayDeathDissolveMontageIfNeeded();

	// 메시 머티리얼과 Niagara를 준비한 뒤 Dissolve 보간을 시작한다.
	void StartDeathDissolveVisual();

	// Dissolve 보간 값을 주기적으로 갱신한다.
	void TickDeathDissolveVisual();

	// Dissolve 연출 타이머와 Niagara를 정리한다.
	void CompleteDeathDissolveVisual();

	// 메시 머티리얼과 Niagara에 Dissolve 진행 값을 반영한다.
	void ApplyDeathDissolveVisualValue(float DissolveValue);

	// 사망 몽타주 이후 Idle로 돌아가지 않도록 현재 포즈를 고정한다.
	void FreezeDeathDissolvePose();
	
	void GiveModGauge(const FPRDamageAppliedContext& Context) const;

protected:
	// ====== Components ======
	// 적은 PlayerState가 아니라 캐릭터 자신이 ASC를 소유
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Ability")
	TObjectPtr<UPRAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Common> CommonSet;

	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Enemy> EnemySet;

	UPROPERTY(VisibleAnywhere, Category = "ProjectR|AI")
	TObjectPtr<UPREnemyThreatComponent> ThreatComponent;

	// 기존 BP 서브컴포넌트 참조 보호용 컴포넌트다. 상태 이벤트는 AttributeSet/BossBase가 직접 발행한다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|AI")
	TObjectPtr<UPREnemyCombatEventRelayComponent> CombatEventRelayComponent;

	// 일반 몬스터 머리 위 HP 바 표시를 담당한다.
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|UI")
	TObjectPtr<UPREnemyWorldHealthBarComponent> EnemyWorldHealthBarComponent;

	// ====== Configs ======
	// 월드 HP 바 사용 여부다. 보스나 특수 적은 비활성화할 수 있다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|UI")
	bool bUseWorldHealthBar = true;
	
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|UI")
	FVector PingMarkerOffset = FVector(0.f,0.f,25.f);
	
	// 이 몬스터가 Possess될 때 실행할 BT
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	// 패턴 선택 규칙 데이터다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	TObjectPtr<UPRPatternDataAsset> PatternDataAsset;

	// 적/보스 전투 이동과 표현 데이터를 담은 자산이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	TObjectPtr<UPRCombatMoveDataAsset> CombatDataAsset;

	// 시야/청각 감지 설정 데이터다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI")
	TObjectPtr<UPRPerceptionConfig> PerceptionConfig;

	// 월드 Tick 최적화 시스템 등록 여부. 보스나 특수 적의 비활성화 용도
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|TickOptimization")
	bool bUseTickOptimization = true;

	// 리스폰 시스템 자동 등록 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Respawn")
	bool bIsRespawnable = true;

	// 일반 그로기 태그와 Groggy Ability 진입을 허용할지 결정한다. 그로기가 없는 보스는 false로 둔다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|AI|Groggy")
	bool bCanEnterGroggyState = true;

	// 월드 Tick 최적화 거리 평가와 렌더 컬링 설정값
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|TickOptimization", meta = (ShowOnlyInnerProperties))
	FPRTickOptimizationConfig TickOptimizationConfig;

	// ====== States ======
	// AI 복귀 기준 위치다. Possess/BeginPlay 중 먼저 도달한 시점의 현재 위치로 저장한다.
	UPROPERTY(VisibleInstanceOnly, Category = "ProjectR|AI")
	FVector HomeLocation = FVector::ZeroVector;

	// HomeLocation을 0,0,0 기본값으로 다시 덮어쓰지 않기 위한 초기화 여부다.
	bool bHasInitializedHomeLocation = false;

	// DataTable에서 읽은 스케일 적용 전 최대 체력
	float BaseMaxHealth = 0.0f;

	// 플레이어 수 변경 이벤트 구독 핸들
	FDelegateHandle PlayerCountChangedDelegateHandle;

	// 서버에서 부여한 Ability/GE를 수동 초기화할 때 추적하기 위한 핸들 묶음이다.
	UPROPERTY()
	FPRAbilitySetHandles GrantedAbilitySetHandles;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "ProjectR|Animation")
	bool bMaintainCombatMoveFocus = false;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "ProjectR|Animation")
	bool bUseCombatMovePose = false;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "ProjectR|Animation")
	bool bUseCombatAimOffset = false;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "ProjectR|Animation")
	bool bUseCombatStrafeState = false;

	bool bHasCachedMovementPresentation = false;
	float CachedMaxWalkSpeed = 0.0f;
	FRotator CachedRotationRate = FRotator::ZeroRotator;
	bool bCachedOrientRotationToMovement = true;
	bool bCachedUseControllerDesiredRotation = false;
	bool bCachedUseControllerRotationYaw = false;
	// 부위 매핑 조회용으로 보유한 StatRow 사본. InitializeEnemyAbilitySystem에서 채운다.
	FPREnemyStatRow CachedStatRow;
	bool bHasCachedStatRow = false;

	// 월드 Tick 최적화 시스템이 마지막으로 적용한 AI 비용 활성 상태
	bool bTickActive = true;

	FTimerHandle DeathDissolveStartTimerHandle;
	FTimerHandle DeathDissolveTickTimerHandle;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DeathDissolveDynamicMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveDeathDissolveNiagara;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> PendingDeathDissolveMontage;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> PendingDeathDissolveNiagaraSystem;

	UPROPERTY(Transient)
	TObjectPtr<UTexture> PendingDeathDissolveTexture;

	FName PendingDeathDissolveScalarParameterName = TEXT("DissolveAmount");
	FName PendingDeathDissolveNiagaraParameterName = TEXT("User.DissolveAmount");
	FVector2D PendingDeathDissolveTextureUV = FVector2D(1.0f, 1.0f);
	float PendingDeathDissolveMontagePlayRate = 1.0f;
	float PendingDeathDissolveDelay = 0.0f;
	bool bPendingDeathDissolveDelayAfterMontageEnd = true;
	float PendingDeathDissolveDuration = 1.0f;
	float PendingDeathDissolveStartValue = 0.0f;
	float PendingDeathDissolveEndValue = 1.0f;
	float PendingDeathDissolveTickInterval = 0.016f;
	float DeathDissolveElapsedTime = 0.0f;
	bool bDeathDissolveVisualStarted = false;

	bool bGameplayTagEventsBound = false;
};
