// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 피격 연동용 전투 타입 정의)
// Author: 배유찬 (약점 및 피해 결과 관련 데이터 타입 정의)
// Author: 손승우 (적 그로기 상태 관련 전투 타입 정의)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "PRCombatTypes.generated.h"

class AActor;
class UObject;

// HitResult가 어떤 특수 부위에 맞았는지 나타낸다.
// ComponentTag의 Armor.* / Weakpoint.* 접두사를 해석한 결과다.
UENUM(BlueprintType)
enum class EPRDamageRegionType : uint8
{
	None		UMETA(DisplayName = "None"),
	Armor		UMETA(DisplayName = "Armor"),
	Weakpoint	UMETA(DisplayName = "Weakpoint")
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRDamageRegionInfo
{
	GENERATED_BODY()

	// 일반 부위, 갑옷, 약점 중 어디에 맞았는지 나타낸다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Combat")
	EPRDamageRegionType RegionType = EPRDamageRegionType::None;

	// 약점 대미지 배율 (대미지 = 무기의 WeakPointDamageMultiplier * RegionDamageMultiplier * BaseDamage, 방어력 계산 제외한 경우) 
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "ProjectR|Combat")
	float RegionDamageMultiplier = 1.0f;
	
	// Optional Context Tag (예: Armor.Torso, Weakpoint.Head)
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "ProjectR|Combat")
	FName RegionTag = NAME_None;

	bool IsArmor() const
	{
		return RegionType == EPRDamageRegionType::Armor;
	}

	bool IsWeakpoint() const
	{
		return RegionType == EPRDamageRegionType::Weakpoint;
	}
};


// 프렌들리 파이어 감쇠 등 전투 산식 상수 모음
namespace PRCombatConstants
{
	// 플레이어 진영 우호 공격 시 데미지 곱연산 배율 (0.0 = 우호 완전 면역)
	constexpr float FriendlyFireMultiplier_PvP = 0.0f;

	// 적 진영 우호 공격 시 데미지 곱연산 배율
	constexpr float FriendlyFireMultiplier_EvE = 0.0f;
	
	// 무기 데미지 -> 그로기 데미지 변환 배율
	constexpr float GroggyDamageCoeff = 0.3f;

	// 방어력 경감 공식의 스케일링 상수. 경감률 = Armor / (Armor + ArmorScaling)
	constexpr float ArmorScaling = 100.0f;

	// 장갑 부위 적중 시 추가 방어력 보너스 (대상 Armor에 더해진다)
	constexpr float ArmorRegionBonus = 50.0f;
}

// ExecCalc 산식의 입력 묶음
USTRUCT()
struct PROJECTR_API FPRDamageInputs
{
	GENERATED_BODY()

	float BaseDamage = 0.f;
	float GroggyDamage = 0.f;
	float TargetArmor = 0.f;
	float ArmorPenetration = 0.f;
	float WeakpointMultiplier = 1.f;
	float CriticalHitChance = 0.f;
	float CriticalDamageMultiplier = 1.f;
	bool bIsFromFriendly = false;
	bool bIsFromPlayer = false;
};

// ExecCalc 산식 결과 묶음
USTRUCT()
struct PROJECTR_API FPRDamageOutputs
{
	GENERATED_BODY()

	float FinalDamage = 0.f;
	float GroggyDamage = 0.f;
	bool bIsCritical = false;
	bool bIsFromFriendly = false;
	FPRDamageRegionInfo Region;
};