// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 피격 관련 반응 범위 데이터 연동)
// Author: 손승우 (아머드 솔저 해머 공격 및 콤보 관련 전투 파라미터 정의)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "ProjectR/Combat/PREnemyAttackTypes.h"
#include "PRSoldierArmoredCombatDataAsset.generated.h"

// Soldier_Armored 해머 콤보 현재 재생 구간
UENUM(BlueprintType)
enum class EPRSoldierArmoredHammerSection : uint8
{
	Combo1		UMETA(DisplayName = "Combo1"),
	Combo2		UMETA(DisplayName = "Combo2"),
	Finisher1	UMETA(DisplayName = "Finisher1"),
	Finisher2	UMETA(DisplayName = "Finisher2")
};

// Soldier_Armored 해머 패밀리 몽타주 섹션 이름 묶음
USTRUCT(BlueprintType)
struct PROJECTR_API FPRSoldierArmoredHammerSectionNames
{
	GENERATED_BODY()

	// HammerCombo_01 시작 섹션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	FName Combo1StartSection = NAME_None;

	// HammerCombo_02 진입 섹션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	FName Combo2EnterSection = NAME_None;

	// Finisher_01 진입 섹션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	FName Finisher1EnterSection = NAME_None;

	// Finisher_02 진입 섹션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Animation")
	FName Finisher2EnterSection = NAME_None;
};

// Soldier_Armored 원본 거리/분기 규칙
USTRUCT(BlueprintType)
struct PROJECTR_API FPRSoldierArmoredHammerFlowConfig
{
	GENERATED_BODY()

	// HammerCombo_02 분기 최대 유효 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Original Reference", meta = (ClampMin = "0.0"))
	float Combo2MaxRange = 0.0f;

	// Finisher_01/02 분기 최대 유효 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Original Reference", meta = (ClampMin = "0.0"))
	float FinisherMaxRange = 0.0f;

	// HammerCombo_01 도중 Finisher_01 진행 확률
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Original Reference", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Finisher1Chance = 0.0f;
};

// Soldier_Armored 전용 전투 데이터 자산
UCLASS(BlueprintType)
class PROJECTR_API UPRSoldierArmoredCombatDataAsset : public UPREnemyCombatDataAsset
{
	GENERATED_BODY()

public:
	// 해머 패밀리 몽타주 섹션 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPRSoldierArmoredHammerSectionNames HammerSections;

	// 원본 거리와 확률 기반 분기 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPRSoldierArmoredHammerFlowConfig HammerFlow;

	// HammerCombo_01 타격값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPREnemyAttackHitConfig Combo1HitConfig;

	// HammerCombo_02 타격값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPREnemyAttackHitConfig Combo2HitConfig;

	// Finisher_01 타격값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPREnemyAttackHitConfig Finisher1HitConfig;

	// Finisher_02 타격값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Hammer")
	FPREnemyAttackHitConfig Finisher2HitConfig;

	// SprintHammer 타격값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Sprint")
	FPREnemyAttackHitConfig SprintHammerHitConfig;
};
