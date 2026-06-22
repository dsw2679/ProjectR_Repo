// Author: 배유찬 (어빌리티 세트 로드 및 입력 매핑 구조 구현)
// Author: 이건주 (배리어 모드 및 무기 전용 어빌리티 매핑 구조 구현)
#include "PRAbilitySet.h"

#include "ProjectR/AbilitySystem/PRGameplayAbility.h"

bool FPRAbilityEntry::IsValid() const
{
	return ::IsValid(AbilityClass);
}

void FPRAbilityEntry::GiveToAbilitySystem(UAbilitySystemComponent* TargetASC,
	FPRAbilitySetHandles& OutHandles,
	UObject* InSourceObject,
	const FGameplayTagContainer* AdditionalDynamicTags) const
{
	FGameplayAbilitySpec Spec(AbilityClass, Level);
	Spec.GetDynamicSpecSourceTags().AppendTags(DynamicTags);
	if (AdditionalDynamicTags)
	{
		// 슬롯 차단 태그 추가
		Spec.GetDynamicSpecSourceTags().AppendTags(*AdditionalDynamicTags);
	}
	if (::IsValid(InSourceObject))
	{
		Spec.SourceObject = InSourceObject;
	}

	// InputTag가 CDO에 설정되어 있으면 DynamicTags에 자동 주입 (AbilitySet이 누락해도 매칭되도록)
	if (const UPRGameplayAbility* CDO = Cast<UPRGameplayAbility>(AbilityClass->GetDefaultObject()))
	{
		const FGameplayTag& CDOInputTag = CDO->GetInputTag();
		if (CDOInputTag.IsValid() && !Spec.GetDynamicSpecSourceTags().HasTagExact(CDOInputTag))
		{
			Spec.GetDynamicSpecSourceTags().AddTag(CDOInputTag);
		}
	}

	const FGameplayAbilitySpecHandle Handle =  TargetASC->GiveAbility(Spec);
	OutHandles.AbilityHandles.Add(Handle);
}

bool FPREffectEntry::IsValid() const
{
	return ::IsValid(EffectClass);
}

void FPREffectEntry::GiveToAbilitySystem(UAbilitySystemComponent* TargetASC, FPRAbilitySetHandles& OutHandles, const UObject* InSourceObject) const
{
	FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
	if (::IsValid(InSourceObject))
	{
		Context.AddSourceObject(InSourceObject);	
	}
	
	const FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectClass, Level, Context);
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->DynamicGrantedTags.AppendTags(DynamicTags);
		const FActiveGameplayEffectHandle ActiveHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
		OutHandles.EffectHandles.Add(ActiveHandle);
	}
}

UPRAbilitySet* CreateRuntimeAbilitySet(
	UObject* Outer,
	const TArray<FPRAbilityEntry>& InAbilities,
	const TArray<FPREffectEntry>& InEffects)
{
	if (!IsValid(Outer))
	{
		return nullptr;
	}

	UPRAbilitySet* RuntimeAbilitySet = NewObject<UPRAbilitySet>(Outer);
	RuntimeAbilitySet->Abilities = InAbilities;
	RuntimeAbilitySet->Effects = InEffects;

	return RuntimeAbilitySet;
}

UPRAbilitySet::UPRAbilitySet()
{
}
