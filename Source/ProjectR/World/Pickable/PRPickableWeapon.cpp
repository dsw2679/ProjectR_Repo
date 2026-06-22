// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (월드 배치용 획득 가능 Weapon 및 관련 시스템 구현)
#include "PRPickableWeapon.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "ProjectR/Interaction/PRInteractableComponent.h"
#include "ProjectR/Interaction/Actions/PRInteraction_EquipWeapon.h"
#include "ProjectR/ItemSystem/Actors/PRWeaponActor.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"

APRPickableWeapon::APRPickableWeapon()
{
	SetReplicates(true);

	InteractionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollision"));
	InteractionCollision->SetSphereRadius(50.f);
	InteractionCollision->SetCollisionProfileName(FName("Interactable"));
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetRootComponent(InteractionCollision);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 상호작용 시 WeaponData 장착을 수행할 Action 등록
	InteractableComponent->InteractionActions.Add(CreateDefaultSubobject<UPRInteraction_EquipWeapon>(TEXT("EquipWeaponAction")));
}

#if WITH_EDITOR
void APRPickableWeapon::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// WeaponData 변경 시 표시 메시를 즉시 동기화
	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(APRPickableWeapon, WeaponData))
	{
		RefreshVisual();
	}
}
#endif

void APRPickableWeapon::RefreshVisual()
{
	if (!IsValid(WeaponMesh))
	{
		return;
	}

	// WeaponData가 비어 있거나 WeaponActorClass가 설정되지 않은 경우 메시 클리어
	if (!IsValid(WeaponData) || !WeaponData->WeaponActorClass)
	{
		WeaponMesh->SetSkeletalMesh(nullptr);
		return;
	}

	// WeaponActor CDO의 메시 컴포넌트에서 스켈레탈 메시 자산을 가져온다
	const APRWeaponActor* WeaponActorCDO = WeaponData->WeaponActorClass->GetDefaultObject<APRWeaponActor>();
	if (!IsValid(WeaponActorCDO))
	{
		WeaponMesh->SetSkeletalMesh(nullptr);
		return;
	}

	const USkeletalMeshComponent* CDOMeshComponent = WeaponActorCDO->GetWeaponMeshComponent();
	USkeletalMesh* SourceMesh = IsValid(CDOMeshComponent) ? CDOMeshComponent->GetSkeletalMeshAsset() : nullptr;
	WeaponMesh->SetSkeletalMesh(SourceMesh);
}
