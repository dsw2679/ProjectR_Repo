// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (특성 성장 및 피격 경직 임계치 테이블 구조체 정의)
// Author: 배유찬 (기본 레벨업 능력치 밸런스 테이블 구조체 정의)
// Author: 이건주 (장비 능력치 가중치 테이블 구조체 정의)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ProjectR/Combat/PRCombatTypes.h"
#include "PRStatRows.generated.h"

// DT_EnemyStats / DT_BossStats 공용 Row. 프로퍼티 이름이 Registry의 리플렉션 키
USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyStatRow : public FTableRowBase
{
	GENERATED_BODY()

	// 최대 체력. 초기화 경로에서 MaxHealth로 주입
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxHealth = 100.0f;

	// 이동 속도 배율 초기값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MovementSpeedMultiplier = 1.0f;

	// 최대 그로기 게이지
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxGroggyGauge = 100.0f;

	// 방어력
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Armor = 0.0f;

	// 적 기본 공격력
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float AttackPower = 10.0f;

	// 본 이름별 데미지 부위 매핑. 미정의 본은 Default Region(None)으로 처리
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FName, FPRDamageRegionInfo> RegionMap;
};

// DT_PlayerStats Row. 프로퍼티 이름이 Registry의 리플렉션 키
USTRUCT(BlueprintType)
struct PROJECTR_API FPRPlayerStatRow : public FTableRowBase
{
	GENERATED_BODY()

	// 최대 체력 초기값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxHealth = 100.0f;

	// 이동 속도 배율 초기값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MovementSpeedMultiplier = 1.0f;

	// 최대 스태미너 초기값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxStamina = 100.0f;

	// 초당 스태미너 회복량 초기값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float StaminaRegenRate = 25.0f;

	// 방어력
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Armor = 0.0f;

	// 회복 가능 체력 초기값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float RecoverableHealth = 0.0f;

	// 누적 강인도 피해 최소값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float PoiseDamageMin = 0.0f;

	// 약한 경직 임계값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float PoiseWeakHitReactThreshold = 33.0f;

	// 강한 경직 임계값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float PoiseStrongHitReactThreshold = 66.0f;

	// 누적 강인도 피해 최대값
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float PoiseDamageMax = 100.0f;

	// 치명타 확률 (0.0 ~ 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CriticalHitChance = 0.05f;

	// 치명타 피해 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CriticalDamageMultiplier = 1.5f;

	// 플레이어 공격력 보너스
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float PlayerAttackPower = 0.0f;
};
