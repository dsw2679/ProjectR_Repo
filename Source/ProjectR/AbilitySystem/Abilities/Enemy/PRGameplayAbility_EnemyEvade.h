// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Evade 상태/패턴 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_EnemyEvade.generated.h"

class AActor;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UBlackboardComponent;

UENUM(BlueprintType)
enum class EPREnemyEvadeDirectionSelectionMode : uint8
{
	Random,
	Left,
	Right,
	AwayFromTarget,
};

// 일반 적이 좌/우 Root Motion 회피 몽타주를 재생하는 공용 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_EnemyEvade : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_EnemyEvade();

	/*~ UGameplayAbility Interface ~*/
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		OUT FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;

private:
	UFUNCTION()
	void HandleEvadeMontageCompleted();

	UFUNCTION()
	void HandleEvadeMontageInterrupted();

	// 현재 설정과 타겟 위치를 기준으로 재생할 회피 몽타주를 선택한다.
	UAnimMontage* SelectEvadeMontage() const;

	// 타겟이 있는 방향의 반대편 회피 몽타주를 선택한다.
	UAnimMontage* SelectMontageAwayFromTarget() const;

	// 좌우 몽타주 중 사용 가능한 몽타주를 무작위로 선택한다.
	UAnimMontage* SelectRandomMontage() const;

	// 블랙보드 current_target 기준으로 현재 공격 대상을 가져온다.
	AActor* ResolveCurrentTargetActor() const;

	// 회피 비용으로 사용할 attack_pressure Blackboard를 가져온다.
	UBlackboardComponent* ResolveBlackboardComponent(const FGameplayAbilityActorInfo* ActorInfo = nullptr) const;

	// 회피 시작 전 attack_pressure 비용을 지불할 수 있는지 확인한다.
	bool CanSpendAttackPressureCost(const FGameplayAbilityActorInfo* ActorInfo = nullptr) const;

	// 회피 시작 시 attack_pressure 비용을 차감한다.
	bool TryConsumeAttackPressureCost() const;

	// 회피 몽타주 종료 콜백이 여러 번 들어와도 Ability 종료를 한 번만 수행한다.
	void FinishEvade(bool bWasCancelled);

protected:
	// 왼쪽으로 이동하는 Root Motion 회피 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Evade|Animation")
	TObjectPtr<UAnimMontage> LeftEvadeMontage;

	// 오른쪽으로 이동하는 Root Motion 회피 몽타주다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Evade|Animation")
	TObjectPtr<UAnimMontage> RightEvadeMontage;

	// 회피 몽타주 재생 속도다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Evade|Animation", meta = (ClampMin = "0.0"))
	float MontagePlayRate = 1.0f;

	// 회피 방향 선택 방식이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Evade")
	EPREnemyEvadeDirectionSelectionMode DirectionSelectionMode = EPREnemyEvadeDirectionSelectionMode::Random;

	// 회피 시작 시 CharacterMovement 속도를 제거할지 여부다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Evade|Movement")
	bool bStopMovementOnActivate = true;

	// 회피 시작 시 AI MoveTo 경로를 중단할지 여부다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Evade|Movement")
	bool bStopAIPathOnActivate = true;

	// 타겟 방향 회피 선택에서 사용할 블랙보드 키 이름이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Evade|Target")
	FName CurrentTargetKey = TEXT("current_target");

	// 회피 비용으로 차감할 attack_pressure Blackboard 키 이름이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Evade|Pressure")
	FName AttackPressureKey = TEXT("attack_pressure");

	// 회피 시작 시 attack_pressure를 차감할지 여부다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Evade|Pressure")
	bool bConsumeAttackPressureOnActivate = true;

	// 회피 1회에 소모할 attack_pressure 값이다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Evade|Pressure", meta = (ClampMin = "0.0", EditCondition = "bConsumeAttackPressureOnActivate"))
	float AttackPressureCost = 2.0f;

	// Blackboard나 pressure 키가 없을 때 회피를 실패시킬지 여부다.
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Evade|Pressure", meta = (EditCondition = "bConsumeAttackPressureOnActivate"))
	bool bFailIfAttackPressureUnavailable = true;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	// 회피 Ability 종료 처리가 이미 요청되었는지 추적한다.
	bool bEvadeFinished = false;

	// GAS 쿨타임 판정에 사용할 회피 쿨타임 태그다.
	FGameplayTagContainer CooldownTagsContainer;
};
