// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (드롭 시스템 연동 탄약 픽업 구현)
// Author: 배유찬 (탄약 획득 상호작용 및 탄종별 획득 판정 구현)
// Author: 이건주 (Mod 버프 연동 탄약 회복 보정 구현)
#include "PRPickableAmmo.h"

#include "AbilitySystemComponent.h"
#include "NiagaraComponent.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"

APRPickableAmmo::APRPickableAmmo()
{
	SetReplicates(true);
	SetNetCullDistanceSquared(FMath::Square(5000.f));

	AmmoMesh = CreateDefaultSubobject<UStaticMeshComponent>("AmmoMesh");
	AmmoNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AmmoNiagara"));

	AmmoMesh->SetupAttachment(RootComponent);
	AmmoNiagara->SetupAttachment(AmmoMesh);
}

void APRPickableAmmo::SetAmmo(EPRAmmoType InAmmoType, float InAmmoAmount)
{
	if (!HasAuthority())
	{
		return;
	}
	
	const bool bTypeChanged = (AmmoType != InAmmoType);
	AmmoType = InAmmoType;
	AmmoAmount = InAmmoAmount;

	// 서버는 OnRep을 받지 않으므로 타입 변경 시 직접 비주얼 갱신
	if (bTypeChanged)
	{
		RefreshVisual();
	}
}

void APRPickableAmmo::BeginPlay()
{
	Super::BeginPlay();

	// 초기 스폰 또는 복제 도착 시점 비주얼 동기화
	RefreshVisual();
}

void APRPickableAmmo::RefreshVisual()
{
	const FPRAmmoVisualInfo* VisualInfo = AmmoTypeVisual.Find(AmmoType);
	if (VisualInfo == nullptr)
	{
		return;
	}

	if (IsValid(AmmoMesh))
	{
		AmmoMesh->SetStaticMesh(VisualInfo->AmmoMesh);
	}

	if (IsValid(AmmoNiagara))
	{
		AmmoNiagara->SetAsset(VisualInfo->AmmoNiagaraEffect);
		AmmoNiagara->SetVariableLinearColor(FName("EffectColor"), VisualInfo->EffectColor);
	}
}

float APRPickableAmmo::GrantAmmoAndGetRemaining(UAbilitySystemComponent* TargetASC)
{
	// 서버 권위에서만 자원 갱신 수행
	if (!HasAuthority() || !IsValid(TargetASC) || AmmoAmount <= 0.f)
	{
		return AmmoAmount;
	}

	const TSubclassOf<UGameplayEffect>* FoundGE = AmmoGEMap.Find(AmmoType);
	const TSubclassOf<UGameplayEffect> EffectClass = FoundGE ? *FoundGE : nullptr;
	if (!IsValid(EffectClass))
	{
		return AmmoAmount;
	}

	const UPRAttributeSet_Weapon* WeaponSet = TargetASC->GetSet<UPRAttributeSet_Weapon>();
	if (!IsValid(WeaponSet))
	{
		return AmmoAmount;
	}

	// AmmoAmount를 MaxMagazineAmmo 대비 백분율로 해석하여 실제 적립량 계산
	const float MaxMagazineAmmo = WeaponSet->GetMaxMagazineAmmoByType(AmmoType);
	const float DesiredAmount = FMath::FloorToFloat(MaxMagazineAmmo * AmmoAmount / 100.0f);
	if (DesiredAmount <= 0.f)
	{
		return AmmoAmount;
	}

	// SetByCaller로 실제 적립량을 GE Spec에 전달
	FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
	Context.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return AmmoAmount;
	}

	// 적용 전후 예비탄 비교로 MaxReserveAmmo 클램프에 잘린 잔여량 산출
	const float BeforeReserve = WeaponSet->GetReserveAmmoByType(AmmoType);

	SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_AmmoMagnitude, DesiredAmount);
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);

	const float AfterReserve = WeaponSet->GetReserveAmmoByType(AmmoType);
	const float Granted = FMath::Max(AfterReserve - BeforeReserve, 0.f);
	const float RemainingRaw = FMath::Max(DesiredAmount - Granted, 0.f);

	// 잔여 raw를 백분율로 환산하여 반환
	if (RemainingRaw <= 0.f)
	{
		return 0.f;
	}

	if (MaxMagazineAmmo <= KINDA_SMALL_NUMBER)
	{
		return AmmoAmount;
	}

	return FMath::CeilToFloat(RemainingRaw * 100.0f / MaxMagazineAmmo);
}

void APRPickableAmmo::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRPickableAmmo, AmmoType);
	DOREPLIFETIME(APRPickableAmmo, AmmoAmount);
}

void APRPickableAmmo::OnRep_AmmoType()
{
	RefreshVisual();
}

void APRPickableAmmo::OnRep_AmmoAmount()
{
}
