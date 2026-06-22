// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (동료 소생 상호작용 액션 실행 로직 구현)
#include "PRInteraction_Revive.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Consumable.h"
#include "ProjectR/Utils/PRGameplayStatics.h"

void UPRInteraction_Revive::OnHoldStart_Implementation(AActor* Interactor)
{
	Super::OnHoldStart_Implementation(Interactor);
	
	if (Interactor->HasAuthority())
	{
		UE_LOG(LogTemp,Warning,TEXT("[Server] Revive Hold Start"));	
	}
	else
	{
		UE_LOG(LogTemp,Warning,TEXT("[Client] Revive Hold Start"));	
	}
	
	if (UAbilitySystemComponent* ASC = UPRGameplayStatics::GetAbilitySystemComponent(Interactor))
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(ASC->GetOwner(),PRGameplayTags::Event_Ability_Revive_Start,FGameplayEventData());
	}
}

void UPRInteraction_Revive::OnHoldCanceled_Implementation(AActor* Interactor)
{
	Super::OnHoldCanceled_Implementation(Interactor);
	if (Interactor->HasAuthority())
	{
		UE_LOG(LogTemp,Warning,TEXT("[Server] Revive Hold Canceled"));	
	}
	else
	{
		UE_LOG(LogTemp,Warning,TEXT("[Client] Revive Hold Canceled"));	
	}
	FinishReviveAbility(Interactor,true);
}

void UPRInteraction_Revive::OnHoldFinished_Implementation(AActor* Interactor)
{
	Super::OnHoldFinished_Implementation(Interactor);
	if (Interactor->HasAuthority())
	{
		UE_LOG(LogTemp,Warning,TEXT("[Server] Revive Hold Finished"));	
	}
	else
	{
		UE_LOG(LogTemp,Warning,TEXT("[Client] Revive Hold Finished"));	
	}
}

void UPRInteraction_Revive::Execute_Implementation(AActor* Interactor)
{
	ensureMsgf(IsValid(CostItem),TEXT("소생 코스트 아이템이 설정되어 있어야함."));
	Super::Execute_Implementation(Interactor);
	
	FinishReviveAbility(Interactor,false);
	
	if (UAbilitySystemComponent* OwnerASC = UPRGameplayStatics::GetAbilitySystemComponent(GetOwner()))
	{
		// 체력 회복 GE 적용
		if (IsValid(HealEffectClass))
		{
			FGameplayEffectContextHandle Context = OwnerASC->MakeEffectContext();
			Context.AddSourceObject(this);
			const FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(HealEffectClass, 1.f, Context);
			if (SpecHandle.IsValid() && SpecHandle.Data.IsValid())
			{
				OwnerASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
			}
		}
		
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerASC->GetOwner(),PRGameplayTags::Event_Ability_GetUp,FGameplayEventData());
	}
	
	if (UPRInventoryComponent* InteractorInventory = UPRGameplayStatics::GetInventoryComponent(Interactor))
	{
		InteractorInventory->RequestRemoveItemByData(CostItem, 1);
	}
}

bool UPRInteraction_Revive::CanInteract_Implementation(AActor* Interactor) const
{
	ensureMsgf(IsValid(CostItem),TEXT("소생 코스트 아이템이 설정되어 있어야함."));
	
	UAbilitySystemComponent* OwnerASC = UPRGameplayStatics::GetAbilitySystemComponent(GetOwner());
	if (!IsValid(OwnerASC))
	{
		return false;
	}
	
	if (!OwnerASC->HasMatchingGameplayTag(PRGameplayTags::State_Down))
	{
		return false;
	}
	
	UAbilitySystemComponent* InteractorASC = UPRGameplayStatics::GetAbilitySystemComponent(Interactor);
	if (IsValid(InteractorASC) && InteractorASC->HasMatchingGameplayTag(PRGameplayTags::State_Reviving))
	{
		return false;
	}
	
	UPRInventoryComponent* InteractorInventory = UPRGameplayStatics::GetInventoryComponent(Interactor);
	if (!IsValid(InteractorInventory))
	{
		return false;
	}
	
	if (UPRItemInstance_Consumable* CostItemInstance = InteractorInventory->FindItemByData<UPRItemInstance_Consumable>(CostItem))
	{
		return CostItemInstance->GetStackCount() > 0;
	}
	
	return false;
}

void UPRInteraction_Revive::FinishReviveAbility(AActor* Interactor, bool bCanceled)
{
	if (UAbilitySystemComponent* ASC = UPRGameplayStatics::GetAbilitySystemComponent(Interactor))
	{
		if (bCanceled)
		{
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(ASC->GetOwner(),PRGameplayTags::Event_Ability_Revive_Canceled,FGameplayEventData());	
		}
		else
		{
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(ASC->GetOwner(),PRGameplayTags::Event_Ability_Revive_Finished,FGameplayEventData());
		}
	}
}
