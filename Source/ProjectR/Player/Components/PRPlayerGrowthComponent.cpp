// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (경험치 획득 기반 레벨업 처리 및 특성 투자 시스템 구현)
// Author: 배유찬 (사망 리스폰 시 경험치 페널티 구현)
#include "PRPlayerGrowthComponent.h"

#include "AbilitySystemComponent.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Growth.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Player.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PRDeveloperSettings.h"

UPRPlayerGrowthComponent::UPRPlayerGrowthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPRPlayerGrowthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UPRPlayerGrowthComponent, TraitInvestmentInfo, COND_OwnerOnly);
}

bool UPRPlayerGrowthComponent::AddExperience(int32 Amount)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || Amount <= 0)
	{
		return false;
	}

	UPRAbilitySystemComponent* ASC = GetPRASC();
	if (!IsValid(ASC))
	{
		return false;
	}

	const int64 CurrentExperience = FMath::Max(static_cast<int64>(ASC->GetNumericAttribute(UPRAttributeSet_Growth::GetExperienceAttribute())), 0);
	const int32 PreviousLevel = FMath::Max(FMath::RoundToInt(ASC->GetNumericAttribute(UPRAttributeSet_Growth::GetLevelAttribute())), 1);
	const int64 NewExperience = CurrentExperience + static_cast<int64>(Amount);
	const int32 NewLevel = ResolveLevelFromExperience(NewExperience);
	ApplyGrowthAttributes(NewExperience, NewLevel, TraitInvestmentInfo);
	if (NewLevel > PreviousLevel)
	{
		if (APRPlayerState* PlayerState = Cast<APRPlayerState>(GetOwner()))
		{
			if (APRPlayerController* PlayerController = Cast<APRPlayerController>(PlayerState->GetOwner()))
			{
				PlayerController->ClientNotifyPlayerLevelUp(PreviousLevel, NewLevel);
			}
		}
	}
	return true;
}

FPRTraitInvestmentResult UPRPlayerGrowthComponent::ConfirmTraitInvestment(const FPRTraitInvestmentInfo& DesiredInvestment)
{
	FPRTraitInvestmentResult Result;
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		Result.FailReason = EPRTraitInvestmentFailReason::InvalidPlayerState;
		return Result;
	}

	EPRTraitInvestmentFailReason FailReason = EPRTraitInvestmentFailReason::None;
	if (!ValidateInvestment(DesiredInvestment, FailReason))
	{
		Result.FailReason = FailReason;
		return Result;
	}

	UPRAbilitySystemComponent* ASC = GetPRASC();
	if (!IsValid(ASC))
	{
		Result.FailReason = EPRTraitInvestmentFailReason::InvalidAbilitySystem;
		return Result;
	}

	const int64 CurrentExperience = FMath::Max(static_cast<int64>(ASC->GetNumericAttribute(UPRAttributeSet_Growth::GetExperienceAttribute())), 0);
	const int32 CurrentLevel = FMath::Max(FMath::RoundToInt(ASC->GetNumericAttribute(UPRAttributeSet_Growth::GetLevelAttribute())), 1);
	ApplyGrowthAttributes(CurrentExperience, CurrentLevel, DesiredInvestment);

	Result.bSuccess = true;
	Result.InvestmentInfo = TraitInvestmentInfo;
	return Result;
}

FPRTraitInvestmentResult UPRPlayerGrowthComponent::ResetTraitInvestment()
{
	FPRTraitInvestmentResult Result;
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		Result.FailReason = EPRTraitInvestmentFailReason::InvalidPlayerState;
		return Result;
	}

	UPRAbilitySystemComponent* ASC = GetPRASC();
	if (!IsValid(ASC))
	{
		Result.FailReason = EPRTraitInvestmentFailReason::InvalidAbilitySystem;
		return Result;
	}

	const int64 CurrentExperience = FMath::Max(static_cast<int64>(ASC->GetNumericAttribute(UPRAttributeSet_Growth::GetExperienceAttribute())), 0);
	const int32 CurrentLevel = FMath::Max(FMath::RoundToInt(ASC->GetNumericAttribute(UPRAttributeSet_Growth::GetLevelAttribute())), 1);
	ApplyGrowthAttributes(CurrentExperience, CurrentLevel, FPRTraitInvestmentInfo());

	Result.bSuccess = true;
	Result.InvestmentInfo = TraitInvestmentInfo;
	return Result;
}

bool UPRPlayerGrowthComponent::GetTraitRule(EPRTraitStatType TraitType, FPRTraitStatRuleRow& OutRule) const
{
	return FindTraitRule(TraitType, OutRule);
}

void UPRPlayerGrowthComponent::ApplyGrowthSaveData(int64 SavedExperience, int32 SavedLevel, const FPRCharacterStatUpgradeInfo& SavedStats)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return;
	}

	FPRTraitInvestmentInfo ClampedInvestment = SavedStats.TraitInvestment;
	for (int32 TraitIndex = static_cast<int32>(EPRTraitStatType::MaxHealth);
		TraitIndex <= static_cast<int32>(EPRTraitStatType::CriticalDamageMultiplier);
		++TraitIndex)
	{
		const EPRTraitStatType TraitType = static_cast<EPRTraitStatType>(TraitIndex);
		FPRTraitStatRuleRow Rule;
		const int32 MaxPoint = FindTraitRule(TraitType, Rule) ? Rule.MaxPoint : 0;
		const int32 CurrentPoint = GetInvestmentPoint(ClampedInvestment, TraitType);
		SetInvestmentPoint(ClampedInvestment, TraitType, FMath::Clamp(CurrentPoint, 0, MaxPoint));
	}

	const int64 ClampedExperience = FMath::Max(SavedExperience, static_cast<int64>(0));
	const int32 CalculatedLevel = ResolveLevelFromExperience(ClampedExperience);
	const int32 NewLevel = FMath::Max(CalculatedLevel, 1);
	const int32 TotalEarnedPoint = GetTotalEarnedPoint(NewLevel);
	int32 OverflowPoint = GetTotalSpentPoint(ClampedInvestment) - TotalEarnedPoint;
	for (int32 TraitIndex = static_cast<int32>(EPRTraitStatType::CriticalDamageMultiplier);
		TraitIndex >= static_cast<int32>(EPRTraitStatType::MaxHealth) && OverflowPoint > 0;
		--TraitIndex)
	{
		const EPRTraitStatType TraitType = static_cast<EPRTraitStatType>(TraitIndex);
		const int32 CurrentPoint = GetInvestmentPoint(ClampedInvestment, TraitType);
		const int32 ReducePoint = FMath::Min(CurrentPoint, OverflowPoint);
		SetInvestmentPoint(ClampedInvestment, TraitType, CurrentPoint - ReducePoint);
		OverflowPoint -= ReducePoint;
	}

	ApplyGrowthAttributes(ClampedExperience, NewLevel, ClampedInvestment);
}

FPRCharacterStatUpgradeInfo UPRPlayerGrowthComponent::MakeGrowthSaveData() const
{
	FPRCharacterStatUpgradeInfo SaveData;
	SaveData.TraitInvestment = TraitInvestmentInfo;

	const UPRAbilitySystemComponent* ASC = GetPRASC();
	if (IsValid(ASC))
	{
		SaveData.UnspentTraitPoint = FMath::Max(FMath::RoundToInt(ASC->GetNumericAttribute(UPRAttributeSet_Growth::GetTraitPointAttribute())), 0);
	}
	return SaveData;
}

void UPRPlayerGrowthComponent::ResetSystem()
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return;
	}

	// 특성 보너스 효과 재적용
	ApplyTraitBonusEffect();
	OnTraitInvestmentChanged.Broadcast(TraitInvestmentInfo);
}

void UPRPlayerGrowthComponent::OnRep_TraitInvestmentInfo()
{
	OnTraitInvestmentChanged.Broadcast(TraitInvestmentInfo);
}

UPRAbilitySystemComponent* UPRPlayerGrowthComponent::GetPRASC() const
{
	const APRPlayerState* PlayerState = Cast<APRPlayerState>(GetOwner());
	return IsValid(PlayerState) ? PlayerState->GetPRAbilitySystemComponent() : nullptr;
}

UDataTable* UPRPlayerGrowthComponent::ResolveLevelExperienceTable() const
{
	if (IsValid(LevelExperienceTable))
	{
		return LevelExperienceTable;
	}

	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	return Settings != nullptr ? Settings->LevelExperienceTable.LoadSynchronous() : nullptr;
}

UDataTable* UPRPlayerGrowthComponent::ResolveTraitStatRuleTable() const
{
	if (IsValid(TraitStatRuleTable))
	{
		return TraitStatRuleTable;
	}

	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	return Settings != nullptr ? Settings->TraitStatRuleTable.LoadSynchronous() : nullptr;
}

TSubclassOf<UGameplayEffect> UPRPlayerGrowthComponent::ResolveTraitBonusEffectClass() const
{
	if (IsValid(TraitBonusEffectClass.Get()))
	{
		return TraitBonusEffectClass;
	}

	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	return Settings != nullptr ? Settings->TraitBonusEffectClass.LoadSynchronous() : nullptr;
}

int32 UPRPlayerGrowthComponent::ResolveLevelFromExperience(int64 InExperience) const
{
	UDataTable* Table = ResolveLevelExperienceTable();
	if (!IsValid(Table))
	{
		return 1;
	}

	int32 ResolvedLevel = 1;
	for (const TPair<FName, uint8*>& RowPair : Table->GetRowMap())
	{
		const FPRLevelExperienceRow* Row = reinterpret_cast<const FPRLevelExperienceRow*>(RowPair.Value);
		if (Row == nullptr)
		{
			continue;
		}

		int32 RowLevel = 0;
		if (!LexTryParseString(RowLevel, *RowPair.Key.ToString()) || RowLevel < 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Growth] 레벨 경험치 테이블 RowName은 1 이상의 숫자여야 합니다. RowName = %s"), *RowPair.Key.ToString());
			continue;
		}

		if (Row->RequiredTotalExperience <= InExperience)
		{
			ResolvedLevel = FMath::Max(ResolvedLevel, RowLevel);
		}
	}
	return FMath::Max(ResolvedLevel, 1);
}

int32 UPRPlayerGrowthComponent::GetTotalEarnedPoint(int32 InLevel) const
{
	UDataTable* Table = ResolveLevelExperienceTable();
	if (!IsValid(Table))
	{
		return 0;
	}

	int32 TotalEarnedPoint = 0;
	for (const TPair<FName, uint8*>& RowPair : Table->GetRowMap())
	{
		const FPRLevelExperienceRow* Row = reinterpret_cast<const FPRLevelExperienceRow*>(RowPair.Value);
		if (Row == nullptr)
		{
			continue;
		}

		int32 RowLevel = 0;
		if (!LexTryParseString(RowLevel, *RowPair.Key.ToString()) || RowLevel < 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Growth] 레벨 경험치 테이블 RowName은 1 이상의 숫자여야 합니다. RowName = %s"), *RowPair.Key.ToString());
			continue;
		}

		if (RowLevel > 1 && RowLevel <= InLevel)
		{
			TotalEarnedPoint += FMath::Max(Row->TraitPointReward, 0);
		}
	}

	return TotalEarnedPoint;
}

int32 UPRPlayerGrowthComponent::GetInvestmentPoint(const FPRTraitInvestmentInfo& InvestmentInfo, EPRTraitStatType TraitType) const
{
	switch (TraitType)
	{
	case EPRTraitStatType::MaxHealth:
		return InvestmentInfo.MaxHealth;
	case EPRTraitStatType::Armor:
		return InvestmentInfo.Armor;
	case EPRTraitStatType::MovementSpeed:
		return InvestmentInfo.MovementSpeed;
	case EPRTraitStatType::AttackPower:
		return InvestmentInfo.AttackPower;
	case EPRTraitStatType::MaxStamina:
		return InvestmentInfo.MaxStamina;
	case EPRTraitStatType::CriticalHitChance:
		return InvestmentInfo.CriticalHitChance;
	case EPRTraitStatType::CriticalDamageMultiplier:
		return InvestmentInfo.CriticalDamageMultiplier;
	default:
		return 0;
	}
}

void UPRPlayerGrowthComponent::SetInvestmentPoint(FPRTraitInvestmentInfo& InvestmentInfo, EPRTraitStatType TraitType, int32 Point) const
{
	switch (TraitType)
	{
	case EPRTraitStatType::MaxHealth:
		InvestmentInfo.MaxHealth = Point;
		break;
	case EPRTraitStatType::Armor:
		InvestmentInfo.Armor = Point;
		break;
	case EPRTraitStatType::MovementSpeed:
		InvestmentInfo.MovementSpeed = Point;
		break;
	case EPRTraitStatType::AttackPower:
		InvestmentInfo.AttackPower = Point;
		break;
	case EPRTraitStatType::MaxStamina:
		InvestmentInfo.MaxStamina = Point;
		break;
	case EPRTraitStatType::CriticalHitChance:
		InvestmentInfo.CriticalHitChance = Point;
		break;
	case EPRTraitStatType::CriticalDamageMultiplier:
		InvestmentInfo.CriticalDamageMultiplier = Point;
		break;
	default:
		break;
	}
}

int32 UPRPlayerGrowthComponent::GetTotalSpentPoint(const FPRTraitInvestmentInfo& InvestmentInfo) const
{
	return InvestmentInfo.MaxHealth
		+ InvestmentInfo.Armor
		+ InvestmentInfo.MovementSpeed
		+ InvestmentInfo.AttackPower
		+ InvestmentInfo.MaxStamina
		+ InvestmentInfo.CriticalHitChance
		+ InvestmentInfo.CriticalDamageMultiplier;
}

bool UPRPlayerGrowthComponent::ValidateInvestment(const FPRTraitInvestmentInfo& DesiredInvestment, EPRTraitInvestmentFailReason& OutFailReason) const
{
	UDataTable* RuleTable = ResolveTraitStatRuleTable();
	if (!IsValid(RuleTable))
	{
		OutFailReason = EPRTraitInvestmentFailReason::InvalidDataTable;
		return false;
	}

	for (int32 TraitIndex = static_cast<int32>(EPRTraitStatType::MaxHealth);
		TraitIndex <= static_cast<int32>(EPRTraitStatType::CriticalDamageMultiplier);
		++TraitIndex)
	{
		const EPRTraitStatType TraitType = static_cast<EPRTraitStatType>(TraitIndex);
		const int32 RequestedPoint = GetInvestmentPoint(DesiredInvestment, TraitType);
		if (RequestedPoint < 0)
		{
			OutFailReason = EPRTraitInvestmentFailReason::NegativePoint;
			return false;
		}

		FPRTraitStatRuleRow Rule;
		if (!FindTraitRule(TraitType, Rule) || RequestedPoint > Rule.MaxPoint)
		{
			OutFailReason = EPRTraitInvestmentFailReason::ExceedTraitMaxPoint;
			return false;
		}
	}

	const UPRAbilitySystemComponent* ASC = GetPRASC();
	if (!IsValid(ASC))
	{
		OutFailReason = EPRTraitInvestmentFailReason::InvalidAbilitySystem;
		return false;
	}

	const int32 CurrentLevel = FMath::Max(FMath::RoundToInt(ASC->GetNumericAttribute(UPRAttributeSet_Growth::GetLevelAttribute())), 1);
	if (GetTotalSpentPoint(DesiredInvestment) > GetTotalEarnedPoint(CurrentLevel))
	{
		OutFailReason = EPRTraitInvestmentFailReason::ExceedTotalPoint;
		return false;
	}

	OutFailReason = EPRTraitInvestmentFailReason::None;
	return true;
}

void UPRPlayerGrowthComponent::ApplyGrowthAttributes(int64 NewExperience, int32 NewLevel, const FPRTraitInvestmentInfo& NewInvestment)
{
	UPRAbilitySystemComponent* ASC = GetPRASC();
	if (!IsValid(ASC))
	{
		return;
	}

	TraitInvestmentInfo = NewInvestment;
	const int32 NewSpentPoint = GetTotalSpentPoint(TraitInvestmentInfo);
	const int32 NewTraitPoint = FMath::Max(GetTotalEarnedPoint(NewLevel) - NewSpentPoint, 0);

	ASC->SetNumericAttributeBase(UPRAttributeSet_Growth::GetExperienceAttribute(), static_cast<float>(FMath::Max(NewExperience, static_cast<int64>(0))));
	ASC->SetNumericAttributeBase(UPRAttributeSet_Growth::GetLevelAttribute(), static_cast<float>(FMath::Max(NewLevel, 1)));
	ASC->SetNumericAttributeBase(UPRAttributeSet_Growth::GetSpentPointAttribute(), static_cast<float>(NewSpentPoint));
	ASC->SetNumericAttributeBase(UPRAttributeSet_Growth::GetTraitPointAttribute(), static_cast<float>(NewTraitPoint));

	if (APRPlayerState* PlayerState = Cast<APRPlayerState>(GetOwner()))
	{
		FPRCharacterStatUpgradeInfo CacheStats;
		CacheStats.UnspentTraitPoint = NewTraitPoint;
		CacheStats.TraitInvestment = TraitInvestmentInfo;
		PlayerState->SyncGrowthCache(NewExperience, NewLevel, CacheStats);
	}

	ApplyTraitBonusEffect();
	OnTraitInvestmentChanged.Broadcast(TraitInvestmentInfo);
	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}

void UPRPlayerGrowthComponent::ApplyTraitBonusEffect()
{
	UPRAbilitySystemComponent* ASC = GetPRASC();
	if (!IsValid(ASC))
	{
		return;
	}

	const float OldMaxHealth = ASC->GetNumericAttribute(UPRAttributeSet_Common::GetMaxHealthAttribute());
	const float OldMaxStamina = ASC->GetNumericAttribute(UPRAttributeSet_Player::GetMaxStaminaAttribute());
	RemoveTraitBonusEffect();

	const TSubclassOf<UGameplayEffect> EffectClass = ResolveTraitBonusEffectClass();
	if (!IsValid(EffectClass.Get()))
	{
		ClampCurrentVitals(OldMaxHealth, OldMaxStamina);
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, Context);
	if (!SpecHandle.IsValid())
	{
		ClampCurrentVitals(OldMaxHealth, OldMaxStamina);
		return;
	}

	TMap<FGameplayTag, float> BonusMap;
	BuildTraitBonusMap(BonusMap);
	for (const TPair<FGameplayTag, float>& BonusPair : BonusMap)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(BonusPair.Key, BonusPair.Value);
	}

	TraitBonusEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	ClampCurrentVitals(OldMaxHealth, OldMaxStamina);
}

void UPRPlayerGrowthComponent::RemoveTraitBonusEffect()
{
	UPRAbilitySystemComponent* ASC = GetPRASC();
	if (IsValid(ASC) && TraitBonusEffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(TraitBonusEffectHandle);
	}
	TraitBonusEffectHandle.Invalidate();
}

void UPRPlayerGrowthComponent::ClampCurrentVitals(float OldMaxHealth, float OldMaxStamina)
{
	UPRAbilitySystemComponent* ASC = GetPRASC();
	if (!IsValid(ASC))
	{
		return;
	}

	const float NewMaxHealth = ASC->GetNumericAttribute(UPRAttributeSet_Common::GetMaxHealthAttribute());
	const float NewMaxStamina = ASC->GetNumericAttribute(UPRAttributeSet_Player::GetMaxStaminaAttribute());
	const float CurrentHealth = ASC->GetNumericAttribute(UPRAttributeSet_Common::GetHealthAttribute());
	const float CurrentStamina = ASC->GetNumericAttribute(UPRAttributeSet_Player::GetStaminaAttribute());
	const float HealthDelta = FMath::Max(NewMaxHealth - OldMaxHealth, 0.0f);
	const float StaminaDelta = FMath::Max(NewMaxStamina - OldMaxStamina, 0.0f);

	ASC->SetNumericAttributeBase(UPRAttributeSet_Common::GetHealthAttribute(), FMath::Clamp(CurrentHealth + HealthDelta, 0.0f, NewMaxHealth));
	ASC->SetNumericAttributeBase(UPRAttributeSet_Player::GetStaminaAttribute(), FMath::Clamp(CurrentStamina + StaminaDelta, 0.0f, NewMaxStamina));
}

void UPRPlayerGrowthComponent::BuildTraitBonusMap(TMap<FGameplayTag, float>& OutBonusMap) const
{
	OutBonusMap.Reset();
	for (int32 TraitIndex = static_cast<int32>(EPRTraitStatType::MaxHealth);
		TraitIndex <= static_cast<int32>(EPRTraitStatType::CriticalDamageMultiplier);
		++TraitIndex)
	{
		const EPRTraitStatType TraitType = static_cast<EPRTraitStatType>(TraitIndex);
		FPRTraitStatRuleRow Rule;
		if (!FindTraitRule(TraitType, Rule))
		{
			continue;
		}

		const float BonusValue = static_cast<float>(GetInvestmentPoint(TraitInvestmentInfo, TraitType)) * Rule.ValuePerPoint;
		switch (TraitType)
		{
		case EPRTraitStatType::MaxHealth:
			OutBonusMap.Add(PRGameplayTags::SetByCaller_Trait_MaxHealth, BonusValue);
			break;
		case EPRTraitStatType::Armor:
			OutBonusMap.Add(PRGameplayTags::SetByCaller_Trait_Armor, BonusValue);
			break;
		case EPRTraitStatType::MovementSpeed:
			OutBonusMap.Add(PRGameplayTags::SetByCaller_Trait_MovementSpeed, BonusValue);
			break;
		case EPRTraitStatType::AttackPower:
			OutBonusMap.Add(PRGameplayTags::SetByCaller_Trait_PlayerAttackPower, BonusValue);
			break;
		case EPRTraitStatType::MaxStamina:
			OutBonusMap.Add(PRGameplayTags::SetByCaller_Trait_MaxStamina, BonusValue);
			break;
		case EPRTraitStatType::CriticalHitChance:
			OutBonusMap.Add(PRGameplayTags::SetByCaller_Trait_CriticalHitChance, BonusValue);
			break;
		case EPRTraitStatType::CriticalDamageMultiplier:
			OutBonusMap.Add(PRGameplayTags::SetByCaller_Trait_CriticalDamageMultiplier, BonusValue);
			break;
		default:
			break;
		}
	}
}

bool UPRPlayerGrowthComponent::FindTraitRule(EPRTraitStatType TraitType, FPRTraitStatRuleRow& OutRule) const
{
	UDataTable* RuleTable = ResolveTraitStatRuleTable();
	if (!IsValid(RuleTable))
	{
		return false;
	}

	TArray<FPRTraitStatRuleRow*> Rows;
	RuleTable->GetAllRows(TEXT("UPRPlayerGrowthComponent::FindTraitRule"), Rows);
	for (const FPRTraitStatRuleRow* Row : Rows)
	{
		if (Row != nullptr && Row->TraitType == TraitType)
		{
			OutRule = *Row;
			return true;
		}
	}
	return false;
}
