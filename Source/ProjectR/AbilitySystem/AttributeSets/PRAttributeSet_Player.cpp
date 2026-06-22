// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (스태미나 소모/회복 및 Poise 경직, 성장 특성 포인트 속성 구현)
// Author: 배유찬 (피해 연동용 플레이어 방어 속성 구현)
// Author: 이건주 (장비 인벤토리 연동용 능력치 속성 구현)
#include "PRAttributeSet_Player.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/PRGameplayTags.h"

namespace
{
	float ResolveRecoverableHealthMax(const UAbilitySystemComponent* AbilitySystemComponent)
	{
		if (!IsValid(AbilitySystemComponent))
		{
			return 0.0f;
		}

		const UPRAttributeSet_Common* CommonSet = AbilitySystemComponent->GetSet<UPRAttributeSet_Common>();
		if (!IsValid(CommonSet))
		{
			return 0.0f;
		}

		return FMath::Max(CommonSet->GetMaxHealth() - CommonSet->GetHealth(), 0.0f);
	}

	float ResolvePoiseDamageMax(float PoiseDamageMin, float PoiseDamageMax)
	{
		return FMath::Max(PoiseDamageMax, PoiseDamageMin);
	}

	float ResolvePoiseThreshold(float Threshold, float MinThreshold, float MaxThreshold)
	{
		return FMath::Clamp(Threshold, MinThreshold, MaxThreshold);
	}

	FGameplayTag ResolveHitReactEventTag(float OldPoiseDamage, float NewPoiseDamage, float IncomingPoiseDamage,
		float StrongHitReactThreshold, float DownHitReactThreshold)
	{
		if (OldPoiseDamage < DownHitReactThreshold && NewPoiseDamage >= DownHitReactThreshold)
		{
			return PRGameplayTags::Event_Ability_PlayerHitReact_Down;
		}

		if (OldPoiseDamage < StrongHitReactThreshold && NewPoiseDamage >= StrongHitReactThreshold)
		{
			return PRGameplayTags::Event_Ability_PlayerHitReact_Strong;
		}
		// TODO:PoiseWeakHitReactThreshold  attributeset에서 빼야하거나, 어떻게할지 정해야함
		if (IncomingPoiseDamage < StrongHitReactThreshold)
		{
			return PRGameplayTags::Event_Ability_PlayerHitReact_Weak;
		}

		return FGameplayTag();
	}

	void SendHitReactEvent(UAbilitySystemComponent* TargetASC, const FGameplayEffectModCallbackData& Data,
		float OldPoiseDamage, float NewPoiseDamage, float IncomingPoiseDamage,
		float StrongHitReactThreshold, float DownHitReactThreshold)
	{
		if (!IsValid(TargetASC))
		{
			return;
		}

		AActor* AvatarActor = TargetASC->GetAvatarActor();
		if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
		{
			return;
		}

		const FGameplayTag EventTag = ResolveHitReactEventTag(
			OldPoiseDamage,
			NewPoiseDamage,
			IncomingPoiseDamage,
			StrongHitReactThreshold,
			DownHitReactThreshold);
		if (!EventTag.IsValid())
		{
			return;
		}

		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Instigator = Data.EffectSpec.GetContext().GetOriginalInstigator();
		Payload.Target = AvatarActor;
		Payload.ContextHandle = Data.EffectSpec.GetContext();
		Payload.EventMagnitude = IncomingPoiseDamage;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(AvatarActor, EventTag, Payload);
	}
}

// =====  UAttributeSet Interface =====
void UPRAttributeSet_Player::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetRecoverableHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, ResolveRecoverableHealthMax(GetOwningAbilitySystemComponent()));
	}
	else if (Attribute == GetAccumulatedPoiseDamageAttribute())
	{
		const float ClampMin = FMath::Max(GetPoiseDamageMin(), 0.0f);
		const float ClampMax = ResolvePoiseDamageMax(ClampMin, GetPoiseDamageMax());
		NewValue = FMath::Clamp(NewValue, ClampMin, ClampMax);
	}
	else if (Attribute == GetMaxStaminaAttribute()
		|| Attribute == GetStaminaRegenRateAttribute()
		|| Attribute == GetRecoverableHealthAttribute()
		|| Attribute == GetPoiseDamageMinAttribute()
		|| Attribute == GetPoiseWeakHitReactThresholdAttribute()
		|| Attribute == GetPoiseStrongHitReactThresholdAttribute()
		|| Attribute == GetPoiseDamageMaxAttribute()
		|| Attribute == GetCriticalHitChanceAttribute()
		|| Attribute == GetCriticalDamageMultiplierAttribute()
		|| Attribute == GetPlayerAttackPowerAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UPRAttributeSet_Player::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}
	else if (Data.EvaluatedData.Attribute == GetRecoverableHealthAttribute())
	{
		SetRecoverableHealth(FMath::Clamp(
			GetRecoverableHealth(),
			0.0f,
			ResolveRecoverableHealthMax(GetOwningAbilitySystemComponent())));
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingRecoverableDamageAttribute())
	{
		const float LocalRecoverableDamage = GetIncomingRecoverableDamage();
		SetIncomingRecoverableDamage(0.0f);
		if (LocalRecoverableDamage <= 0.0f)
		{
			return;
		}

		const float RecoverableHealthMax = ResolveRecoverableHealthMax(GetOwningAbilitySystemComponent());
		const float NewRecoverableHealth = FMath::Clamp(GetRecoverableHealth() + LocalRecoverableDamage, 0.0f, RecoverableHealthMax);
		SetRecoverableHealth(NewRecoverableHealth);
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingRecoverableHealAttribute())
	{
		const float LocalRecoverableHeal = GetIncomingRecoverableHeal();
		SetIncomingRecoverableHeal(0.0f);
		if (LocalRecoverableHeal <= 0.0f)
		{
			return;
		}

		UAbilitySystemComponent* AbilitySystemComponent = GetOwningAbilitySystemComponent();
		const UPRAttributeSet_Common* CommonSet = IsValid(AbilitySystemComponent)
			? AbilitySystemComponent->GetSet<UPRAttributeSet_Common>()
			: nullptr;
		if (!IsValid(CommonSet))
		{
			return;
		}

		UPRAttributeSet_Common* MutableCommonSet = const_cast<UPRAttributeSet_Common*>(CommonSet);
		const float HealAmount = FMath::Min(LocalRecoverableHeal, GetRecoverableHealth());
		const float NewHealth = FMath::Clamp(CommonSet->GetHealth() + HealAmount, 0.0f, CommonSet->GetMaxHealth());
		const float AppliedHeal = FMath::Max(NewHealth - CommonSet->GetHealth(), 0.0f);
		MutableCommonSet->SetHealth(NewHealth);
		SetRecoverableHealth(FMath::Clamp(GetRecoverableHealth() - AppliedHeal, 0.0f, ResolveRecoverableHealthMax(AbilitySystemComponent)));
	}
	else if (Data.EvaluatedData.Attribute == GetAccumulatedPoiseDamageAttribute())
	{
		const float ClampMin = FMath::Max(GetPoiseDamageMin(), 0.0f);
		const float ClampMax = ResolvePoiseDamageMax(ClampMin, GetPoiseDamageMax());
		SetAccumulatedPoiseDamage(FMath::Clamp(GetAccumulatedPoiseDamage(), ClampMin, ClampMax));
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingPoiseDamageAttribute())
	{
		const float LocalPoiseDamage = GetIncomingPoiseDamage();
		SetIncomingPoiseDamage(0.0f);
		if (LocalPoiseDamage <= 0.0f)
		{
			return;
		}

		const float ClampMin = FMath::Max(GetPoiseDamageMin(), 0.0f);
		const float ClampMax = ResolvePoiseDamageMax(ClampMin, GetPoiseDamageMax());
		const float StrongHitReactThreshold = ResolvePoiseThreshold(GetPoiseStrongHitReactThreshold(), ClampMin, ClampMax);
		const float OldPoiseDamage = GetAccumulatedPoiseDamage();
		const float NewPoiseDamage = FMath::Clamp(OldPoiseDamage + LocalPoiseDamage, ClampMin, ClampMax);
		SetAccumulatedPoiseDamage(NewPoiseDamage);
		SendHitReactEvent(
			GetOwningAbilitySystemComponent(),
			Data,
			OldPoiseDamage,
			NewPoiseDamage,
			LocalPoiseDamage,
			StrongHitReactThreshold,
			ClampMax);
		if (NewPoiseDamage >= ClampMax)
		{
			SetAccumulatedPoiseDamage(0.0f);
		}
	}
}

void UPRAttributeSet_Player::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, StaminaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, RecoverableHealth, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, AccumulatedPoiseDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, PoiseDamageMin, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, PoiseWeakHitReactThreshold, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, PoiseStrongHitReactThreshold, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, PoiseDamageMax, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, CriticalHitChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, CriticalDamageMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPRAttributeSet_Player, PlayerAttackPower, COND_None, REPNOTIFY_Always);

}


void UPRAttributeSet_Player::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, Stamina, OldValue);
}

void UPRAttributeSet_Player::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, MaxStamina, OldValue);
}

void UPRAttributeSet_Player::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, StaminaRegenRate, OldValue);
}

void UPRAttributeSet_Player::OnRep_RecoverableHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, RecoverableHealth, OldValue);
}

void UPRAttributeSet_Player::OnRep_AccumulatedPoiseDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, AccumulatedPoiseDamage, OldValue);
}

void UPRAttributeSet_Player::OnRep_PoiseDamageMin(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, PoiseDamageMin, OldValue);
}

void UPRAttributeSet_Player::OnRep_PoiseWeakHitReactThreshold(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, PoiseWeakHitReactThreshold, OldValue);
}

void UPRAttributeSet_Player::OnRep_PoiseStrongHitReactThreshold(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, PoiseStrongHitReactThreshold, OldValue);
}

void UPRAttributeSet_Player::OnRep_PoiseDamageMax(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, PoiseDamageMax, OldValue);
}

void UPRAttributeSet_Player::OnRep_CriticalHitChance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, CriticalHitChance, OldValue);
}

void UPRAttributeSet_Player::OnRep_CriticalDamageMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, CriticalDamageMultiplier, OldValue);
}

void UPRAttributeSet_Player::OnRep_PlayerAttackPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPRAttributeSet_Player, PlayerAttackPower, OldValue);
}
