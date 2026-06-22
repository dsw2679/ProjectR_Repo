// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (피격 반응 상태 연동 구현)
// Author: 배유찬 (데미지 계산 파이프라인 연동 구현)
// Author: 손승우 (피격/그로기/사망 관련 공용 상태 처리 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/Combat/PRCombatTypes.h"
#include "PRGameplayAbility_EnemyBase.generated.h"

class APREnemyBaseCharacter;
class UGameplayEffect;

// 적 Ability의 공통 베이스다.
// 서버 실행 정책과 공용 데미지 적용 함수를 한 곳에 모아 패턴 Ability들이 같은 흐름을 타게 한다.
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility_EnemyBase : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_EnemyBase();

protected:
	// Enemy 전용 컴포넌트/데이터에 접근해야 할 때 사용하는 Avatar 캐스팅 헬퍼다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|Combat")
	APREnemyBaseCharacter* GetEnemyAvatarCharacter() const;

	// EnemyStatRow AttackPower 기반 체력 피해와 공격별 고정 강인도 피해를 대상에게 적용한다.
	void ApplyAttackPowerDamage(AActor* TargetActor, float DamageMultiplier, float PoiseDamage = 0.0f, const FHitResult* HitResult = nullptr);
};
