// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 순간이동 VFX 컴포넌트 구현)
#include "PRFaerinTeleportVFXComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/AudioComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NavigationSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/System/PRAssetManager.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRFaerinTeleportVFX, Log, All);

// ===== 초기화 =====

UPRFaerinTeleportVFXComponent::UPRFaerinTeleportVFXComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UPRFaerinTeleportVFXComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BodyDisappearNiagaraTimerHandle);
	}

	CancelTeleportVFX();
	CleanupTeleportVFXLocal();
	CleanupTransientNiagaraLocal();
	CleanupDissolveMaterialsLocal();

	Super::EndPlay(EndPlayReason);
}

void UPRFaerinTeleportVFXComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bLocalPresentationActive)
	{
		UpdateTeleportVFXPresentation(DeltaTime);
	}

	if (bLocalDissolveActive)
	{
		UpdateDissolvePresentation(DeltaTime);
	}
}

// ===== 실행 =====

bool UPRFaerinTeleportVFXComponent::StartTeleportVFX(
	const FPRFaerinPatternPlan& PatternPlan,
	AActor* TargetActor)
{
	APREnemyBaseCharacter* OwnerEnemy = GetOwnerEnemy();
	if (!IsValid(OwnerEnemy) || !OwnerEnemy->HasAuthority() || !IsValid(TargetActor))
	{
		return false;
	}

	if (bTeleportVFXRunning)
	{
		UE_LOG(
			LogPRFaerinTeleportVFX,
			Warning,
			TEXT("Faerin Teleport VFX가 이미 실행 중이다. Owner=%s, AbilityTag=%s"),
			*GetNameSafe(OwnerEnemy),
			*PatternPlan.AbilityTag.ToString());
		return false;
	}

	FVector ConvergeLocation = FVector::ZeroVector;
	if (!ResolveConvergeLocation(PatternPlan, TargetActor, ConvergeLocation))
	{
		return false;
	}

	CachedBossStartLocation = OwnerEnemy->GetActorLocation();
	CachedBossStartRotation = OwnerEnemy->GetActorRotation();
	CachedConvergeLocation = ConvergeLocation;
	CachedConvergeRotation = ResolveConvergeRotation(PatternPlan, ConvergeLocation, TargetActor);
	bCachedPlayTeleportInStage = ShouldPlayTeleportInStage(PatternPlan);
	bTeleportVFXRunning = true;

	BeginOwnerReplicationOverride();
	MulticastStartTeleportOutPresentation(CachedBossStartLocation, CachedBossStartRotation);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TeleportOutVanishTimerHandle);

		if (TeleportOutVanishDelay <= UE_SMALL_NUMBER)
		{
			World->GetTimerManager().SetTimerForNextTick(this, &UPRFaerinTeleportVFXComponent::BeginHiddenTeleportVFXStage);
		}
		else
		{
			World->GetTimerManager().SetTimer(
				TeleportOutVanishTimerHandle,
				this,
				&UPRFaerinTeleportVFXComponent::BeginHiddenTeleportVFXStage,
				TeleportOutVanishDelay,
				false);
		}
	}
	else
	{
		BeginHiddenTeleportVFXStage();
	}

	return true;
}

void UPRFaerinTeleportVFXComponent::CancelTeleportVFX()
{
	if (!bTeleportVFXRunning && !bLocalPresentationActive && !bLocalDissolveActive)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TeleportOutDissolveTimerHandle);
		World->GetTimerManager().ClearTimer(BodyDisappearNiagaraTimerHandle);
		World->GetTimerManager().ClearTimer(TeleportOutVanishTimerHandle);
		World->GetTimerManager().ClearTimer(HiddenFinishTimerHandle);
		World->GetTimerManager().ClearTimer(TeleportInFinishTimerHandle);
	}

	AActor* OwnerActor = GetOwner();
	const FVector FinalLocation = IsValid(OwnerActor)
		? OwnerActor->GetActorLocation()
		: CachedConvergeLocation;
	const FRotator FinalRotation = IsValid(OwnerActor)
		? OwnerActor->GetActorRotation()
		: CachedConvergeRotation;

	bTeleportVFXRunning = false;
	MulticastCancelTeleportVFXPresentation(FinalLocation, FinalRotation);
	CompleteTeleportVFX(false);
}

void UPRFaerinTeleportVFXComponent::BeginHiddenTeleportVFXStage()
{
	if (!bTeleportVFXRunning)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TeleportOutVanishTimerHandle);
	}

	FVector LeftStartLocation = FVector::ZeroVector;
	FVector LeftWanderLocation = FVector::ZeroVector;
	FVector RightStartLocation = FVector::ZeroVector;
	FVector RightWanderLocation = FVector::ZeroVector;
	BuildVFXPaths(
		CachedBossStartLocation,
		CachedConvergeLocation,
		LeftStartLocation,
		LeftWanderLocation,
		RightStartLocation,
		RightWanderLocation);

	const float ResolvedWanderSeconds = FMath::Max(VFXWanderSeconds, 0.0f);
	const float ResolvedConvergeSeconds = FMath::Max(VFXConvergeSeconds, 0.0f);
	MulticastBeginHiddenTeleportVFXPresentation(
		CachedBossStartLocation,
		CachedBossStartRotation,
		CachedConvergeLocation,
		CachedConvergeRotation,
		LeftStartLocation,
		LeftWanderLocation,
		RightStartLocation,
		RightWanderLocation,
		ResolvedWanderSeconds,
		ResolvedConvergeSeconds,
		bDisableCollisionWhileHidden);

	const float HiddenSeconds = ResolvedWanderSeconds + ResolvedConvergeSeconds;
	if (UWorld* World = GetWorld())
	{
		if (HiddenSeconds <= UE_SMALL_NUMBER)
		{
			World->GetTimerManager().SetTimerForNextTick(this, &UPRFaerinTeleportVFXComponent::FinishHiddenTeleportVFXStage);
		}
		else
		{
			World->GetTimerManager().SetTimer(
				HiddenFinishTimerHandle,
				this,
				&UPRFaerinTeleportVFXComponent::FinishHiddenTeleportVFXStage,
				HiddenSeconds,
				false);
		}
	}
	else
	{
		FinishHiddenTeleportVFXStage();
	}
}

void UPRFaerinTeleportVFXComponent::FinishHiddenTeleportVFXStage()
{
	if (!bTeleportVFXRunning)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HiddenFinishTimerHandle);
	}

	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->SetActorLocationAndRotation(
			CachedConvergeLocation,
			CachedConvergeRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}

	ApplyRevealSplashDamage();

	MulticastFinishTeleportVFXPresentation(CachedConvergeLocation, CachedConvergeRotation, bCachedPlayTeleportInStage);

	if (!bCachedPlayTeleportInStage)
	{
		CompleteTeleportVFX(true);
		return;
	}

	const float WaitSeconds = ResolveMontageDuration(TeleportInMontage, TeleportInMontagePlayRate, TeleportInFinishDelay);
	if (UWorld* World = GetWorld())
	{
		if (WaitSeconds <= UE_SMALL_NUMBER)
		{
			World->GetTimerManager().SetTimerForNextTick(this, &UPRFaerinTeleportVFXComponent::FinishTeleportInStage);
		}
		else
		{
			World->GetTimerManager().SetTimer(
				TeleportInFinishTimerHandle,
				this,
				&UPRFaerinTeleportVFXComponent::FinishTeleportInStage,
				WaitSeconds,
				false);
		}
	}
	else
	{
		FinishTeleportInStage();
	}
}

void UPRFaerinTeleportVFXComponent::FinishTeleportInStage()
{
	if (!bTeleportVFXRunning)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TeleportInFinishTimerHandle);
	}

	CompleteTeleportVFX(true);
}

void UPRFaerinTeleportVFXComponent::CompleteTeleportVFX(const bool bSucceeded)
{
	bTeleportVFXRunning = false;
	EndOwnerReplicationOverride();
	OnTeleportVFXFinished.Broadcast(bSucceeded);
}

// ===== 위치 계산 =====

APREnemyBaseCharacter* UPRFaerinTeleportVFXComponent::GetOwnerEnemy() const
{
	return Cast<APREnemyBaseCharacter>(GetOwner());
}

bool UPRFaerinTeleportVFXComponent::ResolveConvergeLocation(
	const FPRFaerinPatternPlan& PatternPlan,
	AActor* TargetActor,
	FVector& OutLocation) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();
	FVector RawLocation = TargetLocation;
	if (PatternPlan.LoopMetadata.TeleportVFXConvergePolicy == EPRFaerinTeleportVFXConvergePolicy::TargetNearby)
	{
		const float MinRadius = FMath::Max(PatternPlan.LoopMetadata.TeleportVFXNearbyMinRadius, 0.0f);
		const float MaxRadius = FMath::Max(PatternPlan.LoopMetadata.TeleportVFXNearbyMaxRadius, MinRadius);

		for (int32 AttemptIndex = 0; AttemptIndex < 8; ++AttemptIndex)
		{
			const float AngleDegrees = FMath::FRandRange(0.0f, 360.0f);
			const float Radius = FMath::FRandRange(MinRadius, MaxRadius);
			const FVector Direction = FVector::ForwardVector.RotateAngleAxis(AngleDegrees, FVector::UpVector);
			RawLocation = TargetLocation + Direction * Radius + PatternPlan.LoopMetadata.TeleportVFXConvergeWorldOffset;

			if (ProjectLocationToNavigation(RawLocation, OutLocation))
			{
				return true;
			}
		}
	}

	RawLocation += PatternPlan.LoopMetadata.TeleportVFXConvergeWorldOffset;
	if (ProjectLocationToNavigation(RawLocation, OutLocation))
	{
		return true;
	}

	OutLocation = RawLocation;
	return true;
}

FRotator UPRFaerinTeleportVFXComponent::ResolveConvergeRotation(
	const FPRFaerinPatternPlan& PatternPlan,
	const FVector& ConvergeLocation,
	AActor* TargetActor) const
{
	if (!PatternPlan.LoopMetadata.bTeleportVFXFaceTargetOnFinish)
	{
		const AActor* OwnerActor = GetOwner();
		return IsValid(OwnerActor) ? OwnerActor->GetActorRotation() : FRotator::ZeroRotator;
	}

	if (!IsValid(TargetActor))
	{
		const AActor* OwnerActor = GetOwner();
		return IsValid(OwnerActor) ? OwnerActor->GetActorRotation() : FRotator::ZeroRotator;
	}

	FVector ToTarget = TargetActor->GetActorLocation() - ConvergeLocation;
	ToTarget.Z = 0.0f;
	if (!ToTarget.Normalize())
	{
		return TargetActor->GetActorRotation();
	}

	return ToTarget.Rotation();
}

bool UPRFaerinTeleportVFXComponent::ProjectLocationToNavigation(
	const FVector& RawLocation,
	FVector& OutLocation) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!IsValid(NavigationSystem))
	{
		return false;
	}

	FNavLocation ProjectedLocation;
	if (!NavigationSystem->ProjectPointToNavigation(RawLocation, ProjectedLocation, ConvergeNavProjectExtent))
	{
		return false;
	}

	OutLocation = ProjectedLocation.Location;
	return true;
}

bool UPRFaerinTeleportVFXComponent::ShouldPlayTeleportInStage(const FPRFaerinPatternPlan& PatternPlan) const
{
	return PatternPlan.LoopMetadata.TeleportWrapperPolicy == EPRFaerinTeleportWrapperPolicy::TeleportOutAndIn;
}

bool UPRFaerinTeleportVFXComponent::ShouldApplyRevealSplashDamage() const
{
	if (!bApplyRevealSplashDamage
		|| RevealSplashRadius <= 0.0f
		|| (RevealSplashDamageMultiplier <= 0.0f && RevealSplashPoiseDamage <= 0.0f))
	{
		return false;
	}

	const APRBossBaseCharacter* BossCharacter = Cast<APRBossBaseCharacter>(GetOwner());
	if (!IsValid(BossCharacter) || !BossCharacter->HasAuthority())
	{
		return false;
	}

	return static_cast<uint8>(BossCharacter->GetCurrentPhase())
		>= static_cast<uint8>(RevealSplashMinimumPhase);
}

void UPRFaerinTeleportVFXComponent::ApplyRevealSplashDamage()
{
	if (!ShouldApplyRevealSplashDamage())
	{
		return;
	}

	APRBossBaseCharacter* BossCharacter = Cast<APRBossBaseCharacter>(GetOwner());
	UWorld* World = GetWorld();
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(BossCharacter);
	if (!IsValid(BossCharacter) || !IsValid(World))
	{
		return;
	}

	const FVector SplashCenter = BossCharacter->GetActorTransform().TransformPositionNoScale(RevealSplashLocalOffset);
	FVector SplashNiagaraLocation = SplashCenter;
	FRotator SplashNiagaraRotation = FRotator::ZeroRotator;
	ResolveRevealSplashNiagaraTransform(
		BossCharacter,
		SplashCenter,
		SplashNiagaraLocation,
		SplashNiagaraRotation);
	MulticastRevealSplashPresentation(SplashNiagaraLocation, SplashNiagaraRotation, RevealSplashRadius);

	if (!IsValid(SourceASC))
	{
		return;
	}

	TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = RevealSplashDamageEffectClass;
	if (!IsValid(ResolvedDamageEffectClass))
	{
		const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
		if (IsValid(Registry))
		{
			ResolvedDamageEffectClass = Registry->DamageGE_FromEnemy;
		}
	}

	if (!IsValid(ResolvedDamageEffectClass))
	{
		return;
	}

	const FCollisionShape SplashShape = FCollisionShape::MakeSphere(RevealSplashRadius);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FaerinRevealSplashDamage), false, BossCharacter);
	QueryParams.AddIgnoredActor(BossCharacter);
	QueryParams.bFindInitialOverlaps = true;

	TArray<FOverlapResult> OverlapResults;
	const bool bOverlapped = World->OverlapMultiByChannel(
		OverlapResults,
		SplashCenter,
		FQuat::Identity,
		RevealSplashOverlapChannel,
		SplashShape,
		QueryParams);

	if (bDrawDebugRevealSplash)
	{
		DrawDebugSphere(World, SplashCenter, RevealSplashRadius, 32, FColor::Purple, false, RevealSplashDebugDrawDuration);
	}

	if (!bOverlapped)
	{
		return;
	}

	TSet<TWeakObjectPtr<AActor>> DamagedActors;
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* TargetActor = OverlapResult.GetActor();
		const TWeakObjectPtr<AActor> TargetKey(TargetActor);
		if (!IsValid(TargetActor)
			|| TargetActor == BossCharacter
			|| DamagedActors.Contains(TargetKey)
			|| UPRCombatStatics::GetActorTeam(TargetActor) == EPRTeam::Enemy)
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (!IsValid(TargetASC))
		{
			continue;
		}

		FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
		EffectContext.AddSourceObject(this);
		EffectContext.AddInstigator(BossCharacter, BossCharacter);

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
			ResolvedDamageEffectClass,
			1.0f,
			EffectContext);
		if (!SpecHandle.IsValid())
		{
			continue;
		}

		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_AttackMultiplier,
			FMath::Max(RevealSplashDamageMultiplier, 0.0f));
		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_PoiseDamage,
			FMath::Max(RevealSplashPoiseDamage, 0.0f));

		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		DamagedActors.Add(TargetKey);
	}
}

float UPRFaerinTeleportVFXComponent::ResolveMontageDuration(
	UAnimMontage* Montage,
	const float PlayRate,
	const float OverrideSeconds) const
{
	if (OverrideSeconds > 0.0f)
	{
		return OverrideSeconds;
	}

	if (!IsValid(Montage))
	{
		return 0.0f;
	}

	return Montage->GetPlayLength() / FMath::Max(PlayRate, UE_SMALL_NUMBER);
}

void UPRFaerinTeleportVFXComponent::BuildVFXPaths(
	const FVector& BossStartLocation,
	const FVector& ConvergeLocation,
	FVector& OutLeftStart,
	FVector& OutLeftWander,
	FVector& OutRightStart,
	FVector& OutRightWander) const
{
	FVector TravelDirection = ConvergeLocation - BossStartLocation;
	TravelDirection.Z = 0.0f;
	if (!TravelDirection.Normalize())
	{
		const AActor* OwnerActor = GetOwner();
		TravelDirection = IsValid(OwnerActor) ? OwnerActor->GetActorForwardVector() : FVector::ForwardVector;
		TravelDirection.Z = 0.0f;
		TravelDirection.Normalize();
	}

	FVector RightDirection = FVector::CrossProduct(FVector::UpVector, TravelDirection);
	if (!RightDirection.Normalize())
	{
		RightDirection = FVector::RightVector;
	}

	const FVector StartBaseLocation = BossStartLocation + FVector::UpVector * VFXStartHeightOffset;
	OutLeftStart = StartBaseLocation - RightDirection * VFXStartSideOffset;
	OutRightStart = StartBaseLocation + RightDirection * VFXStartSideOffset;
	OutLeftWander = BuildWanderLocation(OutLeftStart - RightDirection * VFXWanderSideOffset, TravelDirection);
	OutRightWander = BuildWanderLocation(OutRightStart + RightDirection * VFXWanderSideOffset, TravelDirection);
}

FVector UPRFaerinTeleportVFXComponent::BuildWanderLocation(
	const FVector& OriginLocation,
	const FVector& TravelDirection) const
{
	const float MinDistance = FMath::Max(VFXWanderMinDistance, 0.0f);
	const float MaxDistance = FMath::Max(VFXWanderMaxDistance, MinDistance);
	const float Radius = FMath::FRandRange(MinDistance, MaxDistance);
	const float AngleDegrees = FMath::FRandRange(0.0f, 360.0f);
	const FVector RandomDirection = FVector::ForwardVector.RotateAngleAxis(AngleDegrees, FVector::UpVector);

	return OriginLocation
		+ RandomDirection * Radius
		+ TravelDirection * VFXWanderForwardOffset
		+ FVector::UpVector * VFXWanderHeightOffset;
}

// ===== 복제/로컬 프레젠테이션 =====

void UPRFaerinTeleportVFXComponent::BeginOwnerReplicationOverride()
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority() || bHasSavedOwnerReplicateMovement)
	{
		return;
	}

	bSavedOwnerReplicateMovement = OwnerActor->IsReplicatingMovement();
	bHasSavedOwnerReplicateMovement = true;
	OwnerActor->SetReplicateMovement(false);
}

void UPRFaerinTeleportVFXComponent::EndOwnerReplicationOverride()
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority() || !bHasSavedOwnerReplicateMovement)
	{
		return;
	}

	OwnerActor->SetReplicateMovement(bSavedOwnerReplicateMovement);
	bHasSavedOwnerReplicateMovement = false;
	OwnerActor->ForceNetUpdate();
}

void UPRFaerinTeleportVFXComponent::StartTeleportOutPresentationLocal(
	FVector BossStartLocation,
	FRotator BossStartRotation)
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	CleanupTeleportVFXLocal();
	CleanupTransientNiagaraLocal();
	bLocalPresentationActive = false;
	bLocalOriginalCollisionEnabled = OwnerActor->GetActorEnableCollision();
	bLocalDisableCollisionWhileHidden = bDisableCollisionWhileHidden;

	OwnerActor->SetActorLocationAndRotation(
		BossStartLocation,
		BossStartRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	OwnerActor->SetActorHiddenInGame(false);
	OwnerActor->SetActorEnableCollision(bLocalOriginalCollisionEnabled);

	SetDissolveValueLocal(1.0f);
	PlayMontageLocal(TeleportOutMontage, TeleportOutMontagePlayRate);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TeleportOutDissolveTimerHandle);
		World->GetTimerManager().ClearTimer(BodyDisappearNiagaraTimerHandle);
		const bool bShouldPlayDissolve = bUseMaterialDissolve || IsValid(DissolveNiagaraSystem);
		if (bShouldPlayDissolve && TeleportOutDissolveStartDelay > UE_SMALL_NUMBER)
		{
			World->GetTimerManager().SetTimer(
				TeleportOutDissolveTimerHandle,
				[this]()
				{
					StartDissolveLocal(1.0f, 0.0f, TeleportOutDissolveSeconds);
				},
				TeleportOutDissolveStartDelay,
				false);
		}
		else if (bShouldPlayDissolve)
		{
			StartDissolveLocal(1.0f, 0.0f, TeleportOutDissolveSeconds);
		}

		if (BodyDisappearNiagaraDelaySeconds > UE_SMALL_NUMBER)
		{
			World->GetTimerManager().SetTimer(
				BodyDisappearNiagaraTimerHandle,
				[this]()
				{
					SpawnBodyNiagaraLocal(BodyDisappearNiagaraSystem);
				},
				BodyDisappearNiagaraDelaySeconds,
				false);
		}
		else
		{
			SpawnBodyNiagaraLocal(BodyDisappearNiagaraSystem);
		}
	}
	else
	{
		SpawnBodyNiagaraLocal(BodyDisappearNiagaraSystem);
	}

	SetComponentTickEnabled(bLocalDissolveActive);
}

void UPRFaerinTeleportVFXComponent::BeginHiddenPresentationLocal(
	FVector ConvergeLocation,
	FRotator ConvergeRotation,
	FVector LeftStartLocation,
	FVector LeftWanderLocation,
	FVector RightStartLocation,
	FVector RightWanderLocation,
	float InWanderSeconds,
	float InConvergeSeconds,
	bool bInDisableCollisionWhileHidden)
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	LocalLeftStartLocation = LeftStartLocation;
	LocalLeftWanderLocation = LeftWanderLocation;
	LocalRightStartLocation = RightStartLocation;
	LocalRightWanderLocation = RightWanderLocation;
	LocalConvergeLocation = ConvergeLocation;
	LocalWanderSeconds = FMath::Max(InWanderSeconds, 0.0f);
	LocalConvergeSeconds = FMath::Max(InConvergeSeconds, 0.0f);
	LocalPresentationElapsedSeconds = 0.0f;
	bLocalPresentationActive = true;
	bLocalDisableCollisionWhileHidden = bInDisableCollisionWhileHidden;
	bLocalDissolveActive = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TeleportOutDissolveTimerHandle);
		World->GetTimerManager().ClearTimer(BodyDisappearNiagaraTimerHandle);
	}
	SetDissolveValueLocal(0.0f);

	OwnerActor->SetActorHiddenInGame(true);
	if (bLocalDisableCollisionWhileHidden)
	{
		OwnerActor->SetActorEnableCollision(false);
	}

	CleanupTeleportVFXLocal();
	LeftTeleportVFXComponent = SpawnTeleportVFXLocal(LeftStartLocation);
	RightTeleportVFXComponent = SpawnTeleportVFXLocal(RightStartLocation);

	// 두 텔레포트 VFX 프로젝타일에 비행 사운드를 attach해 이동 동안 따라 재생한다. (각 머신 로컬 재생)
	// bStopWhenAttachedToDestroyed=true: VFX가 정리/소멸되면 사운드도 함께 정지·정리된다. (루프 큐 잔류 방지)
	if (IsValid(TeleportVFXProjectileSoundCue))
	{
		if (IsValid(LeftTeleportVFXComponent))
		{
			LeftTeleportVFXAudioComponent = UGameplayStatics::SpawnSoundAttached(
				TeleportVFXProjectileSoundCue,
				LeftTeleportVFXComponent,
				NAME_None,
				FVector(ForceInit),
				FRotator::ZeroRotator,
				EAttachLocation::KeepRelativeOffset,
				/*bStopWhenAttachedToDestroyed=*/true);
		}
		if (IsValid(RightTeleportVFXComponent))
		{
			RightTeleportVFXAudioComponent = UGameplayStatics::SpawnSoundAttached(
				TeleportVFXProjectileSoundCue,
				RightTeleportVFXComponent,
				NAME_None,
				FVector(ForceInit),
				FRotator::ZeroRotator,
				EAttachLocation::KeepRelativeOffset,
				/*bStopWhenAttachedToDestroyed=*/true);
		}
	}

	SetTeleportVFXPairLocation(LeftStartLocation, RightStartLocation);

	SetComponentTickEnabled(true);
}

void UPRFaerinTeleportVFXComponent::FinishTeleportPresentationLocal(
	FVector FinalLocation,
	FRotator FinalRotation,
	const bool bPlayTeleportInStage)
{
	// 두 VFX가 집결(합쳐짐)하는 순간 집결 위치에 SFX를 재생한다. (모든 클라이언트. 아래 CleanupTeleportVFXLocal로 VFX/사운드가 정리되기 직전)
	if (IsValid(TeleportVFXMergeSoundCue))
	{
		UGameplayStatics::SpawnSoundAtLocation(this, TeleportVFXMergeSoundCue, FinalLocation);
	}

	AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor))
	{
		OwnerActor->SetActorLocationAndRotation(
			FinalLocation,
			FinalRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		OwnerActor->SetActorHiddenInGame(false);
		if (bLocalDisableCollisionWhileHidden)
		{
			OwnerActor->SetActorEnableCollision(bLocalOriginalCollisionEnabled);
		}
	}

	CleanupTeleportVFXLocal();

	if (bPlayTeleportInStage)
	{
		SetDissolveValueLocal(0.0f);
		SpawnBodyNiagaraLocal(BodyRevealNiagaraSystem);
		PlayMontageLocal(TeleportInMontage, TeleportInMontagePlayRate);
		StartDissolveLocal(0.0f, 1.0f, TeleportInRevealSeconds);
	}
	else
	{
		SetDissolveValueLocal(1.0f);
	}

	bLocalPresentationActive = false;
	SetComponentTickEnabled(bLocalDissolveActive);
}

void UPRFaerinTeleportVFXComponent::CancelTeleportPresentationLocal(
	FVector FinalLocation,
	FRotator FinalRotation)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TeleportOutDissolveTimerHandle);
		World->GetTimerManager().ClearTimer(BodyDisappearNiagaraTimerHandle);
	}

	AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor))
	{
		OwnerActor->SetActorLocationAndRotation(
			FinalLocation,
			FinalRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		OwnerActor->SetActorHiddenInGame(false);
		if (bLocalDisableCollisionWhileHidden)
		{
			OwnerActor->SetActorEnableCollision(bLocalOriginalCollisionEnabled);
		}
	}

	CleanupTeleportVFXLocal();
	CleanupTransientNiagaraLocal();
	SetDissolveValueLocal(1.0f);
	bLocalPresentationActive = false;
	bLocalDissolveActive = false;
	SetComponentTickEnabled(false);
}

void UPRFaerinTeleportVFXComponent::UpdateTeleportVFXPresentation(const float DeltaTime)
{
	LocalPresentationElapsedSeconds += FMath::Max(DeltaTime, 0.0f);

	const float TotalSeconds = LocalWanderSeconds + LocalConvergeSeconds;
	if (TotalSeconds <= UE_SMALL_NUMBER)
	{
		SetTeleportVFXPairLocation(LocalConvergeLocation, LocalConvergeLocation);
		return;
	}

	FVector LeftLocation = LocalConvergeLocation;
	FVector RightLocation = LocalConvergeLocation;
	if (LocalPresentationElapsedSeconds <= LocalWanderSeconds && LocalWanderSeconds > UE_SMALL_NUMBER)
	{
		float Alpha = FMath::Clamp(LocalPresentationElapsedSeconds / LocalWanderSeconds, 0.0f, 1.0f);
		Alpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
		LeftLocation = FMath::Lerp(LocalLeftStartLocation, LocalLeftWanderLocation, Alpha);
		RightLocation = FMath::Lerp(LocalRightStartLocation, LocalRightWanderLocation, Alpha);
	}
	else if (LocalConvergeSeconds > UE_SMALL_NUMBER)
	{
		float Alpha = FMath::Clamp(
			(LocalPresentationElapsedSeconds - LocalWanderSeconds) / LocalConvergeSeconds,
			0.0f,
			1.0f);
		Alpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
		LeftLocation = FMath::Lerp(LocalLeftWanderLocation, LocalConvergeLocation, Alpha);
		RightLocation = FMath::Lerp(LocalRightWanderLocation, LocalConvergeLocation, Alpha);
	}

	SetTeleportVFXPairLocation(LeftLocation, RightLocation);
}

void UPRFaerinTeleportVFXComponent::UpdateDissolvePresentation(const float DeltaTime)
{
	LocalDissolveElapsedSeconds += FMath::Max(DeltaTime, 0.0f);
	const float Duration = FMath::Max(LocalDissolveDurationSeconds, UE_SMALL_NUMBER);
	const float Alpha = FMath::Clamp(LocalDissolveElapsedSeconds / Duration, 0.0f, 1.0f);
	const float DissolveValue = FMath::Lerp(LocalDissolveStartValue, LocalDissolveEndValue, Alpha);
	SetDissolveValueLocal(DissolveValue);

	if (Alpha >= 1.0f)
	{
		bLocalDissolveActive = false;
		SetDissolveValueLocal(LocalDissolveEndValue);
		CleanupDissolveNiagaraLocal();
		SetComponentTickEnabled(bLocalPresentationActive);
	}
}

void UPRFaerinTeleportVFXComponent::SetTeleportVFXPairLocation(
	const FVector& LeftLocation,
	const FVector& RightLocation)
{
	if (IsValid(LeftTeleportVFXComponent))
	{
		LeftTeleportVFXComponent->SetWorldLocation(LeftLocation);
	}

	if (IsValid(RightTeleportVFXComponent))
	{
		RightTeleportVFXComponent->SetWorldLocation(RightLocation);
	}
}

void UPRFaerinTeleportVFXComponent::CleanupTeleportVFXLocal()
{
	// attach한 비행 사운드를 먼저 정지·파괴한다. (오디오 owner가 보스라서 VFX 컴포넌트 파괴만으로는 자동 정지되지 않음)
	if (IsValid(LeftTeleportVFXAudioComponent))
	{
		LeftTeleportVFXAudioComponent->Stop();
		LeftTeleportVFXAudioComponent->DestroyComponent();
	}
	if (IsValid(RightTeleportVFXAudioComponent))
	{
		RightTeleportVFXAudioComponent->Stop();
		RightTeleportVFXAudioComponent->DestroyComponent();
	}
	LeftTeleportVFXAudioComponent = nullptr;
	RightTeleportVFXAudioComponent = nullptr;

	if (IsValid(LeftTeleportVFXComponent))
	{
		LeftTeleportVFXComponent->Deactivate();
		LeftTeleportVFXComponent->DestroyComponent();
	}

	if (IsValid(RightTeleportVFXComponent))
	{
		RightTeleportVFXComponent->Deactivate();
		RightTeleportVFXComponent->DestroyComponent();
	}

	LeftTeleportVFXComponent = nullptr;
	RightTeleportVFXComponent = nullptr;
}

void UPRFaerinTeleportVFXComponent::CleanupTransientNiagaraLocal()
{
	for (UNiagaraComponent* NiagaraComponent : LocalTransientNiagaraComponents)
	{
		if (IsValid(NiagaraComponent))
		{
			NiagaraComponent->Deactivate();
			NiagaraComponent->DestroyComponent();
		}
	}

	LocalTransientNiagaraComponents.Reset();
}

void UPRFaerinTeleportVFXComponent::ResolveRevealSplashNiagaraTransform(
	const APREnemyBaseCharacter* BossCharacter,
	const FVector& SplashCenter,
	FVector& OutLocation,
	FRotator& OutRotation) const
{
	OutLocation = SplashCenter;
	OutRotation = FRotator::ZeroRotator;

	if (RevealSplashNiagaraSocketName == NAME_None
		|| !IsValid(BossCharacter)
		|| !IsValid(BossCharacter->GetMesh()))
	{
		return;
	}

	const USkeletalMeshComponent* MeshComponent = BossCharacter->GetMesh();
	if (!MeshComponent->DoesSocketExist(RevealSplashNiagaraSocketName))
	{
		return;
	}

	const FTransform SocketTransform = MeshComponent->GetSocketTransform(RevealSplashNiagaraSocketName);
	OutLocation = SocketTransform.GetLocation();
	OutRotation = SocketTransform.Rotator();
}

void UPRFaerinTeleportVFXComponent::PlayRevealSplashPresentationLocal(
	FVector SplashLocation,
	FRotator SplashRotation,
	const float SplashRadius)
{
	if (!IsValid(RevealSplashNiagaraSystem))
	{
		return;
	}

	UNiagaraComponent* NiagaraComponent = SpawnTransientNiagaraAtLocationLocal(
		RevealSplashNiagaraSystem,
		SplashLocation,
		SplashRotation,
		RevealSplashNiagaraScale,
		RevealSplashNiagaraLifeSeconds);

	if (IsValid(NiagaraComponent) && RevealSplashRadiusParameterName != NAME_None)
	{
		NiagaraComponent->SetVariableFloat(RevealSplashRadiusParameterName, SplashRadius);
	}
}

void UPRFaerinTeleportVFXComponent::SpawnBodyNiagaraLocal(UNiagaraSystem* NiagaraSystem)
{
	const APREnemyBaseCharacter* OwnerEnemy = GetOwnerEnemy();
	if (!IsValid(OwnerEnemy) || !IsValid(NiagaraSystem) || !IsValid(OwnerEnemy->GetMesh()))
	{
		return;
	}

	const FTransform SpawnTransform = BodyNiagaraAttachSocketName != NAME_None
		? OwnerEnemy->GetMesh()->GetSocketTransform(BodyNiagaraAttachSocketName)
		: OwnerEnemy->GetMesh()->GetComponentTransform();

	SpawnTransientNiagaraAtLocationLocal(
		NiagaraSystem,
		SpawnTransform.GetLocation(),
		SpawnTransform.Rotator(),
		SpawnTransform.GetScale3D());
}

UNiagaraComponent* UPRFaerinTeleportVFXComponent::SpawnTeleportVFXLocal(const FVector& Location) const
{
	if (!IsValid(TeleportVFXNiagaraSystem))
	{
		return nullptr;
	}

	return UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this,
		TeleportVFXNiagaraSystem,
		Location,
		FRotator::ZeroRotator,
		FVector::OneVector,
		false,
		true);
}

UNiagaraComponent* UPRFaerinTeleportVFXComponent::SpawnTransientNiagaraAtLocationLocal(
	UNiagaraSystem* NiagaraSystem,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale,
	const float LifeSeconds)
{
	if (!IsValid(NiagaraSystem))
	{
		return nullptr;
	}

	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this,
		NiagaraSystem,
		Location,
		Rotation,
		Scale,
		false,
		true);

	if (!IsValid(NiagaraComponent))
	{
		return nullptr;
	}

	LocalTransientNiagaraComponents.Add(NiagaraComponent);

	const float ResolvedLifeSeconds = LifeSeconds >= 0.0f
		? LifeSeconds
		: TransientNiagaraLifeSeconds;
	if (ResolvedLifeSeconds > UE_SMALL_NUMBER)
	{
		if (UWorld* World = GetWorld())
		{
			TWeakObjectPtr<UNiagaraComponent> WeakNiagaraComponent = NiagaraComponent;
			FTimerHandle TransientDestroyTimerHandle;
			World->GetTimerManager().SetTimer(
				TransientDestroyTimerHandle,
				FTimerDelegate::CreateWeakLambda(
					this,
					[this, WeakNiagaraComponent]()
					{
						if (UNiagaraComponent* ActiveNiagaraComponent = WeakNiagaraComponent.Get())
						{
							ActiveNiagaraComponent->Deactivate();
							ActiveNiagaraComponent->DestroyComponent();
						}

						LocalTransientNiagaraComponents.RemoveAllSwap(
							[WeakNiagaraComponent](const TObjectPtr<UNiagaraComponent>& CandidateComponent)
							{
								return !IsValid(CandidateComponent)
									|| CandidateComponent.Get() == WeakNiagaraComponent.Get();
							});
					}),
				ResolvedLifeSeconds,
				false);
		}
	}

	return NiagaraComponent;
}

void UPRFaerinTeleportVFXComponent::PlayMontageLocal(UAnimMontage* Montage, const float PlayRate) const
{
	const APREnemyBaseCharacter* OwnerEnemy = GetOwnerEnemy();
	if (!IsValid(OwnerEnemy) || !IsValid(OwnerEnemy->GetMesh()) || !IsValid(Montage))
	{
		return;
	}

	UAnimInstance* AnimInstance = OwnerEnemy->GetMesh()->GetAnimInstance();
	if (!IsValid(AnimInstance))
	{
		return;
	}

	AnimInstance->Montage_Play(Montage, FMath::Max(PlayRate, UE_SMALL_NUMBER));
}

void UPRFaerinTeleportVFXComponent::StartDissolveLocal(
	const float StartValue,
	const float EndValue,
	const float DurationSeconds)
{
	if (!bUseMaterialDissolve && !IsValid(DissolveNiagaraSystem))
	{
		SetDissolveValueLocal(EndValue);
		return;
	}

	if (bUseMaterialDissolve)
	{
		PrepareDissolveMaterialsLocal();
	}
	StartDissolveNiagaraLocal(StartValue);

	LocalDissolveStartValue = StartValue;
	LocalDissolveEndValue = EndValue;
	LocalDissolveDurationSeconds = FMath::Max(DurationSeconds, 0.0f);
	LocalDissolveElapsedSeconds = 0.0f;

	SetDissolveValueLocal(StartValue);
	if (LocalDissolveDurationSeconds <= UE_SMALL_NUMBER)
	{
		SetDissolveValueLocal(EndValue);
		CleanupDissolveNiagaraLocal();
		bLocalDissolveActive = false;
		SetComponentTickEnabled(bLocalPresentationActive);
		return;
	}

	bLocalDissolveActive = true;
	SetComponentTickEnabled(true);
}

void UPRFaerinTeleportVFXComponent::PrepareDissolveMaterialsLocal()
{
	if (!bUseMaterialDissolve || !LocalDissolveDynamicMaterials.IsEmpty())
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	TArray<UMeshComponent*> MeshComponents;
	OwnerActor->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!IsValid(MeshComponent))
		{
			continue;
		}

		const int32 MaterialCount = MeshComponent->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			UMaterialInterface* CurrentMaterial = MeshComponent->GetMaterial(MaterialIndex);
			if (!IsValid(CurrentMaterial))
			{
				continue;
			}

			UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(CurrentMaterial);
			if (!IsValid(DynamicMaterial))
			{
				DynamicMaterial = MeshComponent->CreateDynamicMaterialInstance(MaterialIndex, CurrentMaterial);
			}

			if (IsValid(DynamicMaterial))
			{
				LocalDissolveDynamicMaterials.AddUnique(DynamicMaterial);
			}
		}
	}
}

void UPRFaerinTeleportVFXComponent::StartDissolveNiagaraLocal(const float StartValue)
{
	CleanupDissolveNiagaraLocal();

	APREnemyBaseCharacter* OwnerEnemy = GetOwnerEnemy();
	USkeletalMeshComponent* MeshComponent = IsValid(OwnerEnemy) ? OwnerEnemy->GetMesh() : nullptr;
	if (!IsValid(DissolveNiagaraSystem) || !IsValid(MeshComponent))
	{
		return;
	}

	ActiveDissolveNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		DissolveNiagaraSystem,
		MeshComponent,
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		false,
		true);

	if (!IsValid(ActiveDissolveNiagaraComponent))
	{
		return;
	}

	ActiveDissolveNiagaraComponent->SetVariableFloat(NiagaraDissolveParameterName, StartValue);
	if (IsValid(DissolveTexture))
	{
		ActiveDissolveNiagaraComponent->SetVariableTexture(TEXT("User.DissolveTexture"), DissolveTexture);
	}
	ActiveDissolveNiagaraComponent->SetVariableVec2(TEXT("User.DissolveTextureUV"), DissolveTextureUV);
	ActiveDissolveNiagaraComponent->Activate(true);
}

void UPRFaerinTeleportVFXComponent::CleanupDissolveNiagaraLocal()
{
	if (IsValid(ActiveDissolveNiagaraComponent))
	{
		ActiveDissolveNiagaraComponent->Deactivate();
		ActiveDissolveNiagaraComponent->DestroyComponent();
	}

	ActiveDissolveNiagaraComponent = nullptr;
}

void UPRFaerinTeleportVFXComponent::SetDissolveValueLocal(const float DissolveValue)
{
	if (bUseMaterialDissolve)
	{
		PrepareDissolveMaterialsLocal();
		for (UMaterialInstanceDynamic* DynamicMaterial : LocalDissolveDynamicMaterials)
		{
			if (IsValid(DynamicMaterial))
			{
				DynamicMaterial->SetScalarParameterValue(DissolveScalarParameterName, DissolveValue);
			}
		}
	}

	if (IsValid(ActiveDissolveNiagaraComponent))
	{
		ActiveDissolveNiagaraComponent->SetVariableFloat(NiagaraDissolveParameterName, DissolveValue);
	}
}

void UPRFaerinTeleportVFXComponent::CleanupDissolveMaterialsLocal()
{
	LocalDissolveDynamicMaterials.Reset();
	CleanupDissolveNiagaraLocal();
	bLocalDissolveActive = false;
}

void UPRFaerinTeleportVFXComponent::MulticastStartTeleportOutPresentation_Implementation(
	FVector BossStartLocation,
	FRotator BossStartRotation)
{
	StartTeleportOutPresentationLocal(BossStartLocation, BossStartRotation);
}

void UPRFaerinTeleportVFXComponent::MulticastBeginHiddenTeleportVFXPresentation_Implementation(
	FVector BossStartLocation,
	FRotator BossStartRotation,
	FVector ConvergeLocation,
	FRotator ConvergeRotation,
	FVector LeftStartLocation,
	FVector LeftWanderLocation,
	FVector RightStartLocation,
	FVector RightWanderLocation,
	float InWanderSeconds,
	float InConvergeSeconds,
	bool bInDisableCollisionWhileHidden)
{
	(void)BossStartLocation;
	(void)BossStartRotation;
	(void)ConvergeRotation;

	BeginHiddenPresentationLocal(
		ConvergeLocation,
		ConvergeRotation,
		LeftStartLocation,
		LeftWanderLocation,
		RightStartLocation,
		RightWanderLocation,
		InWanderSeconds,
		InConvergeSeconds,
		bInDisableCollisionWhileHidden);
}

void UPRFaerinTeleportVFXComponent::MulticastFinishTeleportVFXPresentation_Implementation(
	FVector FinalLocation,
	FRotator FinalRotation,
	bool bPlayTeleportInStage)
{
	FinishTeleportPresentationLocal(FinalLocation, FinalRotation, bPlayTeleportInStage);
}

void UPRFaerinTeleportVFXComponent::MulticastRevealSplashPresentation_Implementation(
	FVector SplashLocation,
	FRotator SplashRotation,
	float SplashRadius)
{
	PlayRevealSplashPresentationLocal(SplashLocation, SplashRotation, SplashRadius);
}

void UPRFaerinTeleportVFXComponent::MulticastCancelTeleportVFXPresentation_Implementation(
	FVector FinalLocation,
	FRotator FinalRotation)
{
	CancelTeleportPresentationLocal(FinalLocation, FinalRotation);
}
