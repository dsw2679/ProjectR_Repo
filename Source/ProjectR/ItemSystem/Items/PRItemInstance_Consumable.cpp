// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (소모품 사용 시 인벤토리 수량 차감 및 효과 연동 구현)
// Author: 이건주 (퀵슬롯 연동 및 실시간 HUD 개수 갱신 구현)
#include "PRItemInstance_Consumable.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "ProjectR/AbilitySystem/Abilities/Player/Consumable/PRGA_UseConsumable.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Components/PRQuickSlotComponent.h"
#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/Utils/PRGameplayStatics.h"

void UPRItemInstance_Consumable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool UPRItemInstance_Consumable::ActivateItem(const FPRItemActivationContext& ActivationContext)
{
	// 권위 검증
	if (!IsValid(ActivationContext.UserActor) || !ActivationContext.UserActor->HasAuthority())
	{
		return false;
	}

	// 소비 아이템 데이터와 등록 대상 슬롯 번호 확인
	// 인벤토리 UI는 소비 아이템 리스트를 열 때 클릭한 퀵슬롯 번호를 ContextIndex로 넘김
	UPRConsumableDataAsset* ConsumableData = GetConsumableData();
	if (!IsValid(ConsumableData) || ActivationContext.ContextIndex == INDEX_NONE)
	{
		return false;
	}

	// 퀵슬롯 컴포넌트 조회
	UPRQuickSlotComponent* QuickSlotComponent = UPRGameplayStatics::GetQuickSlotComponent(ActivationContext.UserActor);
	if (!IsValid(QuickSlotComponent))
	{
		return false;
	}

	QuickSlotComponent->RequestRegisterQuickSlotItem(ActivationContext.ContextIndex, ConsumableData);
	return true;
}

bool UPRItemInstance_Consumable::UseItem(AActor* UserActor)
{
	const UPRConsumableDataAsset* ConsumableData = GetConsumableData();
	
	if (!IsValid(UserActor) || !UserActor->HasAuthority())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ConsumableItem][Server] 사용 실패. UseItem() | Owner = %s | User = %s | Item = %s | Count = %d | Reason = InvalidAuthorityOrUser"),
			*GetNameSafe(GetTypedOuter<UPRInventoryComponent>()),
			*GetNameSafe(UserActor),
			*GetNameSafe(ConsumableData),
			StackCount);
		return false;
	}

	if (!CanUseItem(UserActor))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ConsumableItem][Server] 사용 실패. UseItem() | Owner = %s | User = %s | Item = %s | Count = %d | Reason = CannotUse"),
			*GetNameSafe(GetTypedOuter<UPRInventoryComponent>()),
			*GetNameSafe(UserActor),
			*GetNameSafe(ConsumableData),
			StackCount);
		return false;
	}

	UAbilitySystemComponent* ASC = UPRGameplayStatics::GetAbilitySystemComponent(UserActor);
	if (!IsValid(ASC) || !IsValid(ConsumableData->UseAbilityClass))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ConsumableItem][Server] 사용 실패. UseItem() | Owner = %s | User = %s | Item = %s | Count = %d | Reason = InvalidAbility"),
			*GetNameSafe(GetTypedOuter<UPRInventoryComponent>()),
			*GetNameSafe(UserActor),
			*GetNameSafe(ConsumableData),
			StackCount);
		return false;
	}

	FGameplayEventData EventData;
	EventData.Instigator = UserActor;
	EventData.Target = UserActor;
	EventData.OptionalObject = this;

	FGameplayAbilitySpec AbilitySpec(ConsumableData->UseAbilityClass, 1, INDEX_NONE, this);
	const FGameplayAbilitySpecHandle AbilityHandle = ASC->GiveAbilityAndActivateOnce(AbilitySpec, &EventData);
	if (!AbilityHandle.IsValid())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ConsumableItem][Server] 사용 실패. UseItem() | Owner = %s | User = %s | Item = %s | Count = %d | Reason = ActivateAbilityFailed"),
			*GetNameSafe(GetTypedOuter<UPRInventoryComponent>()),
			*GetNameSafe(UserActor),
			*GetNameSafe(ConsumableData),
			StackCount);
		return false;
	}
	// 스택 소모는 Use Consumable 어빌리티 내에서 관리 

	return true;
}

UPRConsumableDataAsset* UPRItemInstance_Consumable::GetConsumableData() const
{
	return Cast<UPRConsumableDataAsset>(ItemData);
}

bool UPRItemInstance_Consumable::CanUseItem(AActor* UserActor) const
{
	const UPRConsumableDataAsset* ConsumableData = GetConsumableData();
	return IsValid(UserActor) && IsValid(ConsumableData) && IsValid(ConsumableData->UseAbilityClass) && StackCount > 0;
}
