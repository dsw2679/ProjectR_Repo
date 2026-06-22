// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (피해 보정 연동 구현)
// Author: 손승우 (아머드 솔저 해머 콤보 시퀀스 및 모션 워핑 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AI/Data/PRSoldierArmoredCombatDataAsset.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyMeleeAttack.h"
#include "PRGameplayAbility_SoldierArmoredHammerFamily.generated.h"

class UAbilityTask_WaitGameplayEvent;

// 원본 Armored 해머 흐름 기반 섹션 점프형 Ability
UCLASS()
class PROJECTR_API UPRGameplayAbility_SoldierArmoredHammerFamily : public UPRGameplayAbility_EnemyMeleeAttack
{
	GENERATED_BODY()

public:
	// Ability 태그 고정, 전투 수치는 CombatDataAsset 참조
	UPRGameplayAbility_SoldierArmoredHammerFamily();

	// CombatDataAsset 적용 후 해머 패밀리 몽타주 Combo1 시작
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 콤보 윈도우 이벤트 태스크 정리 후 공통 종료 처리
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 현재 해머 섹션 타격값 적용 후 서버 Sweep 판정 실행
	virtual void ExecuteMeleeHit() override;

	// 콤보 윈도우 거리/확률 기반 다음 몽타주 섹션 결정
	UFUNCTION()
	void HandleComboWindowGameplayEvent(FGameplayEventData Payload);

private:
	void BindComboWindowEvent();
	void JumpToSection(FName SectionName, EPRSoldierArmoredHammerSection NewSection);
	float GetDistanceToCurrentTarget() const;
	const FPREnemyAttackHitConfig& GetCurrentHitConfig() const;
	void ApplyCurrentHitConfig();

protected:
	// 섹션명, 원본 거리, 확률, 타격값 관리용 DataAsset
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Data")
	TObjectPtr<UPRSoldierArmoredCombatDataAsset> CombatDataAsset;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveComboWindowEventTask;

	EPRSoldierArmoredHammerSection CurrentHammerSection = EPRSoldierArmoredHammerSection::Combo1;
};
