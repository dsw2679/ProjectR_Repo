// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 이건주 (적 AI Penitent 몬스터 Staff Swing 공격 패턴 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyMeleeAttack.h"
#include "PRGameplayAbility_PenitentStaffSwing.generated.h"

class UAbilityTask_WaitGameplayEvent;

// Penitent 지팡이 휘두르기 현재 섹션
enum class EPRPenitentStaffSwingSection : uint8
{
	Swing1,
	Swing2
};

// Penitent 지팡이 휘두르기 Ability
UCLASS()
class PROJECTR_API UPRGameplayAbility_PenitentStaffSwing : public UPRGameplayAbility_EnemyMeleeAttack
{
	GENERATED_BODY()

public:
	// Ability 태그와 StaffSwing 기본값 초기화
	UPRGameplayAbility_PenitentStaffSwing();

	/*~ UGameplayAbility Interface ~*/
	// StaffSwing 1타 섹션 시작과 콤보 윈도우 바인딩
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 콤보 이벤트 태스크 정리 후 공통 종료 처리
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	/*~ UPRGameplayAbility_EnemyMeleeAttack Interface ~*/
	// 현재 StaffSwing 섹션 타격값 적용 후 서버 Sweep 판정 실행
	virtual void ExecuteMeleeHit() override;

	// 콤보 윈도우 거리/확률 기반 2타 섹션 진입 판단
	UFUNCTION()
	void HandleComboWindowGameplayEvent(FGameplayEventData Payload);

private:
	void BindComboWindowEvent();
	void JumpToSection(FName SectionName, EPRPenitentStaffSwingSection NewSection);
	float GetDistanceToCurrentTarget() const;
	const FPREnemyAttackHitConfig& GetCurrentHitConfig() const;

protected:
	// StaffSwing 1타 시작 섹션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|StaffSwing")
	FName StaffSwing1StartSection = TEXT("C1_Start");

	// StaffSwing 2타 진입 섹션
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|StaffSwing")
	FName StaffSwing2EnterSection = TEXT("C2_Start");

	// StaffSwing 2타 분기 최대 거리
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|StaffSwing", meta = (ClampMin = "0.0"))
	float StaffSwing2MaxRange = 250.0f;

	// StaffSwing 2타 분기 확률
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|StaffSwing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StaffSwing2Chance = 0.25f;

	// StaffSwing 1타 판정값
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|StaffSwing")
	FPREnemyAttackHitConfig StaffSwing1HitConfig;

	// StaffSwing 2타 판정값
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Penitent|StaffSwing")
	FPREnemyAttackHitConfig StaffSwing2HitConfig;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveComboWindowEventTask;

	EPRPenitentStaffSwingSection CurrentStaffSwingSection = EPRPenitentStaffSwingSection::Swing1;
};
