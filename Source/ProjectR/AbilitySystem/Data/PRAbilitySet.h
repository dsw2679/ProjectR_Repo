// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (어빌리티 세트 로드 및 입력 매핑 구조 구현)
// Author: 이건주 (배리어 모드 및 무기 전용 어빌리티 매핑 구조 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayAbilitySpec.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"
#include "PRAbilitySet.generated.h"

class UPRGameplayAbility;
class UGameplayEffect;

// 부여 결과 핸들 묶음. Clear 시 대칭 입력
USTRUCT(BlueprintType)
struct PROJECTR_API FPRAbilitySetHandles
{
	GENERATED_BODY()

	// GiveAbility 반환 핸들 누적
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilityHandles;

	// ApplyGameplayEffectSpecToSelf 반환 핸들 누적
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> EffectHandles;

	// 두 배열 비움
	void Reset()
	{
		AbilityHandles.Reset();
		EffectHandles.Reset();
	}
};

// AbilitySet 내 어빌리티 항목
USTRUCT(BlueprintType)
struct PROJECTR_API FPRAbilityEntry
{
	GENERATED_BODY()

	// 부여할 어빌리티 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UPRGameplayAbility> AbilityClass;

	// 어빌리티 레벨. AbilitySpec.Level로 전달
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Level = 1;

	// Spec.DynamicSpecSourceTags에 주입될 태그 (InputTag 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer DynamicTags;

	// AbilityClass 유효성 검사
	bool IsValid() const;
	
	// 어빌리티 부여
	void GiveToAbilitySystem(UAbilitySystemComponent* TargetASC,
		FPRAbilitySetHandles& OutHandles,
		UObject* InSourceObject = nullptr,
		const FGameplayTagContainer* AdditionalDynamicTags = nullptr) const;
};

// AbilitySet 내 Startup GE 항목
USTRUCT(BlueprintType)
struct PROJECTR_API FPREffectEntry
{
	GENERATED_BODY()

	// 부여 시 자동 적용할 GE 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> EffectClass;

	// GE 레벨. Level Curve 기반 값 스케일
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Level = 1.0f;

	// SpecHandle에 부여할 동적 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer DynamicTags;

	// EffectClass 유효성 검사
	bool IsValid() const;
	
	// Effect부여
	void GiveToAbilitySystem(UAbilitySystemComponent* TargetASC, FPRAbilitySetHandles& OutHandles, const UObject* InSourceObject = nullptr) const;
};

// 어빌리티 + Startup GE 일괄 부여 단위
UCLASS(BlueprintType)
class PROJECTR_API UPRAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPRAbilitySet();
	
public:
	// 부여할 어빌리티 목록. 순서대로 GiveAbility
	UPROPERTY(EditAnywhere, Category = "AbilitySet")
	TArray<FPRAbilityEntry> Abilities;

	// 부여 직후 자동 적용할 Startup GE 목록
	UPROPERTY(EditAnywhere, Category = "AbilitySet")
	TArray<FPREffectEntry> Effects;
};


// 2026.04.27, 이건주, 런타임 UPRAbilitySet 생성용 함수 추가
// 어빌리티와 효과 목록으로 런타임 AbilitySet을 생성한다
PROJECTR_API UPRAbilitySet* CreateRuntimeAbilitySet(
	UObject* Outer,
	const TArray<FPRAbilityEntry>& InAbilities,
	const TArray<FPREffectEntry>& InEffects);
