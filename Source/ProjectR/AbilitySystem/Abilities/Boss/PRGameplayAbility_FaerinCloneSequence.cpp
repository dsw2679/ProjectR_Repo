// Copyright ProjectR. All Rights Reserved.

#include "PRGameplayAbility_FaerinCloneSequence.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NiagaraSystem.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRBossAbilityTagUtils.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacter.h"
#include "ProjectR/Character/Enemy/Boss/PRFaerinCharacterEventRouterComponent.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"

UPRGameplayAbility_FaerinCloneSequence::UPRGameplayAbility_FaerinCloneSequence()
{
	SetAssetTags(PRBossAbility::MakePatternAssetTags(PRGameplayTags::Ability_Boss_Faerin_CloneSequence));

	SpawnOffsets.Add(FVector(350.0f, 260.0f, 0.0f));
	SpawnOffsets.Add(FVector(350.0f, -260.0f, 0.0f));
}

// ===== Ability 생명주기 =====

void UPRGameplayAbility_FaerinCloneSequence::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	APRFaerinCharacter* Faerin = nullptr;
	AActor* PrimaryTarget = nullptr;
	if (!InitializeCloneSequence(Faerin, PrimaryTarget) || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	BeginBossPatternAttackCommit();
	bPatternAttackCommitted = true;

	TArray<AActor*> AlivePlayers;
	GatherAlivePlayers(AlivePlayers);

	TArray<AActor*> CloneTargets;
	ResolveCloneTargets(PrimaryTarget, AlivePlayers, CloneTargets);
	PendingCloneTargets.Reset();
	for (AActor* CloneTarget : CloneTargets)
	{
		if (IsAlivePlayerTarget(CloneTarget))
		{
			PendingCloneTargets.Add(CloneTarget);
		}
	}
	if (PendingCloneTargets.IsEmpty()
		|| !IsValid(SummonMontage)
		|| SummonCharacterEventName == NAME_None
		|| !RegisterCharacterEventListener())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveFaerin = Faerin;
	bCloneActorsSpawned = false;
	ActiveSummonMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		SummonMontage,
		FMath::Max(SummonMontagePlayRate, UE_SMALL_NUMBER),
		SummonMontageStartSection);
	if (!IsValid(ActiveSummonMontageTask))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveSummonMontageTask->OnCompleted.AddDynamic(this, &UPRGameplayAbility_FaerinCloneSequence::HandleSummonMontageCompleted);
	ActiveSummonMontageTask->OnInterrupted.AddDynamic(this, &UPRGameplayAbility_FaerinCloneSequence::HandleSummonMontageInterrupted);
	ActiveSummonMontageTask->OnCancelled.AddDynamic(this, &UPRGameplayAbility_FaerinCloneSequence::HandleSummonMontageInterrupted);
	ActiveSummonMontageTask->ReadyForActivation();
}

void UPRGameplayAbility_FaerinCloneSequence::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (bPatternAttackCommitted)
	{
		EndBossPatternAttackCommit();
		bPatternAttackCommitted = false;
	}

	UnregisterCharacterEventListener();
	if (IsValid(ActiveSummonMontageTask))
	{
		ActiveSummonMontageTask->EndTask();
		ActiveSummonMontageTask = nullptr;
	}
	PendingCloneTargets.Reset();
	ActiveFaerin = nullptr;
	bCloneActorsSpawned = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ===== 초기화/타겟 선정 =====

bool UPRGameplayAbility_FaerinCloneSequence::InitializeCloneSequence(
	APRFaerinCharacter*& OutFaerin,
	AActor*& OutPrimaryTarget) const
{
	OutFaerin = Cast<APRFaerinCharacter>(GetBossAvatarCharacter());
	OutPrimaryTarget = GetBossPatternTarget();

	return IsValid(OutFaerin)
		&& OutFaerin->HasAuthority()
		&& CanRunBossPattern()
		&& IsValid(CloneClass)
		&& IsValid(OutPrimaryTarget)
		&& IsAlivePlayerTarget(OutPrimaryTarget);
}

void UPRGameplayAbility_FaerinCloneSequence::GatherAlivePlayers(TArray<AActor*>& OutAlivePlayers) const
{
	OutAlivePlayers.Reset();

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	for (TActorIterator<APRPlayerCharacter> It(World); It; ++It)
	{
		APRPlayerCharacter* PlayerCharacter = *It;
		if (IsAlivePlayerTarget(PlayerCharacter))
		{
			OutAlivePlayers.Add(PlayerCharacter);
		}
	}
}

void UPRGameplayAbility_FaerinCloneSequence::ResolveCloneTargets(
	AActor* PrimaryTarget,
	const TArray<AActor*>& AlivePlayers,
	TArray<AActor*>& OutCloneTargets) const
{
	OutCloneTargets.Reset();
	if (AlivePlayers.IsEmpty())
	{
		if (IsAlivePlayerTarget(PrimaryTarget))
		{
			OutCloneTargets.Add(PrimaryTarget);
		}
		return;
	}

	if (AlivePlayers.Num() == 1)
	{
		OutCloneTargets.Add(IsAlivePlayerTarget(PrimaryTarget) ? PrimaryTarget : AlivePlayers[0]);
		return;
	}

	TArray<AActor*> CandidateTargets;
	for (AActor* AlivePlayer : AlivePlayers)
	{
		if (IsValid(AlivePlayer) && AlivePlayer != PrimaryTarget)
		{
			CandidateTargets.Add(AlivePlayer);
		}
	}

	if (CandidateTargets.IsEmpty())
	{
		if (IsAlivePlayerTarget(PrimaryTarget))
		{
			OutCloneTargets.Add(PrimaryTarget);
		}
		return;
	}

	const APRFaerinCharacter* Faerin = Cast<APRFaerinCharacter>(GetBossAvatarCharacter());
	SortCloneTargetCandidates(Faerin, CandidateTargets);

	const int32 DesiredCloneCount = AlivePlayers.Num() >= 3
		? FMath::Min(FMath::Max(MaxCloneCount, 1), 2)
		: 1;
	for (int32 Index = 0; Index < CandidateTargets.Num() && OutCloneTargets.Num() < DesiredCloneCount; ++Index)
	{
		OutCloneTargets.Add(CandidateTargets[Index]);
	}
}

void UPRGameplayAbility_FaerinCloneSequence::SortCloneTargetCandidates(
	const APRFaerinCharacter* Faerin,
	TArray<AActor*>& CandidateTargets) const
{
	if (!IsValid(Faerin))
	{
		return;
	}

	const FVector FaerinLocation = Faerin->GetActorLocation();
	CandidateTargets.Sort([this, FaerinLocation](const AActor& Left, const AActor& Right)
	{
		const float LeftDistanceSq = FVector::DistSquared2D(FaerinLocation, Left.GetActorLocation());
		const float RightDistanceSq = FVector::DistSquared2D(FaerinLocation, Right.GetActorLocation());
		if (TargetSelectionPolicy == EPRFaerinCloneTargetSelectionPolicy::FarthestFromFaerin)
		{
			return LeftDistanceSq > RightDistanceSq;
		}

		return LeftDistanceSq < RightDistanceSq;
	});
}

bool UPRGameplayAbility_FaerinCloneSequence::SpawnCloneForTarget(
	APRFaerinCharacter* Faerin,
	AActor* CloneTarget,
	const int32 CloneIndex) const
{
	if (!IsValid(Faerin) || !IsAlivePlayerTarget(CloneTarget) || !IsValid(CloneClass))
	{
		return false;
	}

	UWorld* World = Faerin->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Faerin;
	SpawnParameters.Instigator = Faerin;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const FTransform SpawnTransform = ResolveCloneSpawnTransform(Faerin, CloneTarget, CloneIndex);
	APRFaerinCloneCharacter* SpawnedClone = World->SpawnActor<APRFaerinCloneCharacter>(
		CloneClass,
		SpawnTransform,
		SpawnParameters);
	if (!IsValid(SpawnedClone))
	{
		return false;
	}

	FVector AdjustedLocation = SpawnedClone->GetActorLocation();
	FRotator AdjustedRotation = SpawnedClone->GetActorRotation();
	if (World->FindTeleportSpot(SpawnedClone, AdjustedLocation, AdjustedRotation))
	{
		SpawnedClone->SetActorLocationAndRotation(
			AdjustedLocation,
			AdjustedRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}

	SpawnCloneSummonGroundVFX(Faerin, SpawnedClone);
	SpawnedClone->InitializeFaerinClone(Faerin, CloneTarget, CloneRuntimeConfig);
	return true;
}

FTransform UPRGameplayAbility_FaerinCloneSequence::ResolveCloneSpawnTransform(
	const APRFaerinCharacter* Faerin,
	AActor* CloneTarget,
	const int32 CloneIndex) const
{
	FVector LocalOffset = FVector::ZeroVector;
	if (SpawnOffsets.IsValidIndex(CloneIndex))
	{
		LocalOffset = SpawnOffsets[CloneIndex];
	}
	else if (!SpawnOffsets.IsEmpty())
	{
		LocalOffset = SpawnOffsets.Last();
	}

	const FRotator OwnerYawRotation(0.0f, Faerin->GetActorRotation().Yaw, 0.0f);
	FVector SpawnLocation = Faerin->GetActorLocation()
		+ (bRotateSpawnOffsetsByOwner ? OwnerYawRotation.RotateVector(LocalOffset) : LocalOffset);

	const FVector TraceStart = SpawnLocation + FVector(0.0f, 0.0f, FMath::Max(SpawnGroundTraceUpDistance, 0.0f));
	const FVector TraceEnd = SpawnLocation - FVector(0.0f, 0.0f, FMath::Max(SpawnGroundTraceDownDistance, 0.0f));
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRFaerinCloneSpawnGround), false, Faerin);
	if (IsValid(CloneTarget))
	{
		QueryParams.AddIgnoredActor(CloneTarget);
	}

	if (const UWorld* World = Faerin->GetWorld();
		IsValid(World) && World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, SpawnGroundTraceChannel, QueryParams))
	{
		SpawnLocation = GroundHit.ImpactPoint + FVector(0.0f, 0.0f, SpawnGroundHeightOffset);
	}

	FRotator SpawnRotation = OwnerYawRotation;
	if (bFaceAssignedTargetOnSpawn && IsValid(CloneTarget))
	{
		FVector DirectionToTarget = CloneTarget->GetActorLocation() - SpawnLocation;
		DirectionToTarget.Z = 0.0f;
		if (DirectionToTarget.Normalize())
		{
			SpawnRotation = DirectionToTarget.Rotation();
		}
	}
	SpawnRotation.Pitch = 0.0f;
	SpawnRotation.Roll = 0.0f;
	SpawnRotation.Yaw += SpawnYawOffset;

	return FTransform(SpawnRotation, SpawnLocation, FVector::OneVector);
}

bool UPRGameplayAbility_FaerinCloneSequence::IsAlivePlayerTarget(AActor* CandidateActor) const
{
	if (!IsValid(CandidateActor) || UPRCombatStatics::GetActorTeam(CandidateActor) != EPRTeam::Player)
	{
		return false;
	}

	const UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CandidateActor);
	return IsValid(TargetASC)
		&& !TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Down)
		&& !TargetASC->HasMatchingGameplayTag(PRGameplayTags::State_Dead);
}

bool UPRGameplayAbility_FaerinCloneSequence::RegisterCharacterEventListener()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		return false;
	}

	ActiveEventRouter = AvatarActor->FindComponentByClass<UPRFaerinCharacterEventRouterComponent>();
	if (!IsValid(ActiveEventRouter))
	{
		return false;
	}

	ActiveEventRouter->RegisterFaerinEventListener(
		this,
		FFaerinCharacterEventSignature::FDelegate::CreateUObject(
			this,
			&UPRGameplayAbility_FaerinCloneSequence::HandleFaerinCharacterEvent));
	return true;
}

void UPRGameplayAbility_FaerinCloneSequence::UnregisterCharacterEventListener()
{
	if (IsValid(ActiveEventRouter))
	{
		ActiveEventRouter->UnregisterFaerinEventListener(this);
		ActiveEventRouter = nullptr;
	}
}

void UPRGameplayAbility_FaerinCloneSequence::HandleFaerinCharacterEvent(FName EventName)
{
	if (EventName != SummonCharacterEventName || bCloneActorsSpawned)
	{
		return;
	}

	if (!SpawnPendingClonesAtNotify())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (bEndAbilityAfterSummonEvent)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

bool UPRGameplayAbility_FaerinCloneSequence::SpawnPendingClonesAtNotify()
{
	APRFaerinCharacter* Faerin = ActiveFaerin.Get();
	if (!IsValid(Faerin) || !Faerin->HasAuthority())
	{
		return false;
	}

	int32 SpawnedCloneCount = 0;
	for (const TObjectPtr<AActor>& CloneTargetPtr : PendingCloneTargets)
	{
		if (SpawnedCloneCount >= FMath::Max(MaxCloneCount, 1))
		{
			break;
		}

		AActor* CloneTarget = CloneTargetPtr.Get();
		if (SpawnCloneForTarget(Faerin, CloneTarget, SpawnedCloneCount))
		{
			++SpawnedCloneCount;
		}
	}

	bCloneActorsSpawned = SpawnedCloneCount > 0;
	return bCloneActorsSpawned;
}

void UPRGameplayAbility_FaerinCloneSequence::SpawnCloneSummonGroundVFX(
	APRFaerinCharacter* Faerin,
	const APRFaerinCloneCharacter* SpawnedClone) const
{
	if (!IsValid(Faerin) || !IsValid(CloneRuntimeConfig.SpawnLocationNiagaraSystem.Get()) || !IsValid(SpawnedClone))
	{
		return;
	}

	const FSoftObjectPath NiagaraSystemPath(CloneRuntimeConfig.SpawnLocationNiagaraSystem.Get());
	Faerin->Multicast_SpawnFaerinWorldNiagara(
		NiagaraSystemPath,
		ResolveCloneGroundVFXLocation(SpawnedClone),
		FRotator::ZeroRotator,
		CloneRuntimeConfig.WorldNiagaraScale,
		CloneRuntimeConfig.WorldNiagaraLifeSeconds);
}

FVector UPRGameplayAbility_FaerinCloneSequence::ResolveCloneGroundVFXLocation(
	const APRFaerinCloneCharacter* SpawnedClone) const
{
	if (!IsValid(SpawnedClone))
	{
		return FVector::ZeroVector;
	}

	FVector GroundLocation = SpawnedClone->GetActorLocation();
	if (const UCapsuleComponent* CapsuleComponent = SpawnedClone->GetCapsuleComponent())
	{
		GroundLocation.Z -= CapsuleComponent->GetScaledCapsuleHalfHeight();
	}

	return GroundLocation;
}

void UPRGameplayAbility_FaerinCloneSequence::HandleSummonMontageCompleted()
{
	ActiveSummonMontageTask = nullptr;

	if (!bCloneActorsSpawned)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (bEndAbilityOnSummonMontageCompleted)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UPRGameplayAbility_FaerinCloneSequence::HandleSummonMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
