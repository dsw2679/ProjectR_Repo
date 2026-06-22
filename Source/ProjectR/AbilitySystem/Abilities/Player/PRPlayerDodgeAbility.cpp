// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (회피 시 스태미나 소모 및 무적 프레임, 구르기 애니메이션 구현)
// Author: 배유찬 (회피 중 플래시라이트 상태 연동)
#include "ProjectR/AbilitySystem/Abilities/Player/PRPlayerDodgeAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Player/PRCameraModifier.h"
#include "ProjectR/Player/Components/PRActionInputRouterComponent.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "TimerManager.h"

/*~ 초기화 ~*/

UPRPlayerDodgeAbility::UPRPlayerDodgeAbility()
{
	// 회피 Ability가 활성화되는 동안 GAS 태그로 다른 플레이어 액션을 취소하고 차단한다.
	AbilityTags.AddTag(PRGameplayTags::Ability_Player_Dodge);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_Dodging);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_StaminaDepleted);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dodging);
	InputTag = PRGameplayTags::Input_Ability_Dodge;

	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Aim);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Crouch);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Sprint);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Interaction);
	CancelAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_UseConsumable);

	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Aim);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Crouch);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Dodge);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Sprint);

	// 클라이언트 예측으로 몽타주와 루트모션을 즉시 재생하고, 서버에는 발동 순간의 방향만 한 번 전달한다.
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

/*~ UGameplayAbility Interface ~*/

void UPRPlayerDodgeAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!IsValid(Character))
	{
		// Avatar가 캐릭터가 아니면 입력 방향과 전방 벡터를 계산할 수 없어 Ability를 취소한다.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (IsLocallyControlled())
	{
		// 발동 순간의 이동 입력만 샘플링해서 전방 구르기와 백스텝을 결정한다.
		const FVector InputDirection = Character->GetLastMovementInputVector();
		const bool bHasMovementInput = !InputDirection.IsNearlyZero();
		const FVector FinalDirection = bHasMovementInput
			? InputDirection.GetSafeNormal()
			: -Character->GetActorForwardVector();

		if (HasAuthority(&ActivationInfo))
		{
			// 리슨 서버 로컬 플레이어는 RPC 없이 같은 값을 서버 실행 경로에 저장한다.
			bServerReceivedDirection = true;
			SavedDirection = FinalDirection;
			bSavedHasMovementInput = bHasMovementInput;
		}
		else
		{
			// 원격 클라이언트는 회피 발동 순간의 방향과 입력 여부만 서버로 한 번 전송한다.
			Server_SendDodgeDirection(FinalDirection, bHasMovementInput);
		}

		ExecuteDodge(FinalDirection, bHasMovementInput);
	}
	else if (bServerReceivedDirection)
	{
		// 서버 Ability가 RPC보다 늦게 활성화된 경우 저장된 요청 값으로 실행한다.
		ExecuteDodge(SavedDirection, bSavedHasMovementInput);
	}
}

void UPRPlayerDodgeAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bDodgeEndRequested = true;

	if (APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UPRActionInputRouterComponent* ActionInputRouter = PlayerCharacter->GetActionInputRouter())
		{
			ActionInputRouter->UnregisterInputConsumer(this);
		}
	}

	ClearDodgeTimers();
	EndIFrame();
	StopDodgeMontage(DodgeMontageStopBlendOutTime);

	if (IsValid(ActiveMontageTask))
	{
		// 살아있는 몽타주 태스크가 남아 있으면 종료 콜백이 뒤늦게 재진입할 수 있어 정리한다.
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	if (IsValid(ActiveCameraModifier))
	{
		// 즉시 제거하지 않고 모디파이어의 AlphaOut 설정에 따라 자연스럽게 복구한다.
		ActiveCameraModifier->DisableModifier(false);
		ActiveCameraModifier = nullptr;
	}

	if (bDodgeStarted)
	{
		ApplyStaminaRegenDelay();
	}

	bServerReceivedDirection = false;
	SavedDirection = FVector::ZeroVector;
	bSavedHasMovementInput = false;
	bDodgeStarted = false;
	bCanCancelDodgeByInput = false;
	CurrentDodgeInputCancelBlendOutTime = 0.0f;
	ActiveDodgeExecutionId = 0;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

/*~ 입력 스킵 처리 ~*/

bool UPRPlayerDodgeAbility::HandleRoutedActionInput()
{
	if (!IsActive())
	{
		// 이미 종료된 Ability에는 라우팅된 입력을 소비하지 않는다.
		return false;
	}

	if (!bCanCancelDodgeByInput)
	{
		// 회피 중에는 입력을 행동으로 넘기지 않고 소비한다.
		return true;
	}

	// 노티파이 스테이트가 연 구간에서는 입력 한 번으로 몽타주와 Ability를 함께 조기 종료한다.
	bCanCancelDodgeByInput = false;

	const float BlendOutTime = CurrentDodgeInputCancelBlendOutTime > 0.0f
		? CurrentDodgeInputCancelBlendOutTime
		: DodgeInputCancelBlendOutTime;
	bDodgeEndRequested = true;
	StopDodgeMontage(BlendOutTime);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	return true;
}

void UPRPlayerDodgeAbility::SetRoutedInputCancelWindow(bool bCanCancel, float BlendOutTime)
{
	if (!IsActive() || !bCanCancel)
	{
		// Ability가 비활성 상태이거나 닫힘 요청이면 입력 스킵 구간을 해제한다.
		bCanCancelDodgeByInput = false;
		CurrentDodgeInputCancelBlendOutTime = 0.0f;
		return;
	}

	bCanCancelDodgeByInput = true;
	CurrentDodgeInputCancelBlendOutTime = BlendOutTime > 0.0f ? BlendOutTime : DodgeInputCancelBlendOutTime;
}

/*~ 서버 요청 수신 ~*/

void UPRPlayerDodgeAbility::Server_SendDodgeDirection_Implementation(FVector_NetQuantize Direction, bool bHasMovementInput)
{
	// 클라이언트 요청이 서버 실행 경로가 사용할 수 있도록 방향 데이터를 저장한다.
	bServerReceivedDirection = true;
	SavedDirection = Direction;
	bSavedHasMovementInput = bHasMovementInput;

	if (IsActive())
	{
		// 서버 Ability가 이미 활성화되어 대기 중이라면 RPC 수신 시점에 바로 실행한다.
		ExecuteDodge(Direction, bHasMovementInput);
	}
}

/*~ 몽타주 종료 처리 ~*/

void UPRPlayerDodgeAbility::HandleDodgeMontageCompleted()
{
	FinishDodge(false);
}

void UPRPlayerDodgeAbility::HandleDodgeMontageInterrupted()
{
	FinishDodge(true);
}

void UPRPlayerDodgeAbility::HandleDodgeSafetyTimeout(int32 FinishedDodgeExecutionId)
{
	if (FinishedDodgeExecutionId != ActiveDodgeExecutionId)
	{
		// 연속 회피 중 이전 실행의 타이머가 늦게 도착하면 현재 회피를 건드리지 않는다.
		return;
	}

	FinishDodge(false);
}

void UPRPlayerDodgeAbility::FinishDodge(bool bWasCancelled)
{
	if (bDodgeEndRequested || !IsActive())
	{
		// 여러 콜백이 동시에 도착해도 EndAbility는 한 번만 호출한다.
		return;
	}

	bDodgeEndRequested = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

/*~ 무적 프레임 ~*/

void UPRPlayerDodgeAbility::HandleIFrameStartTimer(int32 FinishedDodgeExecutionId)
{
	if (FinishedDodgeExecutionId != ActiveDodgeExecutionId || bDodgeEndRequested || !IsActive())
	{
		// 이전 회피의 타이머이거나 이미 종료된 실행이면 무적 시작을 무시한다.
		return;
	}

	StartIFrame();

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		// 월드가 없으면 타이머를 예약할 수 없으므로 무적 종료 예약을 건너뛴다.
		return;
	}

	float RateScale = FMath::Clamp(DodgeMontagePlayRate, 0.3f, 2.0f); 
	float FinalIFameDuration = IFrameDuration / RateScale;
	
	FTimerDelegate IFrameEndDelegate;
	IFrameEndDelegate.BindUObject(this, &UPRPlayerDodgeAbility::HandleIFrameEndTimer, FinishedDodgeExecutionId);
	World->GetTimerManager().SetTimer(IFrameEndTimerHandle, IFrameEndDelegate, FinalIFameDuration, false);
}

void UPRPlayerDodgeAbility::HandleIFrameEndTimer(int32 FinishedDodgeExecutionId)
{
	if (FinishedDodgeExecutionId != ActiveDodgeExecutionId)
	{
		return;
	}

	EndIFrame();
}

void UPRPlayerDodgeAbility::StartIFrame()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (ActorInfo == nullptr || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		// ActorInfo는 구조체 포인터이고 ASC는 약참조라 각각 null과 핸들 유효성을 확인한다.
		return;
	}

	if (IsValid(InvulnerableEffectClass))
	{
		// 무적 GE가 BP에서 할당된 경우에만 무적 상태를 적용한다.
		const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(InvulnerableEffectClass);
		InvulnerableEffectHandle = ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
	}
}

void UPRPlayerDodgeAbility::EndIFrame()
{
	if (InvulnerableEffectHandle.IsValid())
	{
		// 적용된 무적 GE 핸들이 있을 때만 제거를 요청한다.
		BP_RemoveGameplayEffectFromOwnerWithHandle(InvulnerableEffectHandle);
		InvulnerableEffectHandle.Invalidate();
	}
}

void UPRPlayerDodgeAbility::ClearDodgeTimers()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		// 월드가 없으면 타이머 매니저도 사용할 수 없으므로 정리할 타이머가 없다.
		return;
	}

	World->GetTimerManager().ClearTimer(DodgeEndTimerHandle);
	World->GetTimerManager().ClearTimer(IFrameStartTimerHandle);
	World->GetTimerManager().ClearTimer(IFrameEndTimerHandle);
}

/*~ 회피 실행 ~*/

void UPRPlayerDodgeAbility::ExecuteDodge(FVector Direction, bool bHasMovementInput)
{
	APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Character))
	{
		// 회피 실행에는 플레이어 캐릭터의 회전, 입력 라우터, 무장 정보가 필요하다.
		return;
	}

	const FVector SafeDirection = Direction.IsNearlyZero()
		? (bHasMovementInput ? Character->GetActorForwardVector() : -Character->GetActorForwardVector())
		: Direction.GetSafeNormal();
	const EPRDodgeAnimationType DodgeAnimationType = bHasMovementInput
		? EPRDodgeAnimationType::ForwardRoll
		: EPRDodgeAnimationType::BackStep;
	bDodgeEndRequested = false;
	bDodgeStarted = true;
	bCanCancelDodgeByInput = false;
	CurrentDodgeInputCancelBlendOutTime = 0.0f;
	ClearDodgeTimers();
	EndIFrame();

	// 회피마다 실행 번호를 갱신해 이전 회피의 타이머 콜백과 현재 회피를 구분한다.
	++DodgeExecutionId;
	ActiveDodgeExecutionId = DodgeExecutionId;

	if (bHasMovementInput)
	{
		// 방향 입력 회피는 캐릭터를 입력 방향으로 돌린 뒤 전방 구르기 몽타주만 재생한다.
		Character->SetActorRotation(SafeDirection.Rotation());
	}

	if (UPRActionInputRouterComponent* ActionInputRouter = Character->GetActionInputRouter())
	{
		ActionInputRouter->RegisterInputConsumer(this);
	}
	PlayDodgeMontage(SelectDodgeMontage(Character, DodgeAnimationType));

	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		// 월드가 유효할 때만 안전 종료와 무적 프레임 타이머를 등록한다.
		// 몽타주 콜백이 누락되어도 Ability가 남지 않도록 안전 종료 타이머를 둔다.
		FTimerDelegate DodgeEndDelegate;
		DodgeEndDelegate.BindUObject(this, &UPRPlayerDodgeAbility::HandleDodgeSafetyTimeout, ActiveDodgeExecutionId);
		World->GetTimerManager().SetTimer(DodgeEndTimerHandle, DodgeEndDelegate, DodgeDuration, false);

		// 무적 프레임도 실행 번호를 물고 시작하므로 오래된 타이머를 무시할 수 있다.
		FTimerDelegate IFrameStartDelegate;
		IFrameStartDelegate.BindUObject(this, &UPRPlayerDodgeAbility::HandleIFrameStartTimer, ActiveDodgeExecutionId);
		World->GetTimerManager().SetTimer(IFrameStartTimerHandle, IFrameStartDelegate, IFrameStartDelay, false);
	}

	if (IsLocallyControlled())
	{
		// 카메라 피드백은 로컬 소유 플레이어에게만 적용한다.
		APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
		if (IsValid(PlayerController) && IsValid(PlayerController->PlayerCameraManager))
		{
			// 로컬 컨트롤러와 CameraManager가 모두 유효할 때만 카메라 피드백을 추가한다.
			ActiveCameraModifier = Cast<UPRCameraModifier>(
				PlayerController->PlayerCameraManager->AddNewCameraModifier(UPRCameraModifier::StaticClass())
			);

			if (IsValid(ActiveCameraModifier))
			{
				// 모디파이어 생성에 성공한 경우에만 액션 카메라 값을 설정한다.
				ActiveCameraModifier->SetActionCameraSettings(110.0f, FVector::ZeroVector);
			}
		}
	}
}

UAnimMontage* UPRPlayerDodgeAbility::SelectDodgeMontage(const APRPlayerCharacter* Character, EPRDodgeAnimationType AnimationType) const
{
	const bool bUseForwardRoll = AnimationType == EPRDodgeAnimationType::ForwardRoll;
	if (!IsValid(Character))
	{
		// 캐릭터가 없으면 무장 상태를 알 수 없으므로 맨손 몽타주를 기본값으로 사용한다.
		return bUseForwardRoll ? UnarmedForwardRollMontage.Get() : UnarmedBackStepMontage.Get();
	}

	const UPRWeaponManagerComponent* WeaponManager = Character->GetWeaponManager();
	if (!IsValid(WeaponManager)
		|| WeaponManager->GetArmedState() == EPRArmedState::Unarmed
		|| WeaponManager->GetCurrentWeaponSlot() == EPRWeaponSlotType::None)
	{
		// 무기 매니저가 없거나 무장하지 않은 상태면 맨손 회피 몽타주를 사용한다.
		return bUseForwardRoll ? UnarmedForwardRollMontage.Get() : UnarmedBackStepMontage.Get();
	}

	if (WeaponManager->GetCurrentWeaponSlot() == EPRWeaponSlotType::Primary)
	{
		UAnimMontage* SelectedMontage = bUseForwardRoll ? PrimaryForwardRollMontage.Get() : PrimaryBackStepMontage.Get();
		return IsValid(SelectedMontage) ? SelectedMontage : (bUseForwardRoll ? UnarmedForwardRollMontage.Get() : UnarmedBackStepMontage.Get());
	}

	if (WeaponManager->GetCurrentWeaponSlot() == EPRWeaponSlotType::Secondary)
	{
		UAnimMontage* SelectedMontage = bUseForwardRoll ? SecondaryForwardRollMontage.Get() : SecondaryBackStepMontage.Get();
		return IsValid(SelectedMontage) ? SelectedMontage : (bUseForwardRoll ? UnarmedForwardRollMontage.Get() : UnarmedBackStepMontage.Get());
	}

	return bUseForwardRoll ? UnarmedForwardRollMontage.Get() : UnarmedBackStepMontage.Get();
}

void UPRPlayerDodgeAbility::PlayDodgeMontage(UAnimMontage* DodgeMontage)
{
	if (!IsValid(DodgeMontage))
	{
		// 선택된 몽타주가 없으면 회피를 정상 재생할 수 없으므로 취소 종료한다.
		FinishDodge(true);
		return;
	}

	ActiveDodgeMontage = DodgeMontage;
	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		DodgeMontage,
		FMath::Max(DodgeMontagePlayRate, UE_SMALL_NUMBER));

	if (!IsValid(ActiveMontageTask))
	{
		// GAS 몽타주 태스크 생성 실패 시 콜백을 받을 수 없으므로 취소 종료한다.
		FinishDodge(true);
		return;
	}

	// 몽타주 종료 콜백은 회피 Ability 종료의 주 경로다.
	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRPlayerDodgeAbility::HandleDodgeMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UPRPlayerDodgeAbility::HandleDodgeMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRPlayerDodgeAbility::HandleDodgeMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRPlayerDodgeAbility::HandleDodgeMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
}

void UPRPlayerDodgeAbility::StopDodgeMontage(float BlendOutTime)
{
	if (!IsValid(ActiveDodgeMontage))
	{
		// 현재 Ability가 재생 중인 몽타주가 없으면 정지 요청을 건너뛴다.
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->CurrentMontageStop(BlendOutTime);
	}

	ActiveDodgeMontage = nullptr;
}

/*~ 스태미너 ~*/

void UPRPlayerDodgeAbility::ApplyStaminaRegenDelay()
{
	if (!IsValid(StaminaRegenDelayEffectClass))
	{
		// BP에서 회복 딜레이 GE가 할당되지 않은 경우에는 딜레이 적용 없이 종료한다.
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(StaminaRegenDelayEffectClass);
	if (SpecHandle.IsValid())
	{
		// 유효한 Spec이 만들어진 경우에만 Owner ASC에 회복 딜레이 GE를 적용한다.
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
	}
}
