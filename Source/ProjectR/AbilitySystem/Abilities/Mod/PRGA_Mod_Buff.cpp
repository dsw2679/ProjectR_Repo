// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (버프 활성화 시 스택/게이지 소모 로직 구현)
// Author: 이건주 (GE 기반 무기 버프 Mod 구현)
#include "PRGA_Mod_Buff.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "GameplayEffect.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/PRGameplayTags.h"

UPRGA_Mod_Buff::UPRGA_Mod_Buff()
{
}

bool UPRGA_Mod_Buff::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (!IsValid(BuffEffect))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
		}
		return false;
	}

	if (HasActiveBuffEffect(ActorInfo))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Blocked);
		}
		return false;
	}

	return true;
}

/*~ UPRGA_Mod_HasDuration Interface ~*/

void UPRGA_Mod_Buff::OnDurationStarted_Implementation()
{
	Super::OnDurationStarted_Implementation();

	// 버프 발동 연출
	PlayActivationMontageIfValid();

	if (HasAuthority(&CurrentActivationInfo))
	{
		// 서버 버프 적용
		ApplyBuffEffect();
	}
}

/*~ UPRGA_Mod Interface ~*/

void UPRGA_Mod_Buff::CleanupRuntimeOnAbilityRemoved(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	// 버프 런타임 정리
	RemoveActiveBuffEffect();
	Super::CleanupRuntimeOnAbilityRemoved(ActorInfo, Spec);
}

/*~ 버프 처리 ~*/

bool UPRGA_Mod_Buff::HasActiveBuffEffect(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!IsValid(ASC) || !IsValid(BuffEffect))
	{
		return false;
	}

	FGameplayEffectQuery Query;
	Query.EffectDefinition = BuffEffect;
	return ASC->GetActiveEffects(Query).Num() > 0;
}

bool UPRGA_Mod_Buff::ApplyBuffEffect()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || !IsValid(BuffEffect) || ModDuration <= 0.0f)
	{
		return false;
	}
	// 서버 권한 확인

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(BuffEffect, BuffEffectLevel);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_ModDuration, ModDuration);
	SpecHandle.Data->SetDuration(ModDuration, true);
	
	ActiveBuffHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	if (!ActiveBuffHandle.IsValid())
	{
		return false;
	}

	BindBuffRemovalEvent();
	BindSurvivalTagEvents();
	return true;
}

void UPRGA_Mod_Buff::PlayActivationMontageIfValid()
{
	if (!IsValid(ActivationMontage))
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return;
	}

	ASC->PlayMontage(
		this,
		CurrentActivationInfo,
		ActivationMontage,
		FMath::Max(ActivationMontagePlayRate, UE_SMALL_NUMBER));
}

void UPRGA_Mod_Buff::BindSurvivalTagEvents()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return;
	}

	if (!DeadTagEventHandle.IsValid())
	{
		DeadTagEventHandle = ASC->RegisterGameplayTagEvent(PRGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UPRGA_Mod_Buff::HandleSurvivalTagChanged);
	}

	if (!DownTagEventHandle.IsValid())
	{
		DownTagEventHandle = ASC->RegisterGameplayTagEvent(PRGameplayTags::State_Down, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UPRGA_Mod_Buff::HandleSurvivalTagChanged);
	}
}

void UPRGA_Mod_Buff::UnbindSurvivalTagEvents()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		DeadTagEventHandle.Reset();
		DownTagEventHandle.Reset();
		return;
	}

	if (DeadTagEventHandle.IsValid())
	{
		ASC->UnregisterGameplayTagEvent(DeadTagEventHandle, PRGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved);
		DeadTagEventHandle.Reset();
	}

	if (DownTagEventHandle.IsValid())
	{
		ASC->UnregisterGameplayTagEvent(DownTagEventHandle, PRGameplayTags::State_Down, EGameplayTagEventType::NewOrRemoved);
		DownTagEventHandle.Reset();
	}
}

void UPRGA_Mod_Buff::BindBuffRemovalEvent()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || !ActiveBuffHandle.IsValid() || BuffRemovedDelegateHandle.IsValid())
	{
		return;
	}

	FOnActiveGameplayEffectRemoved_Info* RemovedDelegate = ASC->OnGameplayEffectRemoved_InfoDelegate(ActiveBuffHandle);
	if (RemovedDelegate != nullptr)
	{
		BuffRemovedDelegateHandle = RemovedDelegate->AddUObject(this, &UPRGA_Mod_Buff::HandleBuffEffectRemoved);
	}
}

void UPRGA_Mod_Buff::UnbindBuffRemovalEvent()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || !ActiveBuffHandle.IsValid() || !BuffRemovedDelegateHandle.IsValid())
	{
		BuffRemovedDelegateHandle.Reset();
		return;
	}

	FOnActiveGameplayEffectRemoved_Info* RemovedDelegate = ASC->OnGameplayEffectRemoved_InfoDelegate(ActiveBuffHandle);
	if (RemovedDelegate != nullptr)
	{
		RemovedDelegate->Remove(BuffRemovedDelegateHandle);
	}

	BuffRemovedDelegateHandle.Reset();
}

void UPRGA_Mod_Buff::HandleBuffEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo)
{
	ActiveBuffHandle.Invalidate();
	BuffRemovedDelegateHandle.Reset();
	UnbindSurvivalTagEvents();
}

void UPRGA_Mod_Buff::HandleSurvivalTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		return;
	}

	if (Tag == PRGameplayTags::State_Dead || Tag == PRGameplayTags::State_Down)
	{
		RemoveActiveBuffEffect();
	}
}

void UPRGA_Mod_Buff::RemoveActiveBuffEffect()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (IsValid(ASC) && ActiveBuffHandle.IsValid())
	{
		UnbindBuffRemovalEvent();
		ASC->RemoveActiveGameplayEffect(ActiveBuffHandle);
	}

	ActiveBuffHandle.Invalidate();
	BuffRemovedDelegateHandle.Reset();
	UnbindSurvivalTagEvents();
}
