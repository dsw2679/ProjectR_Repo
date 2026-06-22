// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스전 Pattern 기본 구조 시퀀스 어빌리티 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Enemy/PRGameplayAbility_EnemyBase.h"
#include "PRGameplayAbility_BossPatternBase.generated.h"

class APRBossBaseCharacter;

// 보스 패턴 Ability의 공통 베이스다.
// 패턴 일괄 취소 태그와 보스 Avatar 접근 헬퍼만 제공하고, 실제 패턴 로직은 하위 Ability에 둔다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_BossPatternBase : public UPRGameplayAbility_EnemyBase
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_BossPatternBase();

protected:
	// 현재 Avatar를 보스 캐릭터로 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss")
	APRBossBaseCharacter* GetBossAvatarCharacter() const;

	// 보스 패턴 실행에 필요한 최소 상태가 유효한지 확인한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss")
	bool CanRunBossPattern() const;

	// 보스가 현재 위협 대상으로 삼는 패턴 타겟을 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss")
	AActor* GetBossPatternTarget() const;

	// 현재 패턴 공격 대상이 중간에 바뀌지 않도록 ThreatComponent에 고정한다.
	void BeginBossPatternAttackCommit();

	// 패턴 공격 대상 고정을 해제하고 보류된 타겟 전환을 처리한다.
	void EndBossPatternAttackCommit();
};
