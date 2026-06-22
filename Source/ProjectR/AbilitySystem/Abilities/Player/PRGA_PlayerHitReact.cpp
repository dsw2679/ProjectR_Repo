// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (플레이어 피격 경직 시퀀스 및 화면 이펙트 구현)
// Author: 배유찬 (플래시라이트 상태 연동)
#include "PRGA_PlayerHitReact.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"
#include "TimerManager.h"

namespace
{
	// 다운 FallLoop에 들어간 뒤 바닥을 확인하는 주기다.
	constexpr float DownGroundCheckInterval = 0.05f;

	// 캡슐 하단에서 이 거리만큼 아래를 훑어 착지 가능한 바닥을 찾는다.
	constexpr float DownGroundCheckDistance = 20.0f;

	// 너무 가파른 면을 바닥으로 처리하지 않기 위한 최소 법선 Z 값이다.
	constexpr float DownGroundWalkableNormalZ = 0.5f;

	EPRDamageFXIntensity ResolveDamageFXIntensity(EPRPlayerHitReactType HitReactType)
	{
		switch (HitReactType)
		{
		case EPRPlayerHitReactType::Strong:
			return EPRDamageFXIntensity::Strong;
		case EPRPlayerHitReactType::Down:
			return EPRDamageFXIntensity::Down;
		case EPRPlayerHitReactType::Weak:
		default:
			return EPRDamageFXIntensity::Weak;
		}
	}
}

// 피격 리액션 Ability의 태그, 트리거, 실행 정책을 초기화한다.
UPRGA_PlayerHitReact::UPRGA_PlayerHitReact()
{
	// AttributeSet에서 발행한 피격 리액션 이벤트만 이 Ability를 깨운다.
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_HitReact);
	SetAssetTags(DefaultAbilityTags);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(PRGameplayTags::Cooldown_Ability_PlayerHitReact);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_PlayerInputLocked);

	// 사격 취소
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Zoom);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Revive);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_UseConsumable);
	
	// 피격 리액션 중 새 액션이 끼어들지 않도록 기본 차단 태그를 둔다.
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Zoom);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Aim);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Crouch);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Sprint);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Reload);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Interaction);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_HitReact);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_UseConsumable);

	FAbilityTriggerData WeakTriggerData;
	WeakTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	WeakTriggerData.TriggerTag = PRGameplayTags::Event_Ability_PlayerHitReact_Weak;
	AbilityTriggers.Add(WeakTriggerData);

	FAbilityTriggerData StrongTriggerData;
	StrongTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	StrongTriggerData.TriggerTag = PRGameplayTags::Event_Ability_PlayerHitReact_Strong;
	AbilityTriggers.Add(StrongTriggerData);

	FAbilityTriggerData DownTriggerData;
	DownTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	DownTriggerData.TriggerTag = PRGameplayTags::Event_Ability_PlayerHitReact_Down;
	AbilityTriggers.Add(DownTriggerData);

	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}

// 피격 리액션 이벤트를 해석하고, 행동 취소와 몽타주 재생을 시작한다.
void UPRGA_PlayerHitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const EPRPlayerHitReactType HitReactType = ResolveHitReactType(TriggerEventData);
	if (HitReactType == EPRPlayerHitReactType::None)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bHitReactFinished = false;
	ClearActionLock();
	ClearDownHitReact();
	ActiveHitReactType = HitReactType;
	ActivePlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (IsLocallyControlled())
	{
		APRPlayerCharacter* PlayerCharacter = ActivePlayerCharacter.Get();
		if (IsValid(PlayerCharacter))
		{
			APRPlayerController* PlayerController = Cast<APRPlayerController>(PlayerCharacter->GetController());
			if (IsValid(PlayerController))
			{
				UPRUIControllerComponent* UIController = PlayerController->GetUIController();
				if (IsValid(UIController))
				{
					UIController->ShowDamageFX(ResolveDamageFXIntensity(HitReactType));
				}
			}
		}
	}
	CancelActionsForHitReact(HitReactType);
	ApplyRecoverableHealthRecoveryDelay(TriggerEventData);

	// 이벤트 타입을 실제 재생할 몽타주로 변환한다.
	UAnimMontage* HitReactMontage = SelectHitReactMontage(HitReactType);
	if (!IsValid(HitReactMontage))
	{
		FinishHitReact(true);
		return;
	}
	if (HitReactType == EPRPlayerHitReactType::Down && !HasRequiredDownMontageSections(HitReactMontage))
	{
		FinishHitReact(true);
		return;
	}

	if (APRPlayerCharacter* PlayerCharacter = ActivePlayerCharacter.Get())
	{
		if (HitReactType == EPRPlayerHitReactType::Strong || HitReactType == EPRPlayerHitReactType::Down)
		{
			// 강한 경직과 다운은 현재 이동을 즉시 끊어 행동불능 상태가 움직임과 섞이지 않게 한다.
			UCharacterMovementComponent* CharacterMovement = PlayerCharacter->GetCharacterMovement();
			if (IsValid(CharacterMovement))
			{
				CharacterMovement->StopMovementImmediately();
			}
		}
	}

	StartActionLock(HitReactType);
	if (HitReactType == EPRPlayerHitReactType::Down)
	{
		// 다운은 하나의 몽타주 안에서 Start, FallLoop, Land, GetUp 섹션을 직접 제어한다.
		StartDownHitReact();
	}

	ActiveHitReactMontage = HitReactMontage;
	const FName StartSection = HitReactType == EPRPlayerHitReactType::Down ? DownStartSectionName : NAME_None;
	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		HitReactMontage,
		FMath::Max(MontagePlayRate, UE_SMALL_NUMBER),
		StartSection);

	if (!IsValid(ActiveMontageTask))
	{
		ClearDownHitReact();
		ClearActionLock();
		FinishHitReact(true);
		return;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGA_PlayerHitReact::HandleHitReactMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRGA_PlayerHitReact::HandleHitReactMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGA_PlayerHitReact::HandleHitReactMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGA_PlayerHitReact::HandleHitReactMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
	if (HitReactType == EPRPlayerHitReactType::Down)
	{
		// 몽타주가 실제로 시작된 뒤 섹션 연결을 덮어써 에셋 기본 체인의 영향을 줄인다.
		ConfigureDownMontageFlow();
	}
}

// 피격 리액션 종료 시 태그, 타이머, 몽타주 태스크를 정리한다.
void UPRGA_PlayerHitReact::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bHitReactFinished = true;
	ClearDownHitReact();
	ClearDownHitReactCooldown();
	ClearActionLock();

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	if (IsValid(ActiveHitReactMontage))
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (IsValid(ASC))
		{
			ASC->CurrentMontageStop(MontageStopBlendOutTime);
		}
		ActiveHitReactMontage = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// 피격 리액션 몽타주가 정상 종료되었을 때 Ability를 마무리한다.
void UPRGA_PlayerHitReact::HandleHitReactMontageCompleted()
{
	ClearActionLock();
	FinishHitReact(false);
}

// 피격 리액션 몽타주가 취소되거나 끊겼을 때 Ability를 취소 종료한다.
void UPRGA_PlayerHitReact::HandleHitReactMontageInterrupted()
{
	ClearActionLock();
	FinishHitReact(true);
}

// 다운 몽타주의 현재 섹션을 추적하고 착지 감지 시작과 Land 전환을 처리한다.
void UPRGA_PlayerHitReact::HandleDownMontageSectionChanged(UAnimMontage* Montage, FName SectionName, bool)
{
	if (ActiveHitReactType != EPRPlayerHitReactType::Down
		|| bHitReactFinished
		|| Montage != ActiveHitReactMontage.Get())
	{
		return;
	}

	if (SectionName == DownStartSectionName)
	{
		DownHitReactPhase = EPRPlayerDownHitReactPhase::Start;
		RefreshDownMontageSectionFlow();
		return;
	}

	if (SectionName == DownFallLoopSectionName)
	{
		DownHitReactPhase = EPRPlayerDownHitReactPhase::FallLoop;
		RefreshDownMontageSectionFlow();
		if (bDownLandRequested)
		{
			JumpToDownSection(DownLandSectionName);
			return;
		}

		// 루트모션 중에는 CharacterMovement의 Falling 상태를 신뢰하기 어려워 여기서부터 직접 바닥을 감지한다.
		StartDownGroundCheck();
		return;
	}

	if (SectionName == DownLandSectionName)
	{
		DownHitReactPhase = EPRPlayerDownHitReactPhase::Land;
		bDownLandRequested = false;
		StopDownGroundCheck();
		return;
	}

	if (SectionName == DownGetUpSectionName)
	{
		DownHitReactPhase = EPRPlayerDownHitReactPhase::GetUp;
	}
}

// GameplayEvent 태그를 피격 리액션 타입으로 변환한다.
EPRPlayerHitReactType UPRGA_PlayerHitReact::ResolveHitReactType(const FGameplayEventData* TriggerEventData) const
{
	if (TriggerEventData == nullptr)
	{
		return EPRPlayerHitReactType::None;
	}

	if (TriggerEventData->EventTag.MatchesTagExact(PRGameplayTags::Event_Ability_PlayerHitReact_Down))
	{
		return EPRPlayerHitReactType::Down;
	}

	if (TriggerEventData->EventTag.MatchesTagExact(PRGameplayTags::Event_Ability_PlayerHitReact_Strong))
	{
		return EPRPlayerHitReactType::Strong;
	}

	if (TriggerEventData->EventTag.MatchesTagExact(PRGameplayTags::Event_Ability_PlayerHitReact_Weak))
	{
		return EPRPlayerHitReactType::Weak;
	}

	return EPRPlayerHitReactType::None;
}

// 피격 리액션 타입에 맞는 몽타주 선택 함수로 분기한다.
UAnimMontage* UPRGA_PlayerHitReact::SelectHitReactMontage(EPRPlayerHitReactType HitReactType) const
{
	if (HitReactType == EPRPlayerHitReactType::Weak)
	{
		return SelectWeakHitReactMontage();
	}

	if (HitReactType == EPRPlayerHitReactType::Strong)
	{
		return SelectStrongHitReactMontage();
	}

	if (HitReactType == EPRPlayerHitReactType::Down)
	{
		return DownHitReactMontage.Get();
	}

	return nullptr;
}

// 현재 무기 타입에 맞는 약한 경직 몽타주를 선택한다.
UAnimMontage* UPRGA_PlayerHitReact::SelectWeakHitReactMontage() const
{
	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PlayerCharacter))
	{
		return UnarmedWeakHitReactMontage.Get();
	}

	const UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager();
	if (!IsValid(WeaponManager)
		|| WeaponManager->GetArmedState() == EPRArmedState::Unarmed
		|| WeaponManager->GetCurrentWeaponSlot() == EPRWeaponSlotType::None)
	{
		return UnarmedWeakHitReactMontage.Get();
	}

	const FPRWeaponVisualInfo& CurrentWeaponVisualInfo = WeaponManager->GetCurrentWeaponVisualInfo();
	const UPRWeaponDataAsset* WeaponData = CurrentWeaponVisualInfo.WeaponData.Get();
	if (!IsValid(WeaponData))
	{
		WeaponData = WeaponManager->GetWeaponDataBySlotType(WeaponManager->GetCurrentWeaponSlot());
	}

	const EPRWeaponType WeaponType = IsValid(WeaponData) ? WeaponData->WeaponType : EPRWeaponType::None;
	if (const TObjectPtr<UAnimMontage>* FoundMontage = WeakHitReactMontages.Find(WeaponType))
	{
		UAnimMontage* SelectedMontage = FoundMontage->Get();
		if (IsValid(SelectedMontage))
		{
			return SelectedMontage;
		}
	}

	return UnarmedWeakHitReactMontage.Get();
}

// 현재 무기 타입에 맞는 강한 경직 몽타주를 선택한다.
UAnimMontage* UPRGA_PlayerHitReact::SelectStrongHitReactMontage() const
{
	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PlayerCharacter))
	{
		return UnarmedStrongHitReactMontage.Get();
	}

	const UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager();
	if (!IsValid(WeaponManager)
		|| WeaponManager->GetArmedState() == EPRArmedState::Unarmed
		|| WeaponManager->GetCurrentWeaponSlot() == EPRWeaponSlotType::None)
	{
		return UnarmedStrongHitReactMontage.Get();
	}

	const FPRWeaponVisualInfo& CurrentWeaponVisualInfo = WeaponManager->GetCurrentWeaponVisualInfo();
	const UPRWeaponDataAsset* WeaponData = CurrentWeaponVisualInfo.WeaponData.Get();
	if (!IsValid(WeaponData))
	{
		WeaponData = WeaponManager->GetWeaponDataBySlotType(WeaponManager->GetCurrentWeaponSlot());
	}

	const EPRWeaponType WeaponType = IsValid(WeaponData) ? WeaponData->WeaponType : EPRWeaponType::None;
	if (const TObjectPtr<UAnimMontage>* FoundMontage = StrongHitReactMontages.Find(WeaponType))
	{
		UAnimMontage* SelectedMontage = FoundMontage->Get();
		if (IsValid(SelectedMontage))
		{
			return SelectedMontage;
		}
	}

	return UnarmedStrongHitReactMontage.Get();
}

// 피격 리액션 타입에 따라 현재 진행 중인 플레이어 행동 Ability를 취소한다.
void UPRGA_PlayerHitReact::CancelActionsForHitReact(EPRPlayerHitReactType HitReactType)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return;
	}

	FGameplayTagContainer CancelTags;
	CancelTags.AddTag(PRGameplayTags::Ability_Player_Reload);
	CancelTags.AddTag(PRGameplayTags::Ability_Player_Weapon_Zoom);

	if (HitReactType == EPRPlayerHitReactType::Strong || HitReactType == EPRPlayerHitReactType::Down)
	{
		CancelTags.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
		CancelTags.AddTag(PRGameplayTags::Ability_Player_Aim);
		CancelTags.AddTag(PRGameplayTags::Ability_Player_Crouch);
		CancelTags.AddTag(PRGameplayTags::Ability_Player_Dodge);
		CancelTags.AddTag(PRGameplayTags::Ability_Player_Sprint);
		CancelTags.AddTag(PRGameplayTags::Ability_Player_Interaction);
	}

	ASC->CancelAbilities(&CancelTags, nullptr, this);
}

// 회복 가능 체력 자동 회복 딜레이 GameplayEffect를 적용한다.
void UPRGA_PlayerHitReact::ApplyRecoverableHealthRecoveryDelay(const FGameplayEventData* TriggerEventData)
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	if (!IsValid(RecoverableHealthRecoveryDelayEffectClass))
	{
		return;
	}

	if (TriggerEventData == nullptr || TriggerEventData->EventMagnitude <= 0.0f)
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(RecoverableHealthRecoveryDelayEffectClass);
	if (SpecHandle.IsValid())
	{
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
	}
}

// 다운 리액션의 섹션 상태와 착지 요청 상태를 초기화한다.
void UPRGA_PlayerHitReact::StartDownHitReact()
{
	DownHitReactPhase = EPRPlayerDownHitReactPhase::Start;
	bDownLandRequested = false;
	StartDownHitReactCooldown();
}

// 다운 리액션이 이미 재생 중일 때 같은 Ability가 다시 발동하지 않도록 쿨다운 태그를 부여한다.
void UPRGA_PlayerHitReact::StartDownHitReactCooldown()
{
	if (bDownHitReactCooldownTagAdded)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (IsValid(ASC))
	{
		ASC->AddLooseGameplayTag(PRGameplayTags::Cooldown_Ability_PlayerHitReact);
		bDownHitReactCooldownTagAdded = true;
	}
}

// 다운 리액션 몽타주가 끝나거나 취소되면 HitReact 쿨다운 태그를 제거한다.
void UPRGA_PlayerHitReact::ClearDownHitReactCooldown()
{
	if (!bDownHitReactCooldownTagAdded)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (IsValid(ASC))
	{
		ASC->RemoveLooseGameplayTag(PRGameplayTags::Cooldown_Ability_PlayerHitReact);
	}

	bDownHitReactCooldownTagAdded = false;
}

// 다운 몽타주의 섹션 연결과 섹션 변경 콜백을 설정한다.
void UPRGA_PlayerHitReact::ConfigureDownMontageFlow()
{
	if (ActiveHitReactType != EPRPlayerHitReactType::Down)
	{
		return;
	}

	APRPlayerCharacter* PlayerCharacter = ActivePlayerCharacter.Get();
	if (!IsValid(PlayerCharacter) || !IsValid(ActiveHitReactMontage))
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = PlayerCharacter->GetMesh();
	if (!IsValid(MeshComponent))
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!IsValid(AnimInstance))
	{
		return;
	}

	RefreshDownMontageSectionFlow();

	FOnMontageSectionChanged SectionChangedDelegate;
	SectionChangedDelegate.BindUObject(this, &UPRGA_PlayerHitReact::HandleDownMontageSectionChanged);
	AnimInstance->Montage_SetSectionChangedDelegate(SectionChangedDelegate, ActiveHitReactMontage);
}

// 다운 몽타주의 다음 섹션 연결을 현재 착지 요청 상태에 맞게 갱신한다.
void UPRGA_PlayerHitReact::RefreshDownMontageSectionFlow()
{
	APRPlayerCharacter* PlayerCharacter = ActivePlayerCharacter.Get();
	if (!IsValid(PlayerCharacter) || !IsValid(ActiveHitReactMontage))
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = PlayerCharacter->GetMesh();
	if (!IsValid(MeshComponent))
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!IsValid(AnimInstance))
	{
		return;
	}

	// Start는 항상 FallLoop로 보낸다. 착지 판정은 FallLoop에 들어간 뒤부터만 처리한다.
	const FName FallLoopNextSectionName = bDownLandRequested ? DownLandSectionName : DownFallLoopSectionName;
	AnimInstance->Montage_SetNextSection(DownStartSectionName, DownFallLoopSectionName, ActiveHitReactMontage);
	AnimInstance->Montage_SetNextSection(DownFallLoopSectionName, FallLoopNextSectionName, ActiveHitReactMontage);
	AnimInstance->Montage_SetNextSection(DownLandSectionName, DownGetUpSectionName, ActiveHitReactMontage);
	AnimInstance->Montage_SetNextSection(DownGetUpSectionName, NAME_None, ActiveHitReactMontage);
}

// FallLoop 구간에서 캡슐 기준 바닥 감지 타이머를 시작한다.
void UPRGA_PlayerHitReact::StartDownGroundCheck()
{
	APRPlayerCharacter* PlayerCharacter = ActivePlayerCharacter.Get();
	if (!IsValid(PlayerCharacter))
	{
		return;
	}

	UWorld* World = PlayerCharacter->GetWorld();
	if (!IsValid(World))
	{
		return;
	}
	if (World->GetTimerManager().IsTimerActive(DownGroundCheckTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		DownGroundCheckTimerHandle,
		this,
		&UPRGA_PlayerHitReact::CheckDownGround,
		DownGroundCheckInterval,
		true);
	CheckDownGround();
}

// 다운 바닥 감지 타이머를 중지하고 핸들을 무효화한다.
void UPRGA_PlayerHitReact::StopDownGroundCheck()
{
	APRPlayerCharacter* PlayerCharacter = ActivePlayerCharacter.Get();
	UWorld* World = IsValid(PlayerCharacter) ? PlayerCharacter->GetWorld() : GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(DownGroundCheckTimerHandle);
	}
	DownGroundCheckTimerHandle.Invalidate();
}

// 다운 몽타주 재생 중 바닥 감지 결과에 따라 Land 진입을 요청한다.
void UPRGA_PlayerHitReact::CheckDownGround()
{
	if (ActiveHitReactType != EPRPlayerHitReactType::Down || bHitReactFinished)
	{
		StopDownGroundCheck();
		return;
	}

	if (DownHitReactPhase == EPRPlayerDownHitReactPhase::Land
		|| DownHitReactPhase == EPRPlayerDownHitReactPhase::GetUp)
	{
		StopDownGroundCheck();
		return;
	}

	if (!IsDownMontagePlaying())
	{
		return;
	}

	// 바닥이 확인되는 순간 다음 섹션 연결을 Land로 바꾸고, FallLoop 중이면 즉시 Land로 점프한다.
	if (HasGroundBelowForDownHitReact())
	{
		RequestDownLand();
	}
}

// 현재 다운 리액션 몽타주가 재생 중인지 확인한다.
bool UPRGA_PlayerHitReact::IsDownMontagePlaying() const
{
	const APRPlayerCharacter* PlayerCharacter = ActivePlayerCharacter.Get();
	if (!IsValid(PlayerCharacter) || !IsValid(ActiveHitReactMontage))
	{
		return false;
	}

	const USkeletalMeshComponent* MeshComponent = PlayerCharacter->GetMesh();
	if (!IsValid(MeshComponent))
	{
		return false;
	}

	const UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	return IsValid(AnimInstance) && AnimInstance->Montage_IsPlaying(ActiveHitReactMontage);
}

// 플레이어 캡슐 하단 기준으로 착지 가능한 바닥이 있는지 Sweep으로 확인한다.
bool UPRGA_PlayerHitReact::HasGroundBelowForDownHitReact() const
{
	const APRPlayerCharacter* PlayerCharacter = ActivePlayerCharacter.Get();
	if (!IsValid(PlayerCharacter))
	{
		return false;
	}

	const UCapsuleComponent* CapsuleComponent = PlayerCharacter->GetCapsuleComponent();
	if (!IsValid(CapsuleComponent))
	{
		return false;
	}

	UWorld* World = PlayerCharacter->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const float CapsuleHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
	const float CapsuleRadius = CapsuleComponent->GetScaledCapsuleRadius();
	const float ProbeRadius = FMath::Min(FMath::Max(CapsuleRadius * 0.5f, 5.0f), CapsuleHalfHeight);
	const FVector ProbeStart = PlayerCharacter->GetActorLocation() - FVector(0.0f, 0.0f, CapsuleHalfHeight - ProbeRadius);
	const FVector ProbeEnd = ProbeStart - FVector(0.0f, 0.0f, DownGroundCheckDistance);

	// 루트모션 중 메시 위치가 흔들려도 캡슐 하단 기준으로 바닥을 확인한다.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRDownHitReactGroundCheck), false);
	QueryParams.AddIgnoredActor(PlayerCharacter);
	QueryParams.bFindInitialOverlaps = true;

	FHitResult HitResult;
	const bool bHit = World->SweepSingleByChannel(
		HitResult,
		ProbeStart,
		ProbeEnd,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(ProbeRadius),
		QueryParams);
	return bHit && HitResult.bBlockingHit && HitResult.ImpactNormal.Z >= DownGroundWalkableNormalZ;
}

// 다운 몽타주가 Land 섹션으로 넘어가도록 요청하고 필요 시 즉시 점프한다.
void UPRGA_PlayerHitReact::RequestDownLand()
{
	if (ActiveHitReactType != EPRPlayerHitReactType::Down || bHitReactFinished)
	{
		return;
	}

	// 요청 플래그를 먼저 세워 이후 섹션 갱신에서도 Land 연결이 유지되게 한다.
	bDownLandRequested = true;
	RefreshDownMontageSectionFlow();
	if (DownHitReactPhase == EPRPlayerDownHitReactPhase::FallLoop)
	{
		JumpToDownSection(DownLandSectionName);
	}
}

// 다운 리액션 전용 타이머, 섹션 콜백, 런타임 상태를 정리한다.
void UPRGA_PlayerHitReact::ClearDownHitReact()
{
	StopDownGroundCheck();

	APRPlayerCharacter* PlayerCharacter = ActivePlayerCharacter.Get();
	if (IsValid(PlayerCharacter) && IsValid(ActiveHitReactMontage))
	{
		USkeletalMeshComponent* MeshComponent = PlayerCharacter->GetMesh();
		if (IsValid(MeshComponent))
		{
			UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
			if (IsValid(AnimInstance))
			{
				FOnMontageSectionChanged EmptyDelegate;
				AnimInstance->Montage_SetSectionChangedDelegate(EmptyDelegate, ActiveHitReactMontage);
			}
		}
	}

	ActivePlayerCharacter.Reset();
	ActiveHitReactType = EPRPlayerHitReactType::None;
	DownHitReactPhase = EPRPlayerDownHitReactPhase::None;
	bDownLandRequested = false;
}

// 현재 다운 몽타주를 지정한 섹션으로 이동시킨다.
void UPRGA_PlayerHitReact::JumpToDownSection(FName SectionName)
{
	APRPlayerCharacter* PlayerCharacter = ActivePlayerCharacter.Get();
	if (!IsValid(PlayerCharacter) || !IsValid(ActiveHitReactMontage) || SectionName.IsNone())
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = PlayerCharacter->GetMesh();
	if (!IsValid(MeshComponent))
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (IsValid(AnimInstance))
	{
		AnimInstance->Montage_JumpToSection(SectionName, ActiveHitReactMontage);
	}
}

// 다운 몽타주가 Start, FallLoop, Land, GetUp 섹션을 모두 갖고 있는지 확인한다.
bool UPRGA_PlayerHitReact::HasRequiredDownMontageSections(const UAnimMontage* Montage) const
{
	return IsValid(Montage)
		&& Montage->GetSectionIndex(DownStartSectionName) != INDEX_NONE
		&& Montage->GetSectionIndex(DownFallLoopSectionName) != INDEX_NONE
		&& Montage->GetSectionIndex(DownLandSectionName) != INDEX_NONE
		&& Montage->GetSectionIndex(DownGetUpSectionName) != INDEX_NONE;
}

// 강한 경직과 다운 동안 행동불능 상태 태그를 부여한다.
void UPRGA_PlayerHitReact::StartActionLock(EPRPlayerHitReactType HitReactType)
{
	if (HitReactType != EPRPlayerHitReactType::Strong && HitReactType != EPRPlayerHitReactType::Down)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (IsValid(ASC))
	{
		ASC->AddLooseGameplayTag(PRGameplayTags::State_PlayerHitReactLocked);

		const AActor* AvatarActor = GetAvatarActorFromActorInfo();
		if (IsValid(AvatarActor) && AvatarActor->HasAuthority())
		{
			ASC->AddReplicatedLooseGameplayTag(PRGameplayTags::State_PlayerHitReactLocked);
		}
		FGameplayTagContainer BlockedTags;
		BlockedTags.AddTag(PRGameplayTags::Ability_Player_Dodge);
		ASC->BlockAbilitiesWithTags(BlockedTags);
		
		bActionLockTagAdded = true;
		bActionLockAbilityBlockApplied = true;
	}
}

// 이 Ability가 부여한 행동불능 상태 태그를 제거한다.
void UPRGA_PlayerHitReact::ClearActionLock()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (bActionLockAbilityBlockApplied && IsValid(ASC))
	{
		FGameplayTagContainer BlockedTags;
		BlockedTags.AddTag(PRGameplayTags::Ability_Player_Dodge);
		ASC->UnBlockAbilitiesWithTags(BlockedTags);
		bActionLockAbilityBlockApplied = false;
	}
	
	if (!bActionLockTagAdded)
	{
		return;
	}
	
	if (IsValid(ASC))
	{
		ASC->RemoveLooseGameplayTag(PRGameplayTags::State_PlayerHitReactLocked);

		const AActor* AvatarActor = GetAvatarActorFromActorInfo();
		if (IsValid(AvatarActor) && AvatarActor->HasAuthority())
		{
			ASC->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_PlayerHitReactLocked);
		}
	}

	bActionLockTagAdded = false;
}

// 피격 리액션 Ability 종료를 한 번만 실행하도록 보호한다.
void UPRGA_PlayerHitReact::FinishHitReact(bool bWasCancelled)
{
	if (bHitReactFinished)
	{
		return;
	}

	bHitReactFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}
