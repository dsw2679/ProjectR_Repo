// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (패스트 트래블 및 테스트 환경 셋업 디버그 치트 구현)
// Author: 이건주 (무한 탄약 및 Mod 게이지 충전 디버그 치트 구현)
#include "PRCheatHandler.h"

#include "Engine/NetDriver.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameplayEffect.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Interaction/PRInteractorComponent.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "ProjectR/PRGameplayTags.h"


/*~ RPC 라우팅 ~*/

int32 UPRCheatHandler::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	APRPlayerController* PC = GetTypedOuter<APRPlayerController>();
	return IsValid(PC) ? PC->GetFunctionCallspace(Function, Stack) : FunctionCallspace::Local;
}

bool UPRCheatHandler::CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack)
{
	APRPlayerController* PC = GetTypedOuter<APRPlayerController>();
	if (!IsValid(PC))
	{
		return false;
	}

	UNetDriver* NetDriver = PC->GetNetDriver();
	if (!NetDriver)
	{
		return false;
	}

	NetDriver->ProcessRemoteFunction(PC, Function, Parms, OutParms, Stack, this);
	return true;
}


/*~ 리스폰 ~*/

void UPRCheatHandler::ServerCheatRespawn_Implementation()
{
#if !UE_BUILD_SHIPPING
	APRPlayerController* PC = GetTypedOuter<APRPlayerController>();
	if (!IsValid(PC))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	AGameModeBase* GameMode = World->GetAuthGameMode();
	if (!IsValid(GameMode))
	{
		return;
	}

	// 상호작용 상태 초기화 (포커스 + 진행 중 Hold/Sustained)
	if (UPRInteractorComponent* Interactor = PC->FindComponentByClass<UPRInteractorComponent>())
	{
		Interactor->RequestEndInteract(true);
		Interactor->ClearFocus();
	}

	APRPlayerState* PRPlayerState = PC->GetPlayerState<APRPlayerState>();
	if (IsValid(PRPlayerState))
	{
		// 리스폰 직전 저장 데이터 보관
		PRPlayerState->PrepareStateForRespawn();

		// PlayerState 소유 시스템 런타임 상태 초기화
		PRPlayerState->ResetState();
	}

	// 기존 폰 제거
	if (APawn* CurrentPawn = PC->GetPawn())
	{
		PC->UnPossess();
		CurrentPawn->Destroy();
	}

	GameMode->RestartPlayer(PC);

	// 리스폰 후 치트 공격력 재적용
	ApplyAttackPowerCheatEffect();
#endif
}


/*~ 자원 충전 ~*/

void UPRCheatHandler::ServerCheatFillAmmo_Implementation()
{
#if !UE_BUILD_SHIPPING
	UPRAbilitySystemComponent* ASC = GetOwningPlayerASC();
	if (!IsValid(ASC))
	{
		return;
	}

	const UPRAttributeSet_Weapon* WeaponSet = ASC->GetSet<UPRAttributeSet_Weapon>();
	if (!IsValid(WeaponSet))
	{
		return;
	}

	const EPRAmmoType AmmoTypes[] = { EPRAmmoType::Primary, EPRAmmoType::Secondary };
	for (const EPRAmmoType AmmoType : AmmoTypes)
	{
		const FGameplayAttribute MagazineAttr = UPRAttributeSet_Weapon::GetMagazineAmmoAttribute(AmmoType);
		const FGameplayAttribute ReserveAttr = UPRAttributeSet_Weapon::GetReserveAmmoAttribute(AmmoType);

		if (MagazineAttr.IsValid())
		{
			ASC->SetNumericAttributeBase(MagazineAttr, WeaponSet->GetMaxMagazineAmmoByType(AmmoType));
		}

		if (ReserveAttr.IsValid())
		{
			ASC->SetNumericAttributeBase(ReserveAttr, WeaponSet->GetMaxReserveAmmoByType(AmmoType));
		}
	}
#endif
}


/*~ 무한 모드 ~*/

void UPRCheatHandler::ServerCheatInfiniteMode_Implementation(bool bEnable)
{
#if !UE_BUILD_SHIPPING
	UPRAbilitySystemComponent* ASC = GetOwningPlayerASC();
	if (!IsValid(ASC))
	{
		return;
	}

	const bool bAlreadyApplied = InfiniteModeEffectHandle.IsValid();

	if (bEnable)
	{
		// 이미 적용 중이면 중복 적용 방지
		if (bAlreadyApplied)
		{
			return;
		}

		if (!IsValid(InfiniteModeEffectClass))
		{
			return;
		}

		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddSourceObject(this);

		const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(InfiniteModeEffectClass, 1.f, ContextHandle);
		if (!SpecHandle.IsValid())
		{
			return;
		}

		InfiniteModeEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
	else
	{
		// 적용된 상태에서만 회수
		if (!bAlreadyApplied)
		{
			return;
		}

		ASC->RemoveActiveGameplayEffect(InfiniteModeEffectHandle);
		InfiniteModeEffectHandle.Invalidate();
	}
#endif
}


/*~ 공격력 치트 ~*/

void UPRCheatHandler::ServerCheatAddAttackPower_Implementation(float Amount)
{
#if !UE_BUILD_SHIPPING
	if (Amount <= 0.0f)
	{
		return;
	}

	const float PreviousCheatAddedAttackPower = CheatAddedAttackPower;
	CheatAddedAttackPower += Amount;
	if (!ApplyAttackPowerCheatEffect())
	{
		// 적용 실패 시 이전 치트 수치 복원
		CheatAddedAttackPower = PreviousCheatAddedAttackPower;
		ApplyAttackPowerCheatEffect();
	}
#endif
}

void UPRCheatHandler::ServerCheatResetAttackPower_Implementation()
{
#if !UE_BUILD_SHIPPING
	RemoveAttackPowerCheatEffect();
	CheatAddedAttackPower = 0.0f;
#endif
}


/*~ 플라이 모드 ~*/

void UPRCheatHandler::ServerCheatFly_Implementation(bool bEnable)
{
#if !UE_BUILD_SHIPPING
	APRPlayerController* PC = GetTypedOuter<APRPlayerController>();
	if (!IsValid(PC))
	{
		return;
	}

	APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(PC->GetPawn());
	if (!IsValid(Character))
	{
		return;
	}
	Character->MulticastSetMovementMode(bEnable ? MOVE_Flying : MOVE_Walking);
#endif
}


/*~ 내부 헬퍼 ~*/

UPRAbilitySystemComponent* UPRCheatHandler::GetOwningPlayerASC() const
{
	APRPlayerController* PC = GetTypedOuter<APRPlayerController>();
	if (!IsValid(PC))
	{
		return nullptr;
	}

	APRPlayerState* PS = PC->GetPlayerState<APRPlayerState>();
	if (!IsValid(PS))
	{
		return nullptr;
	}

	return PS->GetPRAbilitySystemComponent();
}

bool UPRCheatHandler::ApplyAttackPowerCheatEffect()
{
	UPRAbilitySystemComponent* ASC = GetOwningPlayerASC();
	if (!IsValid(ASC))
	{
		return false;
	}

	if (CheatAddedAttackPower <= 0.0f)
	{
		// 누적 수치 없음
		RemoveAttackPowerCheatEffect();
		return false;
	}

	if (!IsValid(AttackPowerCheatEffectClass))
	{
		return false;
	}

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(AttackPowerCheatEffectClass, 1.0f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	// 성장 공격력 보너스와 같은 SetByCaller 경로 사용
	SpecHandle.Data->SetSetByCallerMagnitude(PRGameplayTags::SetByCaller_Trait_PlayerAttackPower, CheatAddedAttackPower);

	// 새 효과 준비 완료 후 기존 효과 교체
	RemoveAttackPowerCheatEffect();

	AttackPowerCheatEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	return AttackPowerCheatEffectHandle.IsValid();
}

void UPRCheatHandler::RemoveAttackPowerCheatEffect()
{
	UPRAbilitySystemComponent* ASC = GetOwningPlayerASC();
	if (IsValid(ASC) && AttackPowerCheatEffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(AttackPowerCheatEffectHandle);
	}

	AttackPowerCheatEffectHandle.Invalidate();
}
