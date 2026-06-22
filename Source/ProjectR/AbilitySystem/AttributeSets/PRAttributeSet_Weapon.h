// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (무기 탄약량, 약점 피해율 및 Mod 게이지 충전 속성 구현)
// Author: 이건주 (Mod 버프 연동용 무기 속성 구현)
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRAttributeSet_Weapon.generated.h"

class UPRWeaponDataAsset;
class UPRWeaponModDataAsset;

// 플레이어 무기 슬롯 자원 정보를 관리하는 속성 세트
// 탄약 자원은 실제 탄약 수치 단위로 저장한다
UCLASS()
class PROJECTR_API UPRAttributeSet_Weapon : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPRAttributeSet_Weapon();

	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ UAttributeSet Interface ~*/
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	/*~ UPRAttributeSet_Weapon Interface ~*/
	// 탄약 타입에 대응하는 탄창 현재값 어트리뷰트 핸들을 반환한다
	static FGameplayAttribute GetMagazineAmmoAttribute(EPRAmmoType AmmoType);

	// 탄약 타입에 대응하는 탄창 최대값 어트리뷰트 핸들을 반환한다
	static FGameplayAttribute GetMaxMagazineAmmoAttribute(EPRAmmoType AmmoType);

	// 탄약 타입에 대응하는 예비탄 현재값 어트리뷰트 핸들을 반환한다
	static FGameplayAttribute GetReserveAmmoAttribute(EPRAmmoType AmmoType);

	// 탄약 타입에 대응하는 예비탄 최대값 어트리뷰트 핸들을 반환한다
	static FGameplayAttribute GetMaxReserveAmmoAttribute(EPRAmmoType AmmoType);

	// 탄약 타입에 대응하는 탄창 현재값을 반환한다
	float GetMagazineAmmoByType(EPRAmmoType AmmoType) const;

	// 탄약 타입에 대응하는 탄창 최대값을 반환한다
	float GetMaxMagazineAmmoByType(EPRAmmoType AmmoType) const;

	// 탄약 타입에 대응하는 예비탄 현재값을 반환한다
	float GetReserveAmmoByType(EPRAmmoType AmmoType) const;

	// 탄약 타입에 대응하는 예비탄 최대값을 반환한다
	float GetMaxReserveAmmoByType(EPRAmmoType AmmoType) const;

protected:
	UFUNCTION()
	void OnRep_PrimaryMagazineAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryMaxMagazineAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryReserveAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryMaxReserveAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryModGauge(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryMaxModGauge(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryModStack(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrimaryMaxModStack(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryMagazineAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryMaxMagazineAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryReserveAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryMaxReserveAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryModGauge(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryMaxModGauge(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryModStack(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_SecondaryMaxModStack(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_BaseDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_DamageMultiplier(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ArmorPenetration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_WeakpointMultiplier(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_GroggyDamageMultiplier(const FGameplayAttributeData& OldValue);

public:
	// 주무기 슬롯 탄창 잔탄
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryMagazineAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryMagazineAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryMagazineAmmo)

	// 주무기 슬롯 탄창 최대값
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryMaxMagazineAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryMaxMagazineAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryMaxMagazineAmmo)

	// 주무기 슬롯 예비 탄약
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryReserveAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryReserveAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryReserveAmmo)

	// 주무기 슬롯 예비 탄약 최대값
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryMaxReserveAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryMaxReserveAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryMaxReserveAmmo)

	// 주무기 슬롯 Mod 게이지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryModGauge, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryModGauge;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryModGauge)

	// 주무기 슬롯 Mod 최대 게이지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryMaxModGauge, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryMaxModGauge;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryMaxModGauge)

	// 주무기 슬롯 Mod 스택
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryModStack, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryModStack;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryModStack)

	// 주무기 슬롯 Mod 최대 스택
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrimaryMaxModStack, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData PrimaryMaxModStack;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, PrimaryMaxModStack)

	// 보조무기 슬롯 탄창 잔탄
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryMagazineAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryMagazineAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryMagazineAmmo)

	// 보조무기 슬롯 탄창 최대값
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryMaxMagazineAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryMaxMagazineAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryMaxMagazineAmmo)

	// 보조무기 슬롯 예비 탄약
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryReserveAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryReserveAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryReserveAmmo)

	// 보조무기 슬롯 예비 탄약 최대값
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryMaxReserveAmmo, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryMaxReserveAmmo;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryMaxReserveAmmo)

	// 보조무기 슬롯 Mod 게이지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryModGauge, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryModGauge;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryModGauge)

	// 보조무기 슬롯 Mod 최대 게이지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryMaxModGauge, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryMaxModGauge;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryMaxModGauge)

	// 보조무기 슬롯 Mod 스택
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryModStack, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryModStack;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryModStack)

	// 보조무기 슬롯 Mod 최대 스택
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryMaxModStack, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData SecondaryMaxModStack;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, SecondaryMaxModStack)
	
	// 무기 기본 데미지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseDamage, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData BaseDamage;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, BaseDamage)

	// 데미지 배율
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DamageMultiplier, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData DamageMultiplier;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, DamageMultiplier)

	// 장갑 관통
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ArmorPenetration, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData ArmorPenetration;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, ArmorPenetration)

	// 약점 피해 배율
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WeakpointMultiplier, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData WeakpointMultiplier;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, WeakpointMultiplier)

	// 그로기 데미지 배율
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_GroggyDamageMultiplier, Category = "ProjectR|Attributes|Weapon")
	FGameplayAttributeData GroggyDamageMultiplier;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Weapon, GroggyDamageMultiplier)
};
