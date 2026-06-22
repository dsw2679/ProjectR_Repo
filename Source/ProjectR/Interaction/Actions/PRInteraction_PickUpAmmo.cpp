// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (필드 탄약 획득 및 인벤토리 탄약 보관 구현)
// Author: 이건주 (Mod 버프 연동 탄약 보너스 획득 구현)
#include "PRInteraction_PickUpAmmo.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
#include "ProjectR/World/Pickable/PRPickableAmmo.h"

void UPRInteraction_PickUpAmmo::Execute_Implementation(AActor* Interactor)
{
	Super::Execute_Implementation(Interactor);

	APRPickableAmmo* PickableAmmo = Cast<APRPickableAmmo>(GetOwner());
	if (!IsValid(PickableAmmo) || !PickableAmmo->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UPRGameplayStatics::GetAbilitySystemComponent(Interactor);
	if (!IsValid(TargetASC))
	{
		return;
	}

	const float Remaining = PickableAmmo->GrantAmmoAndGetRemaining(TargetASC);

	// 전부 적립된 경우만 픽업 액터 제거. 일부만 흡수되면 잔여량을 갱신해 월드에 남긴다
	if (Remaining <= 0.f)
	{
		PickableAmmo->Destroy();
	}
	else
	{
		PickableAmmo->SetAmmo(PickableAmmo->GetAmmoType(), Remaining);
	}
}
