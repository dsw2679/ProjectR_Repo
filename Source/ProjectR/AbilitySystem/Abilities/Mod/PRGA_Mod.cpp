// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Mod 게이지/스택 관리 및 투사체 발사형 Mod 기반 구현)
// Author: 이건주 (GE 기반 무기 자원 갱신 및 Mod 공용 로직 구현)
#include "PRGA_Mod.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/GameStateBase.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/AbilitySystem/Effects/PRGE_ModCost_GaugeDuration.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
#include "ProjectR/ItemSystem/Actors/PRWeaponActor.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponModDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"

UPRGA_Mod::UPRGA_Mod()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	// ========= Mod Base  =============
	InputTag = PRGameplayTags::Input_Ability_Mod;

}

/*~ UGameplayAbility Interface ~*/

void UPRGA_Mod::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 활성 무기 캐싱
	if (APRPlayerCharacter* PlayerCharacter = GetPRCharacter<APRPlayerCharacter>())
	{
		if (UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager())
		{
			CachedWeaponManager = WeaponManager;
			CurrentWeapon = WeaponManager->GetActiveWeaponActor();
		}
	}
	
	// 모드 UI 하이라이트용 이벤트 전송
	if (IsLocallyControlled())
	{
		if (UPREventManagerSubsystem* EventManager = GetWorld()->GetSubsystem<UPREventManagerSubsystem>())
		{
			FPRModActivationPayload Payload;
			Payload.bActivated = true;
			Payload.bUsesModDuration = ModCostPolicy == EPRModCostPolicy::GaugeDuration;
			Payload.ModDurationSeconds = Payload.bUsesModDuration ? ModDuration : 0.0f;
			if (UPRWeaponDataAsset* WeaponData = GetCurrentWeaponData())
			{
				Payload.SlotType = WeaponData->SlotType;	
			}
		
			EventManager->BroadcastTyped(PRGameplayTags::Event_Player_ModActivation, Payload);
		}
	}
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPRGA_Mod::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsLocallyControlled())
	{
		// 모드 UI 하이라이트 끄기용 이벤트 전송
		if (UPREventManagerSubsystem* EventManager = GetWorld()->GetSubsystem<UPREventManagerSubsystem>())
		{
			FPRModActivationPayload Payload;
			Payload.bActivated = false;
			Payload.bWasCancelled = bWasCancelled;
			Payload.bUsesModDuration = ModCostPolicy == EPRModCostPolicy::GaugeDuration;
			if (UPRWeaponDataAsset* WeaponData = GetCurrentWeaponData())
			{
				Payload.SlotType = WeaponData->SlotType;	
			}
		
			EventManager->BroadcastTyped(PRGameplayTags::Event_Player_ModActivation, Payload);
		}
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_Mod::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	// 실제 인스턴스 정리
	if (UPRGA_Mod* InstancedAbility = Cast<UPRGA_Mod>(Spec.GetPrimaryInstance()))
	{
		InstancedAbility->CleanupRuntimeOnAbilityRemoved(ActorInfo, Spec);
	}
	else
	{
		CleanupRuntimeOnAbilityRemoved(ActorInfo, Spec);
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}

// ========= Mod Base  =============

bool UPRGA_Mod::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	bool bHasCost = true;
	bool bIsBlocked = false;
	if (ModCostPolicy == EPRModCostPolicy::Stack)
	{
		bHasCost = HasModStackCost(ActorInfo);
	}
	else if (ModCostPolicy == EPRModCostPolicy::GaugeDuration)
	{
		bIsBlocked = HasActiveModGaugeLock(ActorInfo);
		bHasCost = !bIsBlocked && ModDuration > 0.0f && HasFullModGaugeCost(ActorInfo);
	}

	if (bIsBlocked && OptionalRelevantTags)
	{
		OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Blocked);
	}
	else if (!bHasCost && OptionalRelevantTags)
	{
		OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Cost);
	}

	return bHasCost;
}

FActiveGameplayEffectHandle UPRGA_Mod::ApplyModCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
						  const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UPRGA_Mod* AbilityInstance = GetAbilityInstance<UPRGA_Mod>(Handle, ActorInfo);
	if (!AbilityInstance)
	{
		// InstancingPolicy가 Instance를 생성하지 않음
		return FActiveGameplayEffectHandle();
	}
	
	// 비용 핸들 초기화
	AbilityInstance->LastAppliedModCostHandle.Invalidate();

	if (ModCostPolicy == EPRModCostPolicy::Stack)
	{
		AbilityInstance->LastAppliedModCostHandle = ApplyModStackCost(Handle, ActorInfo, ActivationInfo);
	}
	else if (ModCostPolicy == EPRModCostPolicy::GaugeDuration)
	{
		AbilityInstance->LastAppliedModCostHandle = ApplyModGaugeDurationCost(Handle, ActorInfo, ActivationInfo);
	}

	if (AbilityInstance->LastAppliedModCostHandle.IsValid())
	{
		// 비용 핸들 캐싱
		AbilityInstance->ActiveModCostHandles.AddUnique(AbilityInstance->LastAppliedModCostHandle);
	}

	return AbilityInstance->LastAppliedModCostHandle;
}

void UPRGA_Mod::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                          const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
	ApplyModCost(Handle, ActorInfo, ActivationInfo);
}

UPRWeaponModDataAsset* UPRGA_Mod::GetCurrentModData() const
{
	if (UPRItemInstance_Weapon* Item = Cast<UPRItemInstance_Weapon>(GetCurrentSourceObject()))
	{
		return Item->GetModData();
	}
	return nullptr;
}

UPRWeaponManagerComponent* UPRGA_Mod::GetWeaponManager()
{
	if (!CachedWeaponManager.IsValid())
	{
		if (APRPlayerCharacter* PlayerCharacter = GetPRCharacter<APRPlayerCharacter>())
		{
			if (UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager())
			{
				CachedWeaponManager = WeaponManager;
				CurrentWeapon = WeaponManager->GetActiveWeaponActor();
			}
		}
	}
	
	return CachedWeaponManager.Get();
}

void UPRGA_Mod::SetModCostPolicy(EPRModCostPolicy InModCostType)
{
	ModCostPolicy = InModCostType;
}

FActiveGameplayEffectHandle UPRGA_Mod::GetLastAppliedModCostHandle() const
{
	return LastAppliedModCostHandle;
}

FGameplayAttribute UPRGA_Mod::GetCurrentModGaugeAttribute(const FGameplayAbilityActorInfo* ActorInfo) const
{
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;
	UAbilitySystemComponent* ASC = nullptr;
	UPRItemInstance_Weapon* WeaponInstance = nullptr;
	if (!TryGetCurrentModCostContext(ActorInfo, SlotType, ASC, WeaponInstance))
	{
		return FGameplayAttribute();
	}

	return GetModGaugeAttribute(SlotType);
}

void UPRGA_Mod::CleanupRuntimeOnAbilityRemoved(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	UAbilitySystemComponent* ASC = ActorInfo != nullptr ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!IsValid(ASC) && !HasAnyFlags(RF_ClassDefaultObject))
	{
		ASC = GetAbilitySystemComponentFromActorInfo();
	}

	if (!IsValid(ASC))
	{
		ActiveModCostHandles.Reset();
		LastAppliedModCostHandle.Invalidate();
		return;
	}

	for (const FActiveGameplayEffectHandle& Handle : ActiveModCostHandles)
	{
		if (Handle.IsValid())
		{
			// 비용 GE 제거
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	ActiveModCostHandles.Reset();
	LastAppliedModCostHandle.Invalidate();
}

bool UPRGA_Mod::TryGetCurrentModCostContext(const FGameplayAbilityActorInfo* ActorInfo, EPRWeaponSlotType& OutSlotType,
	UAbilitySystemComponent*& OutASC, UPRItemInstance_Weapon*& OutWeaponInstance) const
{
	OutSlotType = EPRWeaponSlotType::None;
	OutASC = nullptr;
	OutWeaponInstance = nullptr;

	if (!ActorInfo)
	{
		return false;
	}

	OutASC = ActorInfo->AbilitySystemComponent.Get();
	if (!IsValid(OutASC))
	{
		return false;
	}

	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get());
	if (!IsValid(PlayerCharacter))
	{
		return false;
	}

	UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager();
	if (!IsValid(WeaponManager))
	{
		return false;
	}

	UPRItemInstance_Weapon* SourceWeaponInstance = Cast<UPRItemInstance_Weapon>(GetCurrentSourceObject());
	if (!IsValid(SourceWeaponInstance) || !IsValid(SourceWeaponInstance->GetWeaponData()))
	{
		return false;
	}

	OutSlotType = SourceWeaponInstance->GetWeaponData()->SlotType;
	if (OutSlotType == EPRWeaponSlotType::None)
	{
		return false;
	}

	if (WeaponManager->GetCurrentWeaponSlot() != OutSlotType
		|| WeaponManager->GetWeaponInstanceBySlotType(OutSlotType) != SourceWeaponInstance)
	{
		return false;
	}

	OutWeaponInstance = SourceWeaponInstance;
	return true;
}

bool UPRGA_Mod::HasModStackCost(const FGameplayAbilityActorInfo* ActorInfo) const
{
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;
	UAbilitySystemComponent* ASC = nullptr;
	UPRItemInstance_Weapon* WeaponInstance = nullptr;
	if (!TryGetCurrentModCostContext(ActorInfo, SlotType, ASC, WeaponInstance))
	{
		return false;
	}

	const FGameplayAttribute StackAttribute = GetModStackAttribute(SlotType);
	if (!StackAttribute.IsValid())
	{
		return false;
	}

	return ASC->GetNumericAttribute(StackAttribute) >= 1.0f;
}

bool UPRGA_Mod::HasFullModGaugeCost(const FGameplayAbilityActorInfo* ActorInfo) const
{
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;
	UAbilitySystemComponent* ASC = nullptr;
	UPRItemInstance_Weapon* WeaponInstance = nullptr;
	if (!TryGetCurrentModCostContext(ActorInfo, SlotType, ASC, WeaponInstance))
	{
		return false;
	}

	const FGameplayAttribute MaxGaugeAttribute = GetMaxModGaugeAttribute(SlotType);
	if (!MaxGaugeAttribute.IsValid())
	{
		return false;
	}

	const FGameplayAttribute GaugeAttribute = GetModGaugeAttribute(SlotType);
	if (!GaugeAttribute.IsValid())
	{
		return false;
	}

	const float MaxGauge = ASC->GetNumericAttribute(MaxGaugeAttribute);
	return MaxGauge > 0.0f && ASC->GetNumericAttribute(GaugeAttribute) >= MaxGauge - KINDA_SMALL_NUMBER;
}

bool UPRGA_Mod::HasActiveModGaugeLock(const FGameplayAbilityActorInfo* ActorInfo) const
{
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;
	UAbilitySystemComponent* ASC = nullptr;
	UPRItemInstance_Weapon* WeaponInstance = nullptr;
	if (!TryGetCurrentModCostContext(ActorInfo, SlotType, ASC, WeaponInstance))
	{
		return false;
	}

	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return ASC->HasMatchingGameplayTag(PRGameplayTags::State_Mod_Primary_GaugeLocked);
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return ASC->HasMatchingGameplayTag(PRGameplayTags::State_Mod_Secondary_GaugeLocked);
	}

	return false;
}

FActiveGameplayEffectHandle UPRGA_Mod::ApplyModStackCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
						  const FGameplayAbilityActivationInfo ActivationInfo) const
{
	TSubclassOf<UGameplayEffect> CostGE;
	
	if (auto WeaponData = GetCurrentWeaponData())
	{
		if (WeaponData->SlotType == EPRWeaponSlotType::Primary)
		{
			CostGE = UPRAssetManager::Get().GetAbilitySystemRegistry()->ModStackCostGE_Primary;
		}
		else if (WeaponData->SlotType == EPRWeaponSlotType::Secondary)
		{
			CostGE = UPRAssetManager::Get().GetAbilitySystemRegistry()->ModStackCostGE_Secondary;
		}
	}
	
	if (!ensureMsgf(IsValid(CostGE),TEXT("AbilitySystemRegistry에 올바른 GE가 할당되지 않음")))
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectSpecHandle GEHandle = MakeOutgoingGameplayEffectSpec(CostGE, 1);
	return ApplyGameplayEffectSpecToOwner(Handle,ActorInfo,ActivationInfo,GEHandle);
}

FActiveGameplayEffectHandle UPRGA_Mod::ApplyModGaugeDurationCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
						  const FGameplayAbilityActivationInfo ActivationInfo) const
{
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;
	UAbilitySystemComponent* ASC = nullptr;
	UPRItemInstance_Weapon* WeaponInstance = nullptr;
	if (!TryGetCurrentModCostContext(ActorInfo, SlotType, ASC, WeaponInstance))
	{
		return FActiveGameplayEffectHandle();
	}

	const FGameplayAttribute MaxGaugeAttribute = GetMaxModGaugeAttribute(SlotType);
	if (!MaxGaugeAttribute.IsValid() || ASC->GetNumericAttribute(MaxGaugeAttribute) <= 0.0f || ModDuration <= 0.0f)
	{
		return FActiveGameplayEffectHandle();
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(Handle,	ActorInfo,ActivationInfo,UPRGE_ModCost_GaugeDuration::StaticClass());
	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_ModDuration, ModDuration);

	if (SlotType == EPRWeaponSlotType::Primary)
	{
		SpecHandle.Data->DynamicGrantedTags.AddTag(PRGameplayTags::State_Mod_Primary_GaugeLocked);
	}
	else if (SlotType == EPRWeaponSlotType::Secondary)
	{
		SpecHandle.Data->DynamicGrantedTags.AddTag(PRGameplayTags::State_Mod_Secondary_GaugeLocked);
	}
	else
	{
		return FActiveGameplayEffectHandle();
	}

	const FActiveGameplayEffectHandle CostHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	if (UWorld* World = GetWorld())
	{
		const AGameStateBase* GameState = World->GetGameState();
		const float ServerWorldTimeSeconds = IsValid(GameState) ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
		WeaponInstance->ModEffectEndServerWorldTimeSeconds = ServerWorldTimeSeconds + ModDuration;
	}

	return CostHandle;
}

FGameplayAttribute UPRGA_Mod::GetModGaugeAttribute(EPRWeaponSlotType SlotType) const
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return UPRAttributeSet_Weapon::GetPrimaryModGaugeAttribute();
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return UPRAttributeSet_Weapon::GetSecondaryModGaugeAttribute();
	}

	return FGameplayAttribute();
}

FGameplayAttribute UPRGA_Mod::GetMaxModGaugeAttribute(EPRWeaponSlotType SlotType) const
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return UPRAttributeSet_Weapon::GetPrimaryMaxModGaugeAttribute();
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return UPRAttributeSet_Weapon::GetSecondaryMaxModGaugeAttribute();
	}

	return FGameplayAttribute();
}

FGameplayAttribute UPRGA_Mod::GetModStackAttribute(EPRWeaponSlotType SlotType) const
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return UPRAttributeSet_Weapon::GetPrimaryModStackAttribute();
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return UPRAttributeSet_Weapon::GetSecondaryModStackAttribute();
	}

	return FGameplayAttribute();
}

FGameplayAttribute UPRGA_Mod::GetMaxModStackAttribute(EPRWeaponSlotType SlotType) const
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return UPRAttributeSet_Weapon::GetPrimaryMaxModStackAttribute();
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return UPRAttributeSet_Weapon::GetSecondaryMaxModStackAttribute();
	}

	return FGameplayAttribute();
}

/*~ 데미지 적용 ~*/

void UPRGA_Mod::ApplyDamage(AActor* TargetActor, float Damage, float GroggyDamage, const FHitResult* HitResult)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(SourceASC))
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeModEffectSpec(Damage, GroggyDamage, HitResult);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 대상 ASC에 GE 적용
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (IsValid(TargetASC))
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}
