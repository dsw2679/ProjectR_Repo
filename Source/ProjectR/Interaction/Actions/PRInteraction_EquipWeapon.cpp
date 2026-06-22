// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (장비 Weapon 상호작용 액션 실행 로직 구현)
#include "PRInteraction_EquipWeapon.h"

#include "GameFramework/Pawn.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"
#include "ProjectR/World/Pickable/PRPickableWeapon.h"

bool UPRInteraction_EquipWeapon::CanInteract_Implementation(AActor* Interactor) const
{
	if (!Super::CanInteract_Implementation(Interactor))
	{
		return false;
	}

	// 소유 액터가 픽업 무기 액터이고 WeaponData가 설정되어 있어야 장착 가능
	const APRPickableWeapon* PickableWeapon = Cast<APRPickableWeapon>(GetOwner());
	if (!IsValid(PickableWeapon) || !IsValid(PickableWeapon->GetWeaponData()))
	{
		return false;
	}

	return true;
}

void UPRInteraction_EquipWeapon::Execute_Implementation(AActor* Interactor)
{
	Super::Execute_Implementation(Interactor);

	APRPickableWeapon* PickableWeapon = Cast<APRPickableWeapon>(GetOwner());
	if (!IsValid(PickableWeapon) || !PickableWeapon->HasAuthority())
	{
		return;
	}

	UPRWeaponDataAsset* WeaponData = PickableWeapon->GetWeaponData();
	if (!IsValid(WeaponData))
	{
		return;
	}

	// 무기 매니저는 Pawn 소유 컴포넌트이므로 Interactor에서 Pawn 해석
	APawn* InteractorPawn = GetPawn(Interactor);
	if (!IsValid(InteractorPawn))
	{
		return;
	}

	UPRWeaponManagerComponent* WeaponManager = nullptr;
	if (APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(InteractorPawn))
	{
		WeaponManager = PlayerCharacter->GetWeaponManager();
	}

	if (!IsValid(WeaponManager))
	{
		return;
	}

	// 인벤토리는 PlayerState 소유이므로 GameplayStatics로 해석
	UPRInventoryComponent* InventoryComponent = UPRGameplayStatics::GetInventoryComponent(InteractorPawn);
	if (!IsValid(InventoryComponent))
	{
		return;
	}
	
	UPRItemInstance_Weapon* Item = InventoryComponent->FindItemByData<UPRItemInstance_Weapon>(WeaponData);
	if (!IsValid(Item))
	{
		Item = InventoryComponent->AddItem<UPRItemInstance_Weapon>(WeaponData);
	}
	if (!IsValid(Item))
	{
		return;
	}

	WeaponManager->EquipWeapon(Item);

	// 장착 완료 후 액터 정리 옵션 처리
	if (PickableWeapon->ShouldDestroyOnPickup())
	{
		PickableWeapon->Destroy();
	}
}
