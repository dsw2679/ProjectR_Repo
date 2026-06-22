// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (무기 탄약량, 약점 피해율 및 Mod 게이지 충전 속성 구현)
// Author: 이건주 (Mod 버프 연동용 무기 속성 구현)
#include "PRAttributeSet_Weapon.h"
#include "GameplayEffectExtension.h"
#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float ModResourceTolerance = KINDA_SMALL_NUMBER;

	struct FPRModResourceResult
	{
		// 정리된 Mod 게이지
		float ModGauge = 0.0f;

		// 정리된 Mod 스택
		float ModStack = 0.0f;
	};

	// 주무기 슬롯 자원 속성 판별
	bool IsPrimarySlotAttribute(const FGameplayAttribute& Attribute)
	{
		return Attribute == UPRAttributeSet_Weapon::GetPrimaryMagazineAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryMaxMagazineAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryReserveAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryMaxReserveAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryModGaugeAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryMaxModGaugeAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryModStackAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetPrimaryMaxModStackAttribute();
	}

	// 보조무기 슬롯 자원 속성 판별
	bool IsSecondarySlotAttribute(const FGameplayAttribute& Attribute)
	{
		return Attribute == UPRAttributeSet_Weapon::GetSecondaryMagazineAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryMaxMagazineAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryReserveAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryMaxReserveAmmoAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryModGaugeAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryMaxModGaugeAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryModStackAttribute()
			|| Attribute == UPRAttributeSet_Weapon::GetSecondaryMaxModStackAttribute();
	}

	// 음수 및 비정상 수치 보정
	float SanitizeModResourceValue(float Value)
	{
		return FMath::IsFinite(Value) ? FMath::Max(Value, 0.0f) : 0.0f;
	}

	// 양수 판정
	bool IsPositiveModResourceValue(float Value)
	{
		return Value > ModResourceTolerance;
	}

	// 스택 사용 판정
	bool IsStackResourceEnabled(float MaxStack)
	{
		return MaxStack >= 1.0f - ModResourceTolerance;
	}

	// 최대 스택 도달 판정
	bool IsModStackAtMax(float CurrentStack, float MaxStack)
	{
		return IsStackResourceEnabled(MaxStack) && CurrentStack >= MaxStack - ModResourceTolerance;
	}

	// 최대 게이지 도달 판정
	bool IsModGaugeFull(float CurrentGauge, float MaxGauge)
	{
		return CurrentGauge >= MaxGauge - ModResourceTolerance;
	}

	// Mod 게이지 초과분 정리
	FPRModResourceResult ResolveModGaugeOverflow(float CurrentGauge, float MaxGauge, float CurrentStack, float MaxStack, bool bAllowStackCharge)
	{
		const float SafeGauge = SanitizeModResourceValue(CurrentGauge);
		const float SafeMaxGauge = SanitizeModResourceValue(MaxGauge);
		const float SafeStack = SanitizeModResourceValue(CurrentStack);
		const float SafeMaxStack = SanitizeModResourceValue(MaxStack);

		FPRModResourceResult Result;
		Result.ModGauge = IsPositiveModResourceValue(SafeMaxGauge)
			? FMath::Clamp(SafeGauge, 0.0f, SafeMaxGauge)
			: SafeGauge;
		Result.ModStack = IsPositiveModResourceValue(SafeMaxStack)
			? FMath::Clamp(SafeStack, 0.0f, SafeMaxStack)
			: SafeStack;

		// 최대 스택 도달 상태
		if (IsModStackAtMax(Result.ModStack, SafeMaxStack))
		{
			Result.ModStack = SafeMaxStack;
			Result.ModGauge = 0.0f;
			return Result;
		}

		// 게이지 최대치 미설정
		if (!IsPositiveModResourceValue(SafeMaxGauge))
		{
			return Result;
		}

		// 스택 미사용 모드
		if (!IsStackResourceEnabled(SafeMaxStack))
		{
			Result.ModStack = 0.0f;
			return Result;
		}

		// 충전 처리 제외 상태
		if (!bAllowStackCharge)
		{
			return Result;
		}

		// 게이지 미충전 상태
		if (!IsModGaugeFull(SafeGauge, SafeMaxGauge))
		{
			return Result;
		}

		// 스택 충전
		Result.ModStack = FMath::Min(Result.ModStack + 1.0f, SafeMaxStack);
		if (IsModStackAtMax(Result.ModStack, SafeMaxStack))
		{
			Result.ModStack = SafeMaxStack;
			Result.ModGauge = 0.0f;
			return Result;
		}

		// 잔여 게이지 반영
		const float RemainingGauge = SafeGauge - SafeMaxGauge;
		if (FMath::IsNearlyZero(RemainingGauge, ModResourceTolerance))
		{
			Result.ModGauge = 0.0f;
		}
		else
		{
			Result.ModGauge = FMath::Clamp(RemainingGauge, 0.0f, SafeMaxGauge);
		}
		return Result;
	}
}

UPRAttributeSet_Weapon::UPRAttributeSet_Weapon()
{
	InitBaseDamage(0.0f);
	InitDamageMultiplier(1.0f);
	InitGroggyDamageMultiplier(0.0f);
	InitWeakpointMultiplier(1.0f);
}

// =====  UPRAttributeSet_Weapon Interface =====
FGameplayAttribute UPRAttributeSet_Weapon::GetMagazineAmmoAttribute(EPRAmmoType AmmoType)
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryMagazineAmmoAttribute()
		: GetSecondaryMagazineAmmoAttribute();
}

FGameplayAttribute UPRAttributeSet_Weapon::GetMaxMagazineAmmoAttribute(EPRAmmoType AmmoType)
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryMaxMagazineAmmoAttribute()
		: GetSecondaryMaxMagazineAmmoAttribute();
}

FGameplayAttribute UPRAttributeSet_Weapon::GetReserveAmmoAttribute(EPRAmmoType AmmoType)
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryReserveAmmoAttribute()
		: GetSecondaryReserveAmmoAttribute();
}

FGameplayAttribute UPRAttributeSet_Weapon::GetMaxReserveAmmoAttribute(EPRAmmoType AmmoType)
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryMaxReserveAmmoAttribute()
		: GetSecondaryMaxReserveAmmoAttribute();
}

float UPRAttributeSet_Weapon::GetMagazineAmmoByType(EPRAmmoType AmmoType) const
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryMagazineAmmo()
		: GetSecondaryMagazineAmmo();
}

float UPRAttributeSet_Weapon::GetMaxMagazineAmmoByType(EPRAmmoType AmmoType) const
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryMaxMagazineAmmo()
		: GetSecondaryMaxMagazineAmmo();
}

float UPRAttributeSet_Weapon::GetReserveAmmoByType(EPRAmmoType AmmoType) const
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryReserveAmmo()
		: GetSecondaryReserveAmmo();
}

float UPRAttributeSet_Weapon::GetMaxReserveAmmoByType(EPRAmmoType AmmoType) const
{
	return AmmoType == EPRAmmoType::Primary
		? GetPrimaryMaxReserveAmmo()
		: GetSecondaryMaxReserveAmmo();
}

// =====  UAttributeSet Interface =====
void UPRAttributeSet_Weapon::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (IsPrimarySlotAttribute(Attribute) || IsSecondarySlotAttribute(Attribute)
		|| Attribute == GetBaseDamageAttribute()
		|| Attribute == GetDamageMultiplierAttribute()
		|| Attribute == GetArmorPenetrationAttribute()
		|| Attribute == GetWeakpointMultiplierAttribute()
		|| Attribute == GetGroggyDamageMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}

	if (Attribute == GetPrimaryMagazineAmmoAttribute())
	{
		const float MaxMagazine = GetPrimaryMaxMagazineAmmo();
		NewValue = MaxMagazine > 0.0f ? FMath::Min(NewValue, MaxMagazine) : 0.0f;
	}
	else if (Attribute == GetSecondaryMagazineAmmoAttribute())
	{
		const float MaxMagazine = GetSecondaryMaxMagazineAmmo();
		NewValue = MaxMagazine > 0.0f ? FMath::Min(NewValue, MaxMagazine) : 0.0f;
	}

	if (Attribute == GetPrimaryModGaugeAttribute()
		|| Attribute == GetPrimaryMaxModGaugeAttribute()
		|| Attribute == GetPrimaryModStackAttribute()
		|| Attribute == GetPrimaryMaxModStackAttribute()
		|| Attribute == GetSecondaryModGaugeAttribute()
		|| Attribute == GetSecondaryMaxModGaugeAttribute()
		|| Attribute == GetSecondaryModStackAttribute()
		|| Attribute == GetSecondaryMaxModStackAttribute())
	{
		NewValue = SanitizeModResourceValue(NewValue);
	}

	if (Attribute == GetPrimaryModStackAttribute())
	{
		const float MaxStack = SanitizeModResourceValue(GetPrimaryMaxModStack());
		if (IsPositiveModResourceValue(MaxStack))
		{
			NewValue = FMath::Min(NewValue, MaxStack);
		}
	}
	else if (Attribute == GetSecondaryModStackAttribute())
	{
		const float MaxStack = SanitizeModResourceValue(GetSecondaryMaxModStack());
		if (IsPositiveModResourceValue(MaxStack))
		{
			NewValue = FMath::Min(NewValue, MaxStack);
		}
	}

	if (Attribute == GetPrimaryReserveAmmoAttribute())
	{
		const float MaxReserve = GetPrimaryMaxReserveAmmo();
		NewValue = MaxReserve > 0.0f ? FMath::Min(NewValue, MaxReserve) : 0.0f;
	}
	else if (Attribute == GetSecondaryReserveAmmoAttribute())
	{
		const float MaxReserve = GetSecondaryMaxReserveAmmo();
		NewValue = MaxReserve > 0.0f ? FMath::Min(NewValue, MaxReserve) : 0.0f;
	}
}

void UPRAttributeSet_Weapon::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& Attribute = Data.EvaluatedData.Attribute;
	if (Attribute == GetPrimaryMagazineAmmoAttribute() || Attribute == GetPrimaryMaxMagazineAmmoAttribute())
	{
		const float MaxMagazine = GetPrimaryMaxMagazineAmmo();
		SetPrimaryMagazineAmmo(MaxMagazine > 0.0f ? FMath::Clamp(GetPrimaryMagazineAmmo(), 0.0f, MaxMagazine) : 0.0f);
	}
	else if (Attribute == GetPrimaryReserveAmmoAttribute() || Attribute == GetPrimaryMaxReserveAmmoAttribute())
	{
		const float MaxReserve = GetPrimaryMaxReserveAmmo();
		SetPrimaryReserveAmmo(MaxReserve > 0.0f ? FMath::Clamp(GetPrimaryReserveAmmo(), 0.0f, MaxReserve) : 0.0f);
	}
	else if (Attribute == GetSecondaryMagazineAmmoAttribute() || Attribute == GetSecondaryMaxMagazineAmmoAttribute())
	{
		const float MaxMagazine = GetSecondaryMaxMagazineAmmo();
		SetSecondaryMagazineAmmo(MaxMagazine > 0.0f ? FMath::Clamp(GetSecondaryMagazineAmmo(), 0.0f, MaxMagazine) : 0.0f);
	}
	else if (Attribute == GetSecondaryReserveAmmoAttribute() || Attribute == GetSecondaryMaxReserveAmmoAttribute())
	{
		const float MaxReserve = GetSecondaryMaxReserveAmmo();
		SetSecondaryReserveAmmo(MaxReserve > 0.0f ? FMath::Clamp(GetSecondaryReserveAmmo(), 0.0f, MaxReserve) : 0.0f);
	}
	else if (Attribute == GetPrimaryModGaugeAttribute())
	{
		const float MaxGauge = GetPrimaryMaxModGauge();
		const FPRModResourceResult Result = ResolveModGaugeOverflow(
			GetPrimaryModGauge(),
			MaxGauge,
			GetPrimaryModStack(),
			GetPrimaryMaxModStack(),
			true);
		if (!FMath::IsNearlyEqual(GetPrimaryModStack(), Result.ModStack, ModResourceTolerance))
		{
			SetPrimaryModStack(Result.ModStack);
		}
		SetPrimaryModGauge(Result.ModGauge);
	}
	else if (Attribute == GetPrimaryMaxModGaugeAttribute())
	{
		const FPRModResourceResult Result = ResolveModGaugeOverflow(
			GetPrimaryModGauge(),
			GetPrimaryMaxModGauge(),
			GetPrimaryModStack(),
			GetPrimaryMaxModStack(),
			false);
		if (!FMath::IsNearlyEqual(GetPrimaryModStack(), Result.ModStack, ModResourceTolerance))
		{
			SetPrimaryModStack(Result.ModStack);
		}
		SetPrimaryModGauge(Result.ModGauge);
	}
	else if (Attribute == GetSecondaryModGaugeAttribute())
	{
		const float MaxGauge = GetSecondaryMaxModGauge();
		const FPRModResourceResult Result = ResolveModGaugeOverflow(
			GetSecondaryModGauge(),
			MaxGauge,
			GetSecondaryModStack(),
			GetSecondaryMaxModStack(),
			true);
		if (!FMath::IsNearlyEqual(GetSecondaryModStack(), Result.ModStack, ModResourceTolerance))
		{
			SetSecondaryModStack(Result.ModStack);
		}
		SetSecondaryModGauge(Result.ModGauge);
	}
	else if (Attribute == GetSecondaryMaxModGaugeAttribute())
	{
		const FPRModResourceResult Result = ResolveModGaugeOverflow(
			GetSecondaryModGauge(),
			GetSecondaryMaxModGauge(),
			GetSecondaryModStack(),
			GetSecondaryMaxModStack(),
			false);
		if (!FMath::IsNearlyEqual(GetSecondaryModStack(), Result.ModStack, ModResourceTolerance))
		{
			SetSecondaryModStack(Result.ModStack);
		}
		SetSecondaryModGauge(Result.ModGauge);
	}
	else if (Attribute == GetPrimaryModStackAttribute())
	{
		const FPRModResourceResult Result = ResolveModGaugeOverflow(
			GetPrimaryModGauge(),
			GetPrimaryMaxModGauge(),
			GetPrimaryModStack(),
			GetPrimaryMaxModStack(),
			false);
		SetPrimaryModStack(Result.ModStack);
		SetPrimaryModGauge(Result.ModGauge);
	}
	else if (Attribute == GetPrimaryMaxModStackAttribute())
	{
		const FPRModResourceResult Result = ResolveModGaugeOverflow(
			GetPrimaryModGauge(),
			GetPrimaryMaxModGauge(),
			GetPrimaryModStack(),
			GetPrimaryMaxModStack(),
			false);
		SetPrimaryModStack(Result.ModStack);
		SetPrimaryModGauge(Result.ModGauge);
	}
	else if (Attribute == GetSecondaryModStackAttribute())
	{
		const FPRModResourceResult Result = ResolveModGaugeOverflow(
			GetSecondaryModGauge(),
			GetSecondaryMaxModGauge(),
			GetSecondaryModStack(),
			GetSecondaryMaxModStack(),
			false);
		SetSecondaryModStack(Result.ModStack);
		SetSecondaryModGauge(Result.ModGauge);
	}
	else if (Attribute == GetSecondaryMaxModStackAttribute())
	{
		const FPRModResourceResult Result = ResolveModGaugeOverflow(
			GetSecondaryModGauge(),
			GetSecondaryMaxModGauge(),
			GetSecondaryModStack(),
			GetSecondaryMaxModStack(),
			false);
		SetSecondaryModStack(Result.ModStack);
		SetSecondaryModGauge(Result.ModGauge);
	}
}

void UPRAttributeSet_Weapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryMagazineAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryMaxMagazineAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryMaxReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryModGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryMaxModGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryModStack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, PrimaryMaxModStack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryMagazineAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryMaxMagazineAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryMaxReserveAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryModGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryMaxModGauge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryModStack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, SecondaryMaxModStack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, BaseDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, DamageMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, ArmorPenetration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, WeakpointMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Weapon, GroggyDamageMultiplier, COND_None, REPNOTIFY_Always);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryMagazineAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryMagazineAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryMaxMagazineAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryMaxMagazineAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryReserveAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryReserveAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryMaxReserveAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryMaxReserveAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryModGauge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryModGauge, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryMaxModGauge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryMaxModGauge, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryModStack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryModStack, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_PrimaryMaxModStack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, PrimaryMaxModStack, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryMagazineAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryMagazineAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryMaxMagazineAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryMaxMagazineAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryReserveAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryReserveAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryMaxReserveAmmo(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryMaxReserveAmmo, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryModGauge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryModGauge, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryMaxModGauge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryMaxModGauge, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryModStack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryModStack, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_SecondaryMaxModStack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, SecondaryMaxModStack, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_BaseDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, BaseDamage, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_DamageMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, DamageMultiplier, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_ArmorPenetration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, ArmorPenetration, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_WeakpointMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, WeakpointMultiplier, OldValue);
}

void UPRAttributeSet_Weapon::OnRep_GroggyDamageMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Weapon, GroggyDamageMultiplier, OldValue);
}
