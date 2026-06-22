// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Mod 충전 게이지 요구량 및 액티브 어빌리티 데이터 정의)
// Author: 이건주 (드론 및 배리어 Mod 전용 비주얼/스탯 데이터 정의)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "PRWeaponModDataAsset.generated.h"

// 무기 Mod 1종의 초기 게이지, 스택, 어빌리티 연결 정보를 담는다
UCLASS(BlueprintType)
class PROJECTR_API UPRWeaponModDataAsset : public UPRItemDataAsset
{
	GENERATED_BODY()

public:
	UPRWeaponModDataAsset();

	void GiveToAbilitySystem(UAbilitySystemComponent* TargetASC,
		FPRAbilitySetHandles& OutHandles,
		UObject* InSourceObject,
		const FGameplayTagContainer* AdditionalDynamicTags = nullptr);
	
public:
	// Mod 분류와 호환성 검증에 사용할 태그 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Mod")
	FGameplayTagContainer ModTags;

	// 슬롯 초기화 시 사용할 최대 게이지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Mod|Stat", meta = (ClampMin = "0.0"))
	float MaxModGauge = 1000.0f;
	
	// 슬롯 초기화 시 사용할 최대 스택
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Mod|Stat", meta = (ClampMin = "0.0"))
	int32 MaxModStack = 0.0f;

	// 활성 슬롯 장착 시 부여할 Mod 어빌리티 목록. 순서대로 GiveAbility
	UPROPERTY(EditAnywhere, Category = "Item|Mod")
	TArray<FPRAbilityEntry> EquippedAbilities;

	// 활성 슬롯 장착 시 부여 직후 자동 적용할 Mod Startup GE 목록
	UPROPERTY(EditAnywhere, Category = "Item|Mod")
	TArray<FPREffectEntry> EquippedEffects;
};
