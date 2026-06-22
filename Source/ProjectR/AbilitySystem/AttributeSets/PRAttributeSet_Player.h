// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (스태미나 소모/회복 및 Poise 경직, 성장 특성 포인트 속성 구현)
// Author: 배유찬 (피해 연동용 플레이어 방어 속성 구현)
// Author: 이건주 (장비 인벤토리 연동용 능력치 속성 구현)
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRAttributeSet_Player.generated.h"

class UPRWeaponDataAsset;
class UPRWeaponModDataAsset;

// 플레이어 전용 자원과 무기 슬롯 자원 정본을 관리하는 속성 세트
UCLASS()
class PROJECTR_API UPRAttributeSet_Player : public UAttributeSet
{
	GENERATED_BODY()

public:
	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ UAttributeSet Interface ~*/
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	
public:
	// 스태미너
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData Stamina;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, Stamina)

	// 최대 스태미너
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData MaxStamina;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, MaxStamina)

	// 초당 스태미너 회복량
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaRegenRate, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData StaminaRegenRate;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, StaminaRegenRate)

	// 회복 가능 체력
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_RecoverableHealth, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData RecoverableHealth;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, RecoverableHealth)

	// 이번 피해로 적립할 회복 가능 체력
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData IncomingRecoverableDamage;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, IncomingRecoverableDamage)

	// 회복 가능 체력에서 현재 체력으로 옮길 회복량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData IncomingRecoverableHeal;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, IncomingRecoverableHeal)

	// 누적 강인도 피해
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AccumulatedPoiseDamage, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData AccumulatedPoiseDamage;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, AccumulatedPoiseDamage)

	// 이번 피격으로 들어온 강인도 피해
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData IncomingPoiseDamage;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, IncomingPoiseDamage)

	// 누적 강인도 피해 최소값
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PoiseDamageMin, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData PoiseDamageMin;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, PoiseDamageMin)

	// 약한 경직 임계값
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PoiseWeakHitReactThreshold, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData PoiseWeakHitReactThreshold;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, PoiseWeakHitReactThreshold)

	// 강한 경직 임계값
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PoiseStrongHitReactThreshold, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData PoiseStrongHitReactThreshold;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, PoiseStrongHitReactThreshold)

	// 누적 강인도 피해 최대값
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PoiseDamageMax, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData PoiseDamageMax;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, PoiseDamageMax)

	// 치명타 확률
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalHitChance, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData CriticalHitChance;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, CriticalHitChance)

	// 치명타 피해 배율
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalDamageMultiplier, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData CriticalDamageMultiplier;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, CriticalDamageMultiplier)

	// 플레이어 공격력 보너스
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PlayerAttackPower, Category = "ProjectR|Attributes|Player")
	FGameplayAttributeData PlayerAttackPower;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Player, PlayerAttackPower)

protected:
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_RecoverableHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_AccumulatedPoiseDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PoiseDamageMin(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PoiseWeakHitReactThreshold(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PoiseStrongHitReactThreshold(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PoiseDamageMax(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CriticalHitChance(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CriticalDamageMultiplier(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PlayerAttackPower(const FGameplayAttributeData& OldValue);


};
