// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (방어구 파츠 스켈레탈 메시 및 능력치 데이터 정의)
// Author: 손승우 (보스전용 특수 장착 에셋 데이터 연동)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Types/PREquipmentTypes.h"
#include "PREquipmentDataAsset.generated.h"

class USkeletalMesh;
class UAbilitySystemComponent;

// 장비 아이템의 정적 데이터를 정의
UCLASS(BlueprintType)
class PROJECTR_API UPREquipmentDataAsset : public UPRItemDataAsset
{
	GENERATED_BODY()

public:
	UPREquipmentDataAsset();

	// 장착 슬롯 반환
	EPREquipmentSlotType GetSlotType() const { return SlotType; }

	// 장비 외형 메시 반환
	TSoftObjectPtr<USkeletalMesh> GetEquipmentMesh() const { return EquipmentMesh; }

	// 플레이어 얼굴 숨김 여부 반환
	bool ShouldHidePlayerFace() const { return SlotType == EPREquipmentSlotType::Head && bHidePlayerFace; }

	// 최대 체력 보너스 반환
	float GetMaxHealthBonus() const { return MaxHealthBonus; }

	// 방어력 보너스 반환
	float GetArmorBonus() const { return ArmorBonus; }

	// 공격력 보너스 반환
	float GetAttackPowerBonus() const { return AttackPowerBonus; }

	// 최대 스태미너 보너스 반환
	float GetMaxStaminaBonus() const { return MaxStaminaBonus; }

	// 추가 장착 GameplayEffect 목록 반환
	const TArray<FPREffectEntry>& GetEquippedEffects() const { return EquippedEffects; }

	// 추가 장착 어빌리티와 효과를 ASC에 부여
	void GiveAdditionalEffectsToAbilitySystem(UAbilitySystemComponent* TargetASC, FPRAbilitySetHandles& OutHandles, UObject* InSourceObject) const;

protected:
	// 이 장비가 장착되는 슬롯 종류
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment")
	EPREquipmentSlotType SlotType = EPREquipmentSlotType::None;

	// 장착 시 캐릭터에 부착될 메시 (추후 ChildMesh로 사용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment")
	TSoftObjectPtr<USkeletalMesh> EquipmentMesh;

	// 머리 장비 장착 시 플레이어 얼굴 메시 숨김 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment", meta = (EditCondition = "SlotType == EPREquipmentSlotType::Head", EditConditionHides))
	bool bHidePlayerFace = false;

	// 장착 시 공용 장비 GE에 전달할 최대 체력 보너스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment|Stat")
	float MaxHealthBonus = 0.0f;

	// 장착 시 공용 장비 GE에 전달할 방어력 보너스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment|Stat")
	float ArmorBonus = 0.0f;

	// 장착 시 공용 장비 GE에 전달할 플레이어 공격력 보너스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment|Stat")
	float AttackPowerBonus = 0.0f;

	// 장착 시 공용 장비 GE에 전달할 최대 스태미너 보너스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment|Stat")
	float MaxStaminaBonus = 0.0f;

	// 공용 장비 GE로 표현하지 않는 추가 어빌리티 목록
	UPROPERTY(EditDefaultsOnly, Category = "Item|Equipment|Effect")
	TArray<FPRAbilityEntry> EquippedAbilities;

	// 공용 장비 GE로 표현하지 않는 추가 GameplayEffect 목록
	UPROPERTY(EditDefaultsOnly, Category = "Item|Equipment|Effect")
	TArray<FPREffectEntry> EquippedEffects;
};
