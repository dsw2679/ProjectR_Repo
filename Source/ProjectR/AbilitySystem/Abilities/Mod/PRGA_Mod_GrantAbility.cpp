// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Mod 게이지 충전 시 어빌리티 활성화 제어 구현)
// Author: 이건주 (드론/배리어 관련 액티브 어빌리티 부여 로직 구현)
#include "PRGA_Mod_GrantAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"

namespace
{
	// Mod 사격 차단 태그 구성
	FGameplayTagContainer BuildModFireBlockTags(const UPRItemInstance_Weapon* WeaponInstance)
	{
		FGameplayTagContainer BlockTags;
		if (!IsValid(WeaponInstance) || !IsValid(WeaponInstance->GetWeaponData()))
		{
			return BlockTags;
		}

		if (WeaponInstance->GetWeaponData()->SlotType == EPRWeaponSlotType::Primary)
		{
			BlockTags.AddTag(PRGameplayTags::State_CurrentWeaponSlot_Secondary);
			BlockTags.AddTag(PRGameplayTags::State_CurrentWeaponSlot_Primary_Base);
		}
		else if (WeaponInstance->GetWeaponData()->SlotType == EPRWeaponSlotType::Secondary)
		{
			BlockTags.AddTag(PRGameplayTags::State_CurrentWeaponSlot_Primary);
			BlockTags.AddTag(PRGameplayTags::State_CurrentWeaponSlot_Secondary_Base);
		}

		return BlockTags;
	}
}

UPRGA_Mod_GrantAbility::UPRGA_Mod_GrantAbility()
{
	
}

void UPRGA_Mod_GrantAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayAbility::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

void UPRGA_Mod_GrantAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!IsValid(AbilityToGrant))
	{
		K2_EndAbility();
		return;
	}

	if (UPRWeaponDataAsset* WeaponData = GetCurrentWeaponData())
	{
		// 발동 슬롯 기록
		SourceWeaponSlot = WeaponData->SlotType;
		BindWeaponEquipmentChanged();
	}
	
	if (HasAuthority(&ActivationInfo))
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			FGameplayAbilitySpec AbilitySpecToGrant (AbilityToGrant,1);
			AbilitySpecToGrant.SourceObject = this;
			if (UPRItemInstance_Weapon* SourceWeapon = Cast<UPRItemInstance_Weapon>(GetCurrentSourceObject()))
			{
				// Mod 사격 Spec 차단 태그
				AbilitySpecToGrant.GetDynamicSpecSourceTags().AppendTags(BuildModFireBlockTags(SourceWeapon));
				SourceWeapon->FireModeState = EPRWeaponFireModeState::ModFire;
				if (UPRWeaponManagerComponent* WeaponManager = GetWeaponManager())
				{
					// 현재 슬롯 사격 모드 태그 갱신
					WeaponManager->RefreshCurrentWeaponFireModeTags();
				}
			}
			GrantedSpecHandle = ASC->GiveAbility(AbilitySpecToGrant);
		}
	
		if (UPRWeaponDataAsset* WeaponData = GetCurrentWeaponData())
		{
			if (ModCostPolicy == EPRModCostPolicy::Stack)
			{
				WaitModStackExhausted(WeaponData->SlotType);	
			}
			else if (ModCostPolicy == EPRModCostPolicy::GaugeDuration)
			{
				// TODO: 게이지 소모 감지
			}
		}
	}
	
	if (IsLocallyControlled())
	{
		// 클라는 서버의 종료 신호를 대기 or Input 재입력 대기
		UAbilityTask_WaitInputPress* WaitInputPress = UAbilityTask_WaitInputPress::WaitInputPress(this,false);
		WaitInputPress->OnPress.AddDynamic(this,&ThisClass::OnInputPressed);
		WaitInputPress->ReadyForActivation();
	}
	
}

void UPRGA_Mod_GrantAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	UnbindWeaponEquipmentChanged();
	CleanupGrantedAbilityRuntime(ActorInfo);
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_Mod_GrantAbility::CleanupRuntimeOnAbilityRemoved(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	UnbindWeaponEquipmentChanged();

	// 부여 어빌리티 정리
	CleanupGrantedAbilityRuntime(ActorInfo);
	Super::CleanupRuntimeOnAbilityRemoved(ActorInfo, Spec);
}

void UPRGA_Mod_GrantAbility::WaitModStackExhausted(EPRWeaponSlotType WeaponSlotType)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		UE_LOG(LogTemp,Warning,TEXT("UPRGA_Mod_GrantAbility::WaitModStackExhausted: Invalid ASC. ObjectName: %s"),*GetName());
	}

	switch (WeaponSlotType)
	{
	case EPRWeaponSlotType::Primary:
		CostAttribute = UPRAttributeSet_Weapon::GetPrimaryModStackAttribute();
		break;
	case EPRWeaponSlotType::Secondary:
		CostAttribute = UPRAttributeSet_Weapon::GetSecondaryModStackAttribute();
		break;
	default:
		break;
	}
	
	if (CostAttribute.IsValid())
	{
		CostExhaustedHandle = ASC->GetGameplayAttributeValueChangeDelegate(CostAttribute).AddWeakLambda(
			this,[this](const FOnAttributeChangeData& AttributeChangeData)
		{
			if (FMath::IsNearlyZero(AttributeChangeData.NewValue))
			{
				K2_EndAbility();		
			}
		});	
	}
}

void UPRGA_Mod_GrantAbility::CleanupGrantedAbilityRuntime(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (UPRItemInstance_Weapon* SourceWeapon = Cast<UPRItemInstance_Weapon>(GetCurrentSourceObject()))
	{
		// 기본 사격 상태 복원
		SourceWeapon->FireModeState = EPRWeaponFireModeState::BaseFire;
		if (UPRWeaponManagerComponent* WeaponManager = GetWeaponManager())
		{
			// 현재 슬롯 사격 모드 태그 갱신
			WeaponManager->RefreshCurrentWeaponFireModeTags();
		}
	}

	UAbilitySystemComponent* ASC = ActorInfo != nullptr ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!IsValid(ASC) && !HasAnyFlags(RF_ClassDefaultObject))
	{
		ASC = GetAbilitySystemComponentFromActorInfo();
	}

	if (!IsValid(ASC))
	{
		CostExhaustedHandle.Reset();
		GrantedSpecHandle = FGameplayAbilitySpecHandle();
		return;
	}

	if (CostAttribute.IsValid() && CostExhaustedHandle.IsValid())
	{
		// 비용 감시 해제
		ASC->GetGameplayAttributeValueChangeDelegate(CostAttribute).Remove(CostExhaustedHandle);
		CostExhaustedHandle.Reset();
	}

	if (GrantedSpecHandle.IsValid())
	{
		// 부여 어빌리티 회수
		ASC->ClearAbility(GrantedSpecHandle);
		GrantedSpecHandle = FGameplayAbilitySpecHandle();
	}
}

void UPRGA_Mod_GrantAbility::BindWeaponEquipmentChanged()
{
	if (SourceWeaponSlot == EPRWeaponSlotType::None)
	{
		return;
	}

	UPRWeaponManagerComponent* WeaponManager = GetWeaponManager();
	if (!IsValid(WeaponManager))
	{
		return;
	}

	// 무기 변경 이벤트 바인딩
	WeaponManager->GetOnWeaponEquipmentChanged().AddUniqueDynamic(this, &ThisClass::HandleWeaponEquipmentChanged);
	BoundWeaponManager = WeaponManager;
}

void UPRGA_Mod_GrantAbility::UnbindWeaponEquipmentChanged()
{
	UPRWeaponManagerComponent* WeaponManager = BoundWeaponManager.Get();
	if (IsValid(WeaponManager))
	{
		// 무기 변경 이벤트 해제
		WeaponManager->GetOnWeaponEquipmentChanged().RemoveDynamic(this, &ThisClass::HandleWeaponEquipmentChanged);
	}

	BoundWeaponManager.Reset();
	SourceWeaponSlot = EPRWeaponSlotType::None;
}

void UPRGA_Mod_GrantAbility::HandleWeaponEquipmentChanged(UPRWeaponManagerComponent* WeaponManagerComponent, EPRWeaponSlotType ChangedSlot)
{
	(void)ChangedSlot;

	if (!IsValid(WeaponManagerComponent) || SourceWeaponSlot == EPRWeaponSlotType::None)
	{
		return;
	}

	if (WeaponManagerComponent->GetCurrentWeaponSlot() == SourceWeaponSlot)
	{
		return;
	}

	// 슬롯 변경 종료
	K2_EndAbility();
}

void UPRGA_Mod_GrantAbility::OnInputPressed(float TimeWaited)
{
	K2_EndAbility();
}
