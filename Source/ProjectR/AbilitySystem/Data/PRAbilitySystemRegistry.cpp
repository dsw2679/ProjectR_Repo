// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (어빌리티 레지스트리 초기 구축 및 무기/상태 데이터 캐싱 구현)
// Author: 이건주 (Mod 버프 관련 무기 데이터 매핑 구현)
#include "PRAbilitySystemRegistry.h"

#include "PRStatRows.h"
#include "GameplayEffect.h"


#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

// =====  조회 =====

FGameplayAttribute UPRAbilitySystemRegistry::FindAttribute(FName PropertyName) const
{
	if (const FGameplayAttribute* Found = PropertyToAttribute.Find(PropertyName))
	{
		return *Found;
	}
	return FGameplayAttribute();
}

UDataTable* UPRAbilitySystemRegistry::GetStatTableSynchronous(EPRCharacterRole Role) const
{
	if (const TSoftObjectPtr<UDataTable>* SoftPtr = StatTables.Find(Role))
	{
		if (!SoftPtr->IsNull())
		{
			return SoftPtr->LoadSynchronous();
		}
	}
	return nullptr;
}

const TArray<FGameplayAttribute>& UPRAbilitySystemRegistry::GetPersistentBaseAttributes(EPRCharacterRole Role) const
{
	if (const FPRPersistentAttributeList* AttributeList = PersistentBaseAttributes.Find(Role))
	{
		return AttributeList->Attributes;
	}

	static const TArray<FGameplayAttribute> EmptyAttributes;
	return EmptyAttributes;
}

// =====  Data Validation =====

#if WITH_EDITOR
EDataValidationResult UPRAbilitySystemRegistry::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// 1) StatTables 엔트리 존재 여부
	const UEnum* RoleEnum = StaticEnum<EPRCharacterRole>();
	if (IsValid(RoleEnum))
	{
		for (int32 i = 0; i < RoleEnum->NumEnums() - 1; ++i)
		{
			const EPRCharacterRole Role = static_cast<EPRCharacterRole>(RoleEnum->GetValueByIndex(i));
			if (!StatTables.Contains(Role))
			{
				Context.AddError(FText::FromString(
					FString::Printf(TEXT("StatTables missing entry for Role %s"),
						*RoleEnum->GetNameStringByIndex(i))));
				Result = EDataValidationResult::Invalid;
			}
		}
	}

	// 2) 각 DT 로드 + 3) RowStruct 일치 + 4) 매핑 커버리지 + 5) Attribute 유효성
	for (const TPair<EPRCharacterRole, TSoftObjectPtr<UDataTable>>& Pair : StatTables)
	{
		UDataTable* DT = Pair.Value.LoadSynchronous();
		if (!IsValid(DT))
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("StatTable for Role %d failed to load"),
					static_cast<int32>(Pair.Key))));
			Result = EDataValidationResult::Invalid;
			continue;
		}

		const UScriptStruct* ExpectedRowStruct = (Pair.Key == EPRCharacterRole::Player)
			? FPRPlayerStatRow::StaticStruct()
			: FPREnemyStatRow::StaticStruct();
		if (DT->GetRowStruct() != ExpectedRowStruct)
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("StatTable %s RowStruct mismatch (expected %s)"),
					*DT->GetName(), *ExpectedRowStruct->GetName())));
			Result = EDataValidationResult::Invalid;
			continue;
		}

		// 매핑 커버리지·Attribute 유효성
		for (TFieldIterator<FFloatProperty> It(ExpectedRowStruct); It; ++It)
		{
			const FName PropName = It->GetFName();
			const FGameplayAttribute* Found = PropertyToAttribute.Find(PropName);
			if (Found == nullptr)
			{
				Context.AddError(FText::FromString(
					FString::Printf(TEXT("PropertyToAttribute missing key '%s' for %s"),
						*PropName.ToString(), *DT->GetName())));
				Result = EDataValidationResult::Invalid;
			}
			else if (!Found->IsValid())
			{
				Context.AddError(FText::FromString(
					FString::Printf(TEXT("PropertyToAttribute['%s'] is invalid FGameplayAttribute"),
						*PropName.ToString())));
				Result = EDataValidationResult::Invalid;
			}
		}
	}

	// 6) 장착 GE 필수 참조 유효성
	const auto ValidateRequiredEquipGE = [&Context, &Result](const TCHAR* PropertyName, const TSubclassOf<UGameplayEffect>& EffectClass)
	{
		if (!IsValid(EffectClass))
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("EquipGE required reference '%s' is invalid"), PropertyName)));
			Result = EDataValidationResult::Invalid;
		}
	};

	ValidateRequiredEquipGE(TEXT("EquipGE_CurrentWeapon"), EquipGE_CurrentWeapon);
	ValidateRequiredEquipGE(TEXT("EquipGE_PrimaryWeapon"), EquipGE_PrimaryWeapon);
	ValidateRequiredEquipGE(TEXT("EquipGE_PrimaryMod"), EquipGE_PrimaryMod);
	ValidateRequiredEquipGE(TEXT("EquipGE_Override_PrimaryAmmo"), EquipGE_Override_PrimaryAmmo);
	ValidateRequiredEquipGE(TEXT("EquipGE_Override_PrimaryModResource"), EquipGE_Override_PrimaryModResource);
	ValidateRequiredEquipGE(TEXT("EquipGE_SecondaryWeapon"), EquipGE_SecondaryWeapon);
	ValidateRequiredEquipGE(TEXT("EquipGE_SecondaryMod"), EquipGE_SecondaryMod);
	ValidateRequiredEquipGE(TEXT("EquipGE_Override_SecondaryAmmo"), EquipGE_Override_SecondaryAmmo);
	ValidateRequiredEquipGE(TEXT("EquipGE_Override_SecondaryModResource"), EquipGE_Override_SecondaryModResource);

	return Result;
}
#endif
