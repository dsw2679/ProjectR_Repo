// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 상태 연동 기능 구현)
// Author: 배유찬 (어빌리티 시스템 컴포넌트 초기 구축 및 태그 이벤트 바인딩 구현)
#include "PRAbilitySystemComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/PRGameplayTags.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "Data/PRAbilitySet.h"
#include "Data/PRAbilitySystemRegistry.h"

namespace
{
	// Avatar 준비 훅 단일 Spec 호출
	void NotifyAvatarSetForAbilitySpec(FGameplayAbilitySpec& Spec, const FGameplayAbilityActorInfo* ActorInfo)
	{
		if (!IsValid(Spec.Ability) || ActorInfo == nullptr || !ActorInfo->AvatarActor.IsValid())
		{
			return;
		}

		const EGameplayAbilityInstancingPolicy::Type InstancingPolicy = Spec.Ability->GetInstancingPolicy();
		if (InstancingPolicy == EGameplayAbilityInstancingPolicy::Type::InstancedPerActor)
		{
			if (const UPRGameplayAbility* Inst = Cast<UPRGameplayAbility>(Spec.GetPrimaryInstance()))
			{
				// PerActor 인스턴스 훅
				Inst->OnAvatarSet(Spec, ActorInfo);
			}
			return;
		}

		if (InstancingPolicy == EGameplayAbilityInstancingPolicy::Type::NonInstanced)
		{
			if (const UPRGameplayAbility* CDO = Cast<UPRGameplayAbility>(Spec.Ability))
			{
				// NonInstanced CDO 훅
				CDO->OnAvatarSet(Spec, ActorInfo);
			}
			return;
		}

		for (UGameplayAbility* AbilityInstance : Spec.GetAbilityInstances())
		{
			if (const UPRGameplayAbility* Inst = Cast<UPRGameplayAbility>(AbilityInstance))
			{
				// PerExecution 인스턴스 훅
				Inst->OnAvatarSet(Spec, ActorInfo);
			}
		}
	}
}

// =====  UActorComponent Interface =====

void UPRAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UPRAbilitySystemComponent, PROwningTags, COND_SimulatedOnly);
}

void UPRAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();

	// GAS의 어빌리티 종료 멀티캐스트 델리게이트에 바인딩. OnAbilityEnded 브로드캐스트 경로
	// TODO: 어빌리티 발동 / 종료 이벤트 래퍼 델리게이트 발행
	//AbilityEndedCallbacks.AddUObject(this, &UPRAbilitySystemComponent::HandleAbilityEnded);
	//AbilityActivatedCallbacks.AddUObject(this, &UPRAbilitySystemComponent::HandleAbilityActivated);
}

// =====  UAbilitySystemComponent Interface =====

void UPRAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);

	if (const FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get())
	{
		// AvatarActor 보유 상태에서 후부여된 어빌리티 훅
		NotifyAvatarSetForAbilitySpec(AbilitySpec, ActorInfo);
	}

	// ActivationPolicy == OnGiven 이면 부여 즉시 활성화
	const UPRGameplayAbility* CDO = Cast<UPRGameplayAbility>(AbilitySpec.Ability);
	if (IsValid(CDO) && CDO->GetActivationPolicy() == EPRAbilityActivationPolicy::OnGiven)
	{
		TryActivateAbility(AbilitySpec.Handle);
	}

	// 어빌리티 종료 통지 수신을 위해 Spec의 OnAbilityEnded에 바인딩은 개별 인스턴스에서 처리.
	// 여기서는 ASC의 범용 종료 콜백만 등록
	if (IsValid(AbilitySpec.Ability))
	{
		// GAS는 FAbilityEndedData 콜백을 ASC OnAbilityEnded 델리게이트로 제공
	}
}

void UPRAbilitySystemComponent::NotifyAvatarSet()
{
	const FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
	if (ActorInfo == nullptr || !ActorInfo->AvatarActor.IsValid())
	{
		return;
	}

	// AvatarActor 준비 이후 전체 어빌리티 훅 호출
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		NotifyAvatarSetForAbilitySpec(Spec, ActorInfo);
	}
}

void UPRAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	if (!Spec.IsActive() || Spec.Ability == nullptr)
	{
		return;
	}

	// WaitInputPressed Task에 이벤트 전달을 위해 InvokeReplicatedEvent 사용
	const EGameplayAbilityInstancingPolicy::Type InstancingPolicy = Spec.Ability->GetInstancingPolicy();
	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::Type::InstancedPerActor)
	{
		if (UGameplayAbility* Inst = Spec.GetPrimaryInstance())
		{
			const FGameplayAbilityActivationInfo Info = Inst->GetCurrentActivationInfo();
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Info.GetActivationPredictionKey());
		}
	}
	else
	{
		for (UGameplayAbility* Inst : Spec.GetAbilityInstances())
		{
			const FGameplayAbilityActivationInfo Info = Inst->GetCurrentActivationInfo();
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Info.GetActivationPredictionKey());
		}
	}
}

void UPRAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	if (!Spec.IsActive() || Spec.Ability == nullptr)
	{
		return;
	}

	const EGameplayAbilityInstancingPolicy::Type InstancingPolicy = Spec.Ability->GetInstancingPolicy();
	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::Type::InstancedPerActor)
	{
		if (UGameplayAbility* Inst = Spec.GetPrimaryInstance())
		{
			const FGameplayAbilityActivationInfo Info = Inst->GetCurrentActivationInfo();
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Info.GetActivationPredictionKey());
		}
	}
	else
	{
		for (UGameplayAbility* Inst : Spec.GetAbilityInstances())
		{
			const FGameplayAbilityActivationInfo Info = Inst->GetCurrentActivationInfo();
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Info.GetActivationPredictionKey());
		}
	}
}

void UPRAbilitySystemComponent::OnTagUpdated(const FGameplayTag& Tag, bool bTagExists)
{
	Super::OnTagUpdated(Tag, bTagExists);
	
	if (GetOwner()&& GetOwner()->GetNetOwner())
	{
		if (bTagExists)
		{
			PROwningTags.AddTag(Tag);
		}
		else
		{
			PROwningTags.RemoveTag(Tag);
		}
		if (OnGameplayTagUpdated.IsBound())
		{
			OnGameplayTagUpdated.Broadcast(Tag, bTagExists);
		}
	}
}

void UPRAbilitySystemComponent::MulticastTagUpdated_Implementation(const FGameplayTag Tag, bool TagExists)
{
	if (OnGameplayTagUpdated.IsBound())
	{
		OnGameplayTagUpdated.Broadcast(Tag, TagExists);
	}
}

// =====  AbilitySet 부여/해제 =====

void UPRAbilitySystemComponent::GiveAbilitySet(const UPRAbilitySet* AbilitySet, FPRAbilitySetHandles& OutHandles, UObject* InSourceObject)
{
	if (!IsValid(AbilitySet) || !IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const FPRAbilityEntry& Entry : AbilitySet->Abilities)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		Entry.GiveToAbilitySystem(this, OutHandles, InSourceObject);
	}

	for (const FPREffectEntry& Entry : AbilitySet->Effects)
	{
		if (!Entry.IsValid())
		{
			continue;
		}
		
		Entry.GiveToAbilitySystem(this, OutHandles, InSourceObject);
	}
}

void UPRAbilitySystemComponent::ClearAbilitySetByHandles(FPRAbilitySetHandles& Handles)
{
	if (!IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const FActiveGameplayEffectHandle& Handle : Handles.EffectHandles)
	{
		if (Handle.IsValid())
		{
			RemoveActiveGameplayEffect(Handle);
		}
	}

	for (const FGameplayAbilitySpecHandle& Handle : Handles.AbilityHandles)
	{
		if (Handle.IsValid())
		{
			ClearAbility(Handle);
		}
	}

	Handles.Reset();
}

void UPRAbilitySystemComponent::ResetSystem()
{
	if (!IsOwnerActorAuthoritative())
	{
		return;
	}

	// 리스폰 이전 Pawn과 입력에서 이어진 활성 어빌리티 상태 정리
	CancelAllAbilities();
	ClearAbilityInput();

	// 리스폰 후 ApplySaveData가 장비, 무기, 특성 효과를 다시 부여하므로 현재 활성 GameplayEffect 일괄 제거
	RemoveActiveEffects(FGameplayEffectQuery());
}

bool UPRAbilitySystemComponent::InitializeAttributesFromRegistry(const UPRAbilitySystemRegistry* Registry,
                                                                   EPRCharacterRole Role, FName RowName)
{
	if (!IsValid(Registry) || !IsOwnerActorAuthoritative())
	{
		return false;
	}

	UDataTable* DT = Registry->GetStatTableSynchronous(Role);
	if (!IsValid(DT))
	{
		return false;
	}

	const UScriptStruct* RowStruct = DT->GetRowStruct();
	if (RowStruct == nullptr)
	{
		return false;
	}

	uint8* RowData = DT->FindRowUnchecked(RowName);
	if (RowData == nullptr)
	{
		return false;
	}

	// Row의 모든 FFloatProperty를 순회. 프로퍼티명을 Registry로 조회하여 Attribute 주입
	for (TFieldIterator<FFloatProperty> It(RowStruct); It; ++It)
	{
		const FFloatProperty* Prop = *It;
		const FName PropName = Prop->GetFName();
		const float Value = Prop->GetPropertyValue_InContainer(RowData);

		const FGameplayAttribute Attr = Registry->FindAttribute(PropName);
		if (!Attr.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Registry missing mapping for property '%s'"), *PropName.ToString());
			continue;
		}

		SetNumericAttributeBase(Attr, Value);
	}
	
	// 초기화 GE 적용
	if (IsValid(Registry->InitializeGE))
	{
		FGameplayEffectContextHandle EffectContextHandle = MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(Registry->InitializeGE, 1.0f, EffectContextHandle);
		ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	return true;
}

FPRAttributeBaseSnapshot UPRAbilitySystemComponent::MakeAttributeBaseSnapshot(const TArray<FGameplayAttribute>& Attributes) const
{
	FPRAttributeBaseSnapshot Snapshot;
	Snapshot.Entries.Reserve(Attributes.Num());

	for (const FGameplayAttribute& Attribute : Attributes)
	{
		if (!Attribute.IsValid())
		{
			continue;
		}

		FPRAttributeBaseEntry Entry;
		Entry.Attribute = Attribute;
		Entry.BaseValue = GetNumericAttributeBase(Attribute);
		Snapshot.Entries.Add(Entry);
	}

	return Snapshot;
}

void UPRAbilitySystemComponent::ApplyAttributeBaseSnapshot(const FPRAttributeBaseSnapshot& InSnapshot)
{
	if (!IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const FPRAttributeBaseEntry& Entry : InSnapshot.Entries)
	{
		if (!Entry.Attribute.IsValid())
		{
			continue;
		}

		// Attribute Base 값 복원
		SetNumericAttributeBase(Entry.Attribute, Entry.BaseValue);
	}
}

void UPRAbilitySystemComponent::AbilityInputPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (!Spec.Ability)
		{
			continue;
		}
		
		bool bHasInputTag = Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag);
		if (!bHasInputTag)
		{
			if (UPRGameplayAbility* PRGA = Cast<UPRGameplayAbility>(Spec.Ability))
			{
				bHasInputTag = PRGA->GetInputTag().MatchesTagExact(InputTag);
			}
		}
		
		if (bHasInputTag)
		{
			UE_LOG(LogTemp, Warning, TEXT("AbilityInputPressed %s"), *Spec.Ability->GetName());
			InputPressedSpecHandles.AddUnique(Spec.Handle);
			InputHeldSpecHandles.AddUnique(Spec.Handle);
		}
	}
}

void UPRAbilitySystemComponent::AbilityInputReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (!Spec.Ability)
		{
			continue;
		}
		
		bool bHasInputTag = Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag);
		if (!bHasInputTag)
		{
			if (UPRGameplayAbility* PRGA = Cast<UPRGameplayAbility>(Spec.Ability))
			{
				bHasInputTag = PRGA->GetInputTag().MatchesTagExact(InputTag);
			}
		}
		
		if (bHasInputTag)
		{
			InputReleasedSpecHandles.AddUnique(Spec.Handle);
			InputHeldSpecHandles.Remove(Spec.Handle);
		}
	}
}

void UPRAbilitySystemComponent::ProcessAbilityInput(float /*DeltaTime*/, bool /*bGamePaused*/)
{
	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

	// WhileInputHeld: 입력 유지 중, 비활성인 Spec만 후보
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (Spec->Ability && !Spec->IsActive())
			{
				const UPRGameplayAbility* CDO = Cast<UPRGameplayAbility>(Spec->Ability);
				if (IsValid(CDO) && CDO->GetActivationPolicy() == EPRAbilityActivationPolicy::WhileInputHeld)
				{
					AbilitiesToActivate.AddUnique(SpecHandle);
				}
			}
		}
	}

	// OnInputTriggered: 이번 프레임 Pressed
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (Spec->Ability)
			{
				Spec->InputPressed = true;

				if (Spec->IsActive())
				{
					AbilitySpecInputPressed(*Spec);
				}
				else
				{
					const UPRGameplayAbility* CDO = Cast<UPRGameplayAbility>(Spec->Ability);
					if (IsValid(CDO) && CDO->GetActivationPolicy() == EPRAbilityActivationPolicy::OnInputTriggered)
					{
						AbilitiesToActivate.AddUnique(SpecHandle);
					}
				}
			}
		}
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : AbilitiesToActivate)
	{
		// TryActivateAbility
		if (!TryActivateAbility(SpecHandle))
		{
			// 실패시 이벤트 훅 호출
			if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle))
			{
				const UPRGameplayAbility* CDO = Cast<UPRGameplayAbility>(Spec->Ability);
				CDO->OnFailActivateAbility(this,Spec);
			}
		}
	}

	// Released
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (Spec->Ability)
			{
				Spec->InputPressed = false;
				if (Spec->IsActive())
				{
					AbilitySpecInputReleased(*Spec);
				}
			}
		}
	}

	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UPRAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

// =====  AI / 서버 호출 경로 =====

bool UPRAbilitySystemComponent::TryActivateAbilityOnServer(const FGameplayTag& AbilityTag,
                                                             FGameplayAbilitySpecHandle& OutHandle)
{
	if (!AbilityTag.IsValid() || !IsOwnerActorAuthoritative())
	{
		return false;
	}

	FGameplayTagContainer Query;
	Query.AddTag(AbilityTag);

	TArray<FGameplayAbilitySpec*> MatchingSpecs;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(Query, MatchingSpecs);

	for (FGameplayAbilitySpec* Spec : MatchingSpecs)
	{
		if (Spec && TryActivateAbility(Spec->Handle))
		{
			OutHandle = Spec->Handle;
			return true;
		}
	}

	return false;
}

bool UPRAbilitySystemComponent::CanActivateAbilityByTag(const FGameplayTag& AbilityTag,
                                                         FGameplayTagContainer& OutFailureTags) const
{
	if (!AbilityTag.IsValid())
	{
		OutFailureTags.AddTag(PRGameplayTags::Fail_Invalid);
		return false;
	}

	FGameplayTagContainer Query;
	Query.AddTag(AbilityTag);

	TArray<FGameplayAbilitySpec*> MatchingSpecs;
	const_cast<UPRAbilitySystemComponent*>(this)->GetActivatableGameplayAbilitySpecsByAllMatchingTags(Query, MatchingSpecs);

	for (FGameplayAbilitySpec* Spec : MatchingSpecs)
	{
		if (Spec == nullptr || Spec->Ability == nullptr)
		{
			continue;
		}
		FGameplayTagContainer FailureTags;
		if (Spec->Ability->CanActivateAbility(Spec->Handle, AbilityActorInfo.Get(), nullptr, nullptr, &FailureTags))
		{
			return true;
		}
		OutFailureTags.AppendTags(FailureTags);
	}

	if (OutFailureTags.IsEmpty())
	{
		OutFailureTags.AddTag(PRGameplayTags::Fail_Invalid);
	}
	return false;
}

bool UPRAbilitySystemComponent::TryConsumeClientReplicatedTargetData(FGameplayAbilitySpecHandle AbilityHandle,
	FPredictionKey AbilityOriginalPredictionKey)
{
	TSharedPtr<FAbilityReplicatedDataCache> CachedData = AbilityTargetDataMap.Find(FGameplayAbilitySpecHandleAndPredictionKey(AbilityHandle, AbilityOriginalPredictionKey));
	if (CachedData.IsValid())
	{
		const bool bConsumed = CachedData->TargetData.Num() > 0;
		CachedData->TargetData.Clear();
		CachedData->bTargetConfirmed = false;
		CachedData->bTargetCancelled = false;
		return bConsumed;
	}
	return false;
}

void UPRAbilitySystemComponent::MulticastTriggerEvent_Implementation(FGameplayTag EventTag)
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(),EventTag,FGameplayEventData());
}

// =====  내부 =====

void UPRAbilitySystemComponent::HandleAbilityEnded(const FAbilityEndedData& EndedData)
{
	if (EndedData.AbilityThatEnded == nullptr)
	{
		return;
	}
	const FGameplayTagContainer& Tags = EndedData.AbilityThatEnded->AbilityTags;
	const FGameplayTag PrimaryTag = Tags.IsEmpty() ? FGameplayTag() : Tags.First();
	OnAbilityEnded.Broadcast(PrimaryTag, EndedData.bWasCancelled);
}

void UPRAbilitySystemComponent::HandleAbilityActivated(UGameplayAbility* ActivatedAbility)
{
	
}

void UPRAbilitySystemComponent::OnRep_OwningTags(const FGameplayTagContainer& OldTags)
{
	if (!OnGameplayTagUpdated.IsBound())
	{
		return;
	}
	
	// 사라진 태그 조회
	for (auto& OldTag : OldTags)
	{
		if (!PROwningTags.HasTagExact(OldTag))
		{
			OnGameplayTagUpdated.Broadcast(OldTag,false);
		}
	}
	
	// 추가된 태그 조회
	for (auto& NewTag : PROwningTags)
	{
		if (!OldTags.HasTagExact(NewTag))
		{
			OnGameplayTagUpdated.Broadcast(NewTag,true);
		}
	}
}
