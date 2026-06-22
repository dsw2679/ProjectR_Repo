// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Attribute Set 성장 구현)
#include "PRAttributeSet_Growth.h"

#include "Net/UnrealNetwork.h"

void UPRAttributeSet_Growth::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Growth, Experience, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Growth, Level, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Growth, TraitPoint, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Growth, SpentPoint, COND_OwnerOnly, REPNOTIFY_Always);
}

void UPRAttributeSet_Growth::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetExperienceAttribute()
		|| Attribute == GetTraitPointAttribute()
		|| Attribute == GetSpentPointAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetLevelAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}

void UPRAttributeSet_Growth::OnRep_Experience(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Growth, Experience, OldValue);
}

void UPRAttributeSet_Growth::OnRep_Level(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Growth, Level, OldValue);
}

void UPRAttributeSet_Growth::OnRep_TraitPoint(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Growth, TraitPoint, OldValue);
}

void UPRAttributeSet_Growth::OnRep_SpentPoint(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Growth, SpentPoint, OldValue);
}
