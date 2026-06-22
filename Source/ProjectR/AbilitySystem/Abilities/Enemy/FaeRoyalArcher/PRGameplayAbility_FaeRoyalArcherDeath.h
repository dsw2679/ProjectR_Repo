// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (적 AI Fae 로열 아처 사망 상태/패턴 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyDeath.h"
#include "PRGameplayAbility_FaeRoyalArcherDeath.generated.h"

// Fae Royal Archer 전용 죽음 Ability다.
// 공중 전투/Perch 표현 상태를 정리한 뒤 원작형 죽음 몽타주 후보를 선택한다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_FaeRoyalArcherDeath : public UPRGameplayAbility_EnemyDeath
{
	GENERATED_BODY()

public:
	// Royal Archer 죽음 연출 기본값을 초기화한다.
	UPRGameplayAbility_FaeRoyalArcherDeath();

protected:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 사용할 죽음 몽타주 후보를 결정한다.
	UAnimMontage* ResolveDeathMontageCandidate() const;

	// 죽음 몽타주 재생 전에 Royal Archer 전용 전투 표현 상태를 정리한다.
	void PrepareRoyalArcherDeathPresentation() const;

protected:
	// DeathMontage보다 우선 사용할 원작형 죽음 몽타주 후보 목록이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|RoyalArcher")
	TArray<TObjectPtr<UAnimMontage>> DeathMontageCandidates;

	// true면 후보 목록에서 무작위로 선택하고, false면 첫 번째 유효 후보를 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Animation|RoyalArcher")
	bool bRandomizeDeathMontageCandidate = true;
};
