// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 God Fall Static 검격 Actor 구현)
#include "PRFaerinGodFallStaticSwordActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AI/Data/PRFaerinGodFallDataAsset.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PRAssetManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRFaerinGodFallSword, Log, All);

namespace
{
	FVector ResolveGodFallLocalAxisVector(const EPRFaerinGodFallLocalAxis Axis)
	{
		switch (Axis)
		{
		case EPRFaerinGodFallLocalAxis::PlusX:
			return FVector::ForwardVector;
		case EPRFaerinGodFallLocalAxis::MinusX:
			return -FVector::ForwardVector;
		case EPRFaerinGodFallLocalAxis::PlusY:
			return FVector::RightVector;
		case EPRFaerinGodFallLocalAxis::MinusY:
			return -FVector::RightVector;
		case EPRFaerinGodFallLocalAxis::PlusZ:
			return FVector::UpVector;
		case EPRFaerinGodFallLocalAxis::MinusZ:
			return -FVector::UpVector;
		default:
			return FVector::UpVector;
		}
	}

	FVector ProjectDirectionOnPlaneSafe(const FVector& Direction, const FVector& PlaneNormal, const FVector& Fallback)
	{
		const FVector Normal = PlaneNormal.GetSafeNormal();
		FVector Projected = Direction - FVector::DotProduct(Direction, Normal) * Normal;
		if (!Projected.Normalize())
		{
			Projected = Fallback - FVector::DotProduct(Fallback, Normal) * Normal;
			if (!Projected.Normalize())
			{
				return FVector::ForwardVector;
			}
		}

		return Projected;
	}

	FQuat MakeQuatMappingLocalAxesToWorldAxes(
		FVector LocalBladeAxis,
		FVector LocalFaceAxis,
		FVector DesiredBladeWorld,
		FVector DesiredFaceWorld)
	{
		LocalBladeAxis = LocalBladeAxis.GetSafeNormal();
		LocalFaceAxis = ProjectDirectionOnPlaneSafe(LocalFaceAxis, LocalBladeAxis, FVector::RightVector);
		DesiredBladeWorld = DesiredBladeWorld.GetSafeNormal();
		DesiredFaceWorld = ProjectDirectionOnPlaneSafe(DesiredFaceWorld, DesiredBladeWorld, FVector::RightVector);

		FQuat Result = FQuat::FindBetweenNormals(LocalBladeAxis, DesiredBladeWorld);
		const FVector RotatedFaceAxis = Result.RotateVector(LocalFaceAxis).GetSafeNormal();
		const float TwistSin = FVector::DotProduct(FVector::CrossProduct(RotatedFaceAxis, DesiredFaceWorld), DesiredBladeWorld);
		const float TwistCos = FVector::DotProduct(RotatedFaceAxis, DesiredFaceWorld);
		const float TwistRadians = FMath::Atan2(TwistSin, TwistCos);
		Result = FQuat(DesiredBladeWorld, TwistRadians) * Result;
		Result.Normalize();

#if DO_CHECK
		const FVector TestBlade = Result.RotateVector(LocalBladeAxis).GetSafeNormal();
		const FVector TestFace = Result.RotateVector(LocalFaceAxis).GetSafeNormal();
		ensureMsgf(
			FVector::DotProduct(TestBlade, DesiredBladeWorld) > 0.98f,
			TEXT("GodFall EntryOrbit cone rotation blade axis mapping failed."));
		ensureMsgf(
			FVector::DotProduct(TestFace, DesiredFaceWorld) > 0.98f,
			TEXT("GodFall EntryOrbit cone rotation face axis mapping failed."));
#endif

		return Result;
	}
}

// ===== 생성 =====

APRFaerinGodFallStaticSwordActor::APRFaerinGodFallStaticSwordActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PatternLifeSpan = 0.0f;
	SetReplicateMovement(false);
	bAlwaysRelevant = true;
	SetNetDormancy(DORM_Awake);

	StaticSwordMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticSwordMeshComponent"));
	StaticSwordMeshComponent->SetupAttachment(SceneRoot);
	StaticSwordMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// ===== AActor Interface =====

void APRFaerinGodFallStaticSwordActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateSwordVisualPresentation(DeltaSeconds);

	if (!HasAuthority())
	{
		UpdateClientSwordPresentation(DeltaSeconds);
		RefreshSwordTickEnabled();
		return;
	}

	if (SwordState == EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead)
	{
		UpdateTargetOverheadMovement(DeltaSeconds);
		RefreshSwordTickEnabled();
		return;
	}

	if (SwordState == EPRFaerinGodFallStaticSwordState::EntryOrbitStartDelay
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryOrbitGathering
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryOrbitPreSpinHold
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryOrbiting
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryOrbitPostSpinHold)
	{
		UpdateEntryOrbitMovement(DeltaSeconds);
		RefreshSwordTickEnabled();
		return;
	}

	if (SwordState == EPRFaerinGodFallStaticSwordState::EntryStraightening)
	{
		UpdateEntryStraightening(DeltaSeconds);
		RefreshSwordTickEnabled();
		return;
	}

	if (SwordState == EPRFaerinGodFallStaticSwordState::EntryDiving
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryDiveReturning
		|| SwordState == EPRFaerinGodFallStaticSwordState::Dropping
		|| SwordState == EPRFaerinGodFallStaticSwordState::Returning)
	{
		UpdateSegmentMovement(DeltaSeconds);
	}

	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRFaerinGodFallStaticSwordActor, SwordState);
	DOREPLIFETIME(APRFaerinGodFallStaticSwordActor, SwordIndex);
	DOREPLIFETIME(APRFaerinGodFallStaticSwordActor, SourceBoneName);
	DOREPLIFETIME(APRFaerinGodFallStaticSwordActor, HomeLocation);
	DOREPLIFETIME(APRFaerinGodFallStaticSwordActor, GodFallData);
}

void APRFaerinGodFallStaticSwordActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearSwordTimers();
	ResetSwordVisualTransform();
	Super::EndPlay(EndPlayReason);
}

// ===== APRBossPatternActor Interface =====

void APRFaerinGodFallStaticSwordActor::CancelPatternActor()
{
	if (!HasAuthority())
	{
		return;
	}

	ClearSwordTimers();
	bImpactWarningSpawned = false;
	bEntryOrbitImpactDropActive = false;
	bEntryOrbitImpactSpreadActive = false;
	ResetSwordVisualTransform();
	SetSwordState(EPRFaerinGodFallStaticSwordState::Cancelled);
	MulticastGodFallSwordCancelled();
	ExpirePatternActor();
}

// ===== 초기화 / 충전 =====

void APRFaerinGodFallStaticSwordActor::InitializeGodFallStaticSword(APRBossBaseCharacter* InOwnerBoss,
	AActor* InPatternTarget,
	UPRFaerinGodFallDataAsset* InGodFallData,
	const int32 InSwordIndex,
	const FName InSourceBoneName,
	const FTransform& InInitialTransform)
{
	if (!HasAuthority())
	{
		return;
	}

	GodFallData = InGodFallData;
	SwordIndex = InSwordIndex;
	SourceBoneName = InSourceBoneName;
	HomeLocation = InInitialTransform.GetLocation();

	SetActorTransform(InInitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
	CaptureMeshBaselineTransform();
	ResetSwordVisualTransform();
	MulticastSetSwordPresentationLocation(InInitialTransform.GetLocation());
	InitializeBossPatternActor(InOwnerBoss, InPatternTarget);
	SetSwordState(EPRFaerinGodFallStaticSwordState::SpawnedFromRigBone);
	ForceNetUpdate();
}

void APRFaerinGodFallStaticSwordActor::BeginCharging(const float ChargeSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	ClearSwordTimers();
	PatternTarget = nullptr;
	bHasAssignedAttackLocation = false;
	bImpactWarningSpawned = false;
	bEntryOrbitImpactDropActive = false;
	bEntryOrbitImpactSpreadActive = false;
	OverheadMoveSpeed = 0.0f;
	ResetSwordVisualTransform();
	SetActorRotation(FRotator::ZeroRotator);
	ApplySwordPresentationLocation(HomeLocation);
	MulticastSetSwordPresentationLocation(HomeLocation);
	SetSwordState(EPRFaerinGodFallStaticSwordState::Charging);

	if (ChargeSeconds <= 0.0f)
	{
		FinishCharging();
		return;
	}

	GetWorldTimerManager().SetTimer(
		ChargeTimerHandle,
		this,
		&APRFaerinGodFallStaticSwordActor::FinishCharging,
		ChargeSeconds,
		false);
}

void APRFaerinGodFallStaticSwordActor::FinishCharging()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplySwordPresentationLocation(HomeLocation);
	SetActorRotation(FRotator::ZeroRotator);
	MulticastSetSwordPresentationLocation(HomeLocation);
	ResetSwordVisualTransform();
	SetSwordState(EPRFaerinGodFallStaticSwordState::Charged);
	OnChargeFinished.Broadcast(this);
}

bool APRFaerinGodFallStaticSwordActor::StartEntryDive(const float DiveDistance,
	const float DiveSeconds,
	const float ReturnSeconds,
	const float ChargeSecondsAfterReturn)
{
	if (!HasAuthority()
		|| SwordState == EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead
		|| SwordState == EPRFaerinGodFallStaticSwordState::Telegraph
		|| SwordState == EPRFaerinGodFallStaticSwordState::Dropping
		|| SwordState == EPRFaerinGodFallStaticSwordState::Impact
		|| SwordState == EPRFaerinGodFallStaticSwordState::Returning)
	{
		return false;
	}

	ClearSwordTimers();
	PatternTarget = nullptr;
	bHasAssignedAttackLocation = false;
	bImpactWarningSpawned = false;
	bEntryOrbitImpactDropActive = false;
	bEntryOrbitImpactSpreadActive = false;
	EntryDiveReturnSeconds = FMath::Max(ReturnSeconds, 0.0f);
	EntryDiveChargeSecondsAfterReturn = FMath::Max(ChargeSecondsAfterReturn, 0.0f);

	SegmentStartLocation = GetActorLocation();
	SegmentTargetLocation = HomeLocation - FVector(0.0f, 0.0f, FMath::Max(DiveDistance, 0.0f));
	SegmentElapsedSeconds = 0.0f;
	SegmentDurationSeconds = FMath::Max(DiveSeconds, 0.0f);

	SetSwordState(EPRFaerinGodFallStaticSwordState::EntryDiving);
	MulticastStartSwordPresentationSegment(
		EPRFaerinGodFallStaticSwordState::EntryDiving,
		SegmentStartLocation,
		SegmentTargetLocation,
		SegmentDurationSeconds);
	if (SegmentDurationSeconds <= UE_SMALL_NUMBER)
	{
		FinishEntryDiveDown();
		return true;
	}

	RefreshSwordTickEnabled();
	return true;
}


bool APRFaerinGodFallStaticSwordActor::StartEntryOrbit(
	const FVector& OrbitCenterLocation,
	const float StartDelaySeconds,
	const float GatherDurationSeconds,
	const float PreSpinHoldSeconds,
	const float OrbitDurationSeconds,
	const float PostSpinHoldSeconds)
{
	if (!HasAuthority()
		|| !IsValid(GodFallData)
		|| SwordState == EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead
		|| SwordState == EPRFaerinGodFallStaticSwordState::Telegraph
		|| SwordState == EPRFaerinGodFallStaticSwordState::Dropping
		|| SwordState == EPRFaerinGodFallStaticSwordState::Impact
		|| SwordState == EPRFaerinGodFallStaticSwordState::Returning)
	{
		return false;
	}

	ClearSwordTimers();
	PatternTarget = nullptr;
	bHasAssignedAttackLocation = false;
	bImpactWarningSpawned = false;
	EntryOrbitCenterLocation = OrbitCenterLocation;
	EntryOrbitCenterLocation.Z += GodFallData->EntryOrbitHeightOffset;
	EntryOrbitGatherStartLocation = GetActorLocation();
	EntryOrbitElapsedSeconds = 0.0f;
	EntryOrbitStartDelaySeconds = FMath::Max(StartDelaySeconds, 0.0f);
	EntryOrbitGatherDurationSeconds = FMath::Max(GatherDurationSeconds, 0.0f);
	EntryOrbitPreSpinHoldSeconds = FMath::Max(PreSpinHoldSeconds, 0.0f);
	EntryOrbitDurationSeconds = FMath::Max(OrbitDurationSeconds, 0.0f);
	EntryOrbitPostSpinHoldSeconds = FMath::Max(PostSpinHoldSeconds, 0.0f);

	const float SpawnServerWorldTimeSeconds = ResolveServerWorldTimeSeconds();
	MulticastStartSwordEntryOrbit(
		EntryOrbitCenterLocation,
		EntryOrbitGatherStartLocation,
		EntryOrbitStartDelaySeconds,
		EntryOrbitGatherDurationSeconds,
		EntryOrbitPreSpinHoldSeconds,
		EntryOrbitDurationSeconds,
		EntryOrbitPostSpinHoldSeconds,
		SpawnServerWorldTimeSeconds);

	if (EntryOrbitStartDelaySeconds > UE_SMALL_NUMBER)
	{
		SetSwordState(EPRFaerinGodFallStaticSwordState::EntryOrbitStartDelay);
		ApplySwordPresentationLocation(EntryOrbitGatherStartLocation);
	}
	else if (EntryOrbitGatherDurationSeconds > UE_SMALL_NUMBER)
	{
		SetSwordState(EPRFaerinGodFallStaticSwordState::EntryOrbitGathering);
		ApplySwordPresentationLocation(EntryOrbitGatherStartLocation);
	}
	else if (EntryOrbitPreSpinHoldSeconds > UE_SMALL_NUMBER)
	{
		SetSwordState(EPRFaerinGodFallStaticSwordState::EntryOrbitPreSpinHold);
		ApplySwordPresentationLocation(ResolveEntryOrbitLocation(0.0f));
	}
	else if (EntryOrbitDurationSeconds > UE_SMALL_NUMBER)
	{
		SetSwordState(EPRFaerinGodFallStaticSwordState::EntryOrbiting);
		ApplySwordPresentationLocation(ResolveEntryOrbitLocation(0.0f));
	}
	else
	{
		SetSwordState(EPRFaerinGodFallStaticSwordState::EntryOrbitPostSpinHold);
		ApplySwordPresentationLocation(ResolveEntryOrbitLocation(EntryOrbitDurationSeconds));
	}

	RefreshSwordTickEnabled();
	return true;
}

bool APRFaerinGodFallStaticSwordActor::StartEntryOrbitImpactDrop(
	const float DropSeconds,
	const float ImpactHoldSeconds,
	const float RiseSecondsAfterStraighten,
	const float ChargeStartDelayAfterRise,
	const float ChargeSecondsAfterReturn)
{
	if (!HasAuthority()
		|| !IsValid(GodFallData)
		|| (SwordState != EPRFaerinGodFallStaticSwordState::EntryOrbitStartDelay
			&& SwordState != EPRFaerinGodFallStaticSwordState::EntryOrbitGathering
			&& SwordState != EPRFaerinGodFallStaticSwordState::EntryOrbitPreSpinHold
			&& SwordState != EPRFaerinGodFallStaticSwordState::EntryOrbiting
			&& SwordState != EPRFaerinGodFallStaticSwordState::EntryOrbitPostSpinHold
			&& SwordState != EPRFaerinGodFallStaticSwordState::SpawnedFromRigBone
			&& SwordState != EPRFaerinGodFallStaticSwordState::EntryDiving))
	{
		return false;
	}

	ClearSwordTimers();
	bEntryOrbitImpactDropActive = true;
	bEntryOrbitImpactSpreadActive = GodFallData->bUseEntryOrbitImpactSpread;
	EntryDiveReturnSeconds = FMath::Max(RiseSecondsAfterStraighten, 0.0f);
	EntryImpactHoldSeconds = FMath::Max(ImpactHoldSeconds, 0.0f);
	EntryChargeStartDelayAfterRise = FMath::Max(ChargeStartDelayAfterRise, 0.0f);
	EntryDiveChargeSecondsAfterReturn = FMath::Max(ChargeSecondsAfterReturn, 0.0f);

	// Drop 직전 최종 orbit 위치를 한 번 더 고정한다. 타이머가 조금 빨리 호출돼도 최소 반경/최대 기울기 상태에서 떨어지게 한다.
	ApplySwordPresentationLocation(ResolveEntryOrbitLocation(EntryOrbitDurationSeconds));

	FVector ImpactLocation = FVector::ZeroVector;
	if (!ResolveEntryOrbitImpactLocation(ImpactLocation))
	{
		ImpactLocation = GetActorLocation() - FVector(0.0f, 0.0f, FMath::Max(GodFallData->EntrySwordDiveDistance, 0.0f));
	}

	SegmentStartLocation = GetActorLocation();
	SegmentTargetLocation = ImpactLocation;
	SegmentElapsedSeconds = 0.0f;
	SegmentDurationSeconds = FMath::Max(DropSeconds, 0.0f);

	SetSwordState(EPRFaerinGodFallStaticSwordState::EntryDiving);
	MulticastStartSwordPresentationSegment(
		EPRFaerinGodFallStaticSwordState::EntryDiving,
		SegmentStartLocation,
		SegmentTargetLocation,
		SegmentDurationSeconds);

	if (SegmentDurationSeconds <= UE_SMALL_NUMBER)
	{
		FinishEntryOrbitImpactDrop();
		return true;
	}

	RefreshSwordTickEnabled();
	return true;
}

bool APRFaerinGodFallStaticSwordActor::StartAssignedAttack(AActor* InAssignedTarget,
	const float InWarningSeconds,
	const float InOverheadMoveSeconds)
{
	if (!HasAuthority() || !CanStartAssignedAttack() || !IsValidAssignedTarget(InAssignedTarget))
	{
		return false;
	}

	PatternTarget = InAssignedTarget;

	FVector ResolvedOverheadLocation = FVector::ZeroVector;
	FVector ResolvedImpactLocation = FVector::ZeroVector;
	if (!ResolveAssignedAttackLocations(ResolvedOverheadLocation, ResolvedImpactLocation))
	{
		UE_LOG(LogPRFaerinGodFallSword, Warning,
			TEXT("God Fall sword failed to resolve assigned attack locations. Sword=%s, Target=%s"),
			*GetNameSafe(this),
			*GetNameSafe(InAssignedTarget));
		PatternTarget = nullptr;
		return false;
	}

	ClearSwordTimers();
	bImpactWarningSpawned = false;
	bEntryOrbitImpactDropActive = false;
	bEntryOrbitImpactSpreadActive = false;
	AssignedOverheadLocation = ResolvedOverheadLocation;
	AssignedImpactLocation = ResolvedImpactLocation;
	AssignedWarningSeconds = FMath::Max(InWarningSeconds, 0.0f);
	bHasAssignedAttackLocation = true;

	if (IsValid(GodFallData) && GodFallData->bDrawDebug)
	{
		DrawDebugLine(GetWorld(), HomeLocation, AssignedOverheadLocation, FColor::Cyan, false, FMath::Max(InOverheadMoveSeconds, 0.05f));
		DrawDebugSphere(GetWorld(), AssignedOverheadLocation, 56.0f, 12, FColor::Cyan, false, FMath::Max(InWarningSeconds, 0.05f));
	}

	SegmentStartLocation = GetActorLocation();
	SegmentElapsedSeconds = 0.0f;
	SegmentDurationSeconds = 0.0f;
	OverheadMoveElapsedSeconds = 0.0f;
	OverheadMoveDurationSeconds = FMath::Max(InOverheadMoveSeconds, 0.0f);
	OverheadMoveSpeed = 0.0f;
	if (IsValid(GodFallData))
	{
		const float DurationScale = GodFallData->TargetOverheadMoveSeconds > UE_SMALL_NUMBER
			? OverheadMoveDurationSeconds / GodFallData->TargetOverheadMoveSeconds
			: 1.0f;
		OverheadMoveAcceleration = FMath::Max(GodFallData->TargetOverheadMoveAcceleration, 0.0f)
			/ FMath::Max(DurationScale, 0.01f);
	}
	else
	{
		OverheadMoveAcceleration = 0.0f;
	}

	SetSwordState(EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead);
	MulticastStartSwordTargetOverhead(
		InAssignedTarget,
		AssignedOverheadLocation,
		OverheadMoveDurationSeconds,
		OverheadMoveAcceleration);
	if (OverheadMoveDurationSeconds <= UE_SMALL_NUMBER)
	{
		FinishMoveToTargetOverhead();
		return true;
	}

	RefreshSwordTickEnabled();
	return true;
}

// ===== 복제 / BP 이벤트 =====

void APRFaerinGodFallStaticSwordActor::OnRep_SwordState()
{
	VisualStateElapsedSeconds = 0.0f;
	if (SwordState == EPRFaerinGodFallStaticSwordState::Charging
		|| SwordState == EPRFaerinGodFallStaticSwordState::Charged
		|| SwordState == EPRFaerinGodFallStaticSwordState::Returning
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryDiveReturning
		|| SwordState == EPRFaerinGodFallStaticSwordState::Cancelled
		|| SwordState == EPRFaerinGodFallStaticSwordState::None)
	{
		GetWorldTimerManager().ClearTimer(ImpactWarningTimerHandle);
		bImpactWarningSpawned = false;
		ResetSwordVisualTransform();
	}
	BP_OnGodFallSwordStateChanged(SwordState);
	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::OnRep_GodFallData()
{
	if (!bMeshBaselineCaptured)
	{
		CaptureMeshBaselineTransform();
	}
	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::MulticastGodFallSwordImpact_Implementation(
	FVector_NetQuantize ImpactLocation,
	FRotator ImpactRotation,
	UNiagaraSystem* NiagaraSystem,
	FVector Scale,
	float LifeSeconds)
{
	SpawnNiagaraAtLocationLocal(NiagaraSystem, ImpactLocation, ImpactRotation, Scale, LifeSeconds);
	BP_OnGodFallSwordImpact();
}

void APRFaerinGodFallStaticSwordActor::MulticastScheduleGodFallSwordImpactWarning_Implementation(
	FVector_NetQuantize ImpactLocation,
	FRotator ImpactRotation,
	FSoftObjectPath NiagaraSystemPath,
	FVector Scale,
	float LifeSeconds,
	float SpawnServerWorldTimeSeconds)
{
	if (bImpactWarningSpawned || !NiagaraSystemPath.IsValid())
	{
		return;
	}

	const float DelaySeconds = FMath::Max(SpawnServerWorldTimeSeconds - ResolveServerWorldTimeSeconds(), 0.0f);
	ScheduleImpactWarningLocal(ImpactLocation, DelaySeconds, ImpactRotation, NiagaraSystemPath, Scale, LifeSeconds);
}

void APRFaerinGodFallStaticSwordActor::MulticastGodFallSwordCancelled_Implementation()
{
	ResetSwordVisualTransform();
	BP_OnGodFallSwordCancelled();
}

// ===== 상태 전이 =====

void APRFaerinGodFallStaticSwordActor::SetSwordState(const EPRFaerinGodFallStaticSwordState NewState)
{
	if (SwordState == NewState)
	{
		return;
	}

	SwordState = NewState;
	VisualStateElapsedSeconds = 0.0f;
	if (SwordState == EPRFaerinGodFallStaticSwordState::Charging
		|| SwordState == EPRFaerinGodFallStaticSwordState::Charged
		|| SwordState == EPRFaerinGodFallStaticSwordState::Returning
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryDiveReturning
		|| SwordState == EPRFaerinGodFallStaticSwordState::Cancelled
		|| SwordState == EPRFaerinGodFallStaticSwordState::None)
	{
		GetWorldTimerManager().ClearTimer(ImpactWarningTimerHandle);
		bImpactWarningSpawned = false;
		ResetSwordVisualTransform();
	}
	BP_OnGodFallSwordStateChanged(SwordState);
	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::ClearSwordTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChargeTimerHandle);
		World->GetTimerManager().ClearTimer(TelegraphTimerHandle);
		World->GetTimerManager().ClearTimer(ImpactHoldTimerHandle);
		World->GetTimerManager().ClearTimer(ImpactWarningTimerHandle);
		World->GetTimerManager().ClearTimer(EntryReturnChargeDelayTimerHandle);
		World->GetTimerManager().ClearTimer(StrikeSoundTimerHandle);
	}
}

void APRFaerinGodFallStaticSwordActor::FinishEntryDiveDown()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplySwordPresentationLocation(SegmentTargetLocation);
	ApplyImpactDamage();
	const FRotator ImpactRotation = IsValid(GodFallData)
		? GodFallData->SwordImpactNiagaraRotationOffset
		: FRotator::ZeroRotator;
	UNiagaraSystem* ImpactNiagaraSystem = IsValid(GodFallData) ? GodFallData->SwordImpactNiagaraSystem : nullptr;
	const FVector ImpactNiagaraScale = IsValid(GodFallData) ? GodFallData->SwordImpactNiagaraScale : FVector::OneVector;
	const float ImpactNiagaraLifeSeconds = IsValid(GodFallData) ? GodFallData->SwordImpactNiagaraLifeSeconds : 0.0f;
	MulticastGodFallSwordImpact(SegmentTargetLocation, ImpactRotation, ImpactNiagaraSystem, ImpactNiagaraScale, ImpactNiagaraLifeSeconds);

	SegmentStartLocation = GetActorLocation();
	SegmentTargetLocation = HomeLocation;
	SegmentElapsedSeconds = 0.0f;
	SegmentDurationSeconds = EntryDiveReturnSeconds;

	SetSwordState(EPRFaerinGodFallStaticSwordState::EntryDiveReturning);
	MulticastStartSwordPresentationSegment(
		EPRFaerinGodFallStaticSwordState::EntryDiveReturning,
		SegmentStartLocation,
		SegmentTargetLocation,
		SegmentDurationSeconds);
	if (SegmentDurationSeconds <= UE_SMALL_NUMBER)
	{
		FinishEntryDiveReturn();
		return;
	}

	RefreshSwordTickEnabled();
}


void APRFaerinGodFallStaticSwordActor::FinishEntryOrbitImpactDrop()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplySwordPresentationLocation(SegmentTargetLocation);
	bEntryOrbitImpactSpreadActive = false;
	MulticastSetSwordPresentationLocation(SegmentTargetLocation);
	SetSwordState(EPRFaerinGodFallStaticSwordState::EntryImpact);

	const FRotator ImpactRotation = IsValid(GodFallData)
		? GodFallData->SwordImpactNiagaraRotationOffset
		: FRotator::ZeroRotator;
	UNiagaraSystem* ImpactNiagaraSystem = IsValid(GodFallData) ? GodFallData->SwordImpactNiagaraSystem : nullptr;
	const FVector ImpactNiagaraScale = IsValid(GodFallData) ? GodFallData->SwordImpactNiagaraScale : FVector::OneVector;
	const float ImpactNiagaraLifeSeconds = IsValid(GodFallData) ? GodFallData->SwordImpactNiagaraLifeSeconds : 0.0f;
	MulticastGodFallSwordImpact(SegmentTargetLocation, ImpactRotation, ImpactNiagaraSystem, ImpactNiagaraScale, ImpactNiagaraLifeSeconds);
	OnEntryImpactFinished.Broadcast(this);

	const float HoldSeconds = EntryImpactHoldSeconds;
	if (HoldSeconds <= 0.0f)
	{
		BeginEntryOrbitStraightening();
		return;
	}

	GetWorldTimerManager().SetTimer(
		ImpactHoldTimerHandle,
		this,
		&APRFaerinGodFallStaticSwordActor::BeginEntryOrbitStraightening,
		HoldSeconds,
		false);
}

void APRFaerinGodFallStaticSwordActor::BeginEntryOrbitStraightening()
{
	if (!HasAuthority())
	{
		return;
	}

	EntryStraightenStartRotation = GetActorRotation();
	EntryStraightenTargetRotation = FRotator::ZeroRotator;
	EntryStraightenElapsedSeconds = 0.0f;
	EntryStraightenDurationSeconds = IsValid(GodFallData)
		? FMath::Max(GodFallData->EntryOrbitReturnStraightenSeconds, 0.0f)
		: 0.0f;
	EntryStraightenEaseExponent = IsValid(GodFallData)
		? FMath::Max(GodFallData->EntryOrbitReturnStraightenExponent, 0.1f)
		: 1.0f;

	if (IsValid(GodFallData) && GodFallData->bRandomizeEntryOrbitReturnTiming)
	{
		const auto ResolveRandomRangeValue = [](const FPRFaerinGodFallFloatRange& Range, const float FallbackValue, const float MinClamp)
		{
			const float ClampedMin = FMath::Max(FMath::Min(Range.Min, Range.Max), MinClamp);
			const float ClampedMax = FMath::Max(FMath::Max(Range.Min, Range.Max), MinClamp);
			if (FMath::IsNearlyEqual(ClampedMin, ClampedMax))
			{
				return ClampedMin;
			}

			if (ClampedMax < MinClamp)
			{
				return FMath::Max(FallbackValue, MinClamp);
			}

			return FMath::FRandRange(ClampedMin, ClampedMax);
		};

		EntryStraightenDurationSeconds = ResolveRandomRangeValue(
			GodFallData->EntryOrbitReturnStraightenSecondsRange,
			EntryStraightenDurationSeconds,
			0.0f);
		EntryStraightenEaseExponent = ResolveRandomRangeValue(
			GodFallData->EntryOrbitReturnStraightenExponentRange,
			EntryStraightenEaseExponent,
			0.1f);
		EntryDiveReturnSeconds = ResolveRandomRangeValue(
			GodFallData->EntryOrbitRiseAfterStraightenSecondsRange,
			EntryDiveReturnSeconds,
			0.0f);
	}

	SetSwordState(EPRFaerinGodFallStaticSwordState::EntryStraightening);
	MulticastStartSwordEntryStraightening(
		EntryStraightenStartRotation,
		EntryStraightenTargetRotation,
		EntryStraightenDurationSeconds,
		EntryStraightenEaseExponent);
	if (EntryStraightenDurationSeconds <= UE_SMALL_NUMBER)
	{
		UpdateEntryStraightening(EntryStraightenDurationSeconds);
		return;
	}

	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::UpdateEntryOrbitMovement(const float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	EntryOrbitElapsedSeconds += DeltaSeconds;

	const float TotalElapsedSeconds = EntryOrbitElapsedSeconds;
	if (TotalElapsedSeconds < EntryOrbitStartDelaySeconds)
	{
		SetSwordState(EPRFaerinGodFallStaticSwordState::EntryOrbitStartDelay);
		ApplySwordPresentationLocation(EntryOrbitGatherStartLocation);
		return;
	}

	const float GatherElapsedSeconds = TotalElapsedSeconds - EntryOrbitStartDelaySeconds;
	if (GatherElapsedSeconds < EntryOrbitGatherDurationSeconds)
	{
		SetSwordState(EPRFaerinGodFallStaticSwordState::EntryOrbitGathering);
		ApplySwordPresentationLocation(ResolveEntryOrbitGatherLocation(GatherElapsedSeconds));
		return;
	}

	const float PreSpinElapsedSeconds = GatherElapsedSeconds - EntryOrbitGatherDurationSeconds;
	if (PreSpinElapsedSeconds < EntryOrbitPreSpinHoldSeconds)
	{
		SetSwordState(EPRFaerinGodFallStaticSwordState::EntryOrbitPreSpinHold);
		ApplySwordPresentationLocation(ResolveEntryOrbitLocation(0.0f));
		return;
	}

	const float SpinElapsedSeconds = PreSpinElapsedSeconds - EntryOrbitPreSpinHoldSeconds;
	if (SpinElapsedSeconds < EntryOrbitDurationSeconds)
	{
		SetSwordState(EPRFaerinGodFallStaticSwordState::EntryOrbiting);
		ApplySwordPresentationLocation(ResolveEntryOrbitLocation(SpinElapsedSeconds));
		return;
	}

	const float PostSpinElapsedSeconds = SpinElapsedSeconds - EntryOrbitDurationSeconds;
	if (PostSpinElapsedSeconds < EntryOrbitPostSpinHoldSeconds)
	{
		SetSwordState(EPRFaerinGodFallStaticSwordState::EntryOrbitPostSpinHold);
		ApplySwordPresentationLocation(ResolveEntryOrbitLocation(EntryOrbitDurationSeconds));
		return;
	}

	SetSwordState(EPRFaerinGodFallStaticSwordState::EntryOrbitPostSpinHold);
	ApplySwordPresentationLocation(ResolveEntryOrbitLocation(EntryOrbitDurationSeconds));
}

void APRFaerinGodFallStaticSwordActor::UpdateEntryStraightening(const float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	EntryStraightenElapsedSeconds += DeltaSeconds;
	const float RawAlpha = FMath::Clamp(
		EntryStraightenElapsedSeconds / FMath::Max(EntryStraightenDurationSeconds, UE_SMALL_NUMBER),
		0.0f,
		1.0f);
	const float Alpha = FMath::Pow(RawAlpha, FMath::Max(EntryStraightenEaseExponent, 0.1f));
	SetActorRotation(FMath::Lerp(EntryStraightenStartRotation, EntryStraightenTargetRotation, Alpha));

	if (RawAlpha < 1.0f)
	{
		return;
	}

	SetActorRotation(EntryStraightenTargetRotation);
	bEntryOrbitImpactDropActive = false;
	SegmentStartLocation = GetActorLocation();
	SegmentTargetLocation = HomeLocation;
	SegmentElapsedSeconds = 0.0f;
	SegmentDurationSeconds = EntryDiveReturnSeconds;

	SetSwordState(EPRFaerinGodFallStaticSwordState::EntryDiveReturning);
	MulticastStartSwordPresentationSegment(
		EPRFaerinGodFallStaticSwordState::EntryDiveReturning,
		SegmentStartLocation,
		SegmentTargetLocation,
		SegmentDurationSeconds);
	if (SegmentDurationSeconds <= UE_SMALL_NUMBER)
	{
		FinishEntryDiveReturn();
		return;
	}

	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::FinishEntryDiveReturn()
{
	if (!HasAuthority())
	{
		return;
	}

	bEntryOrbitImpactDropActive = false;
	bEntryOrbitImpactSpreadActive = false;
	ApplySwordPresentationLocation(HomeLocation);
	SetActorRotation(FRotator::ZeroRotator);
	MulticastSetSwordPresentationLocation(HomeLocation);
	ResetSwordVisualTransform();

	if (EntryChargeStartDelayAfterRise > UE_SMALL_NUMBER)
	{
		GetWorldTimerManager().SetTimer(
			EntryReturnChargeDelayTimerHandle,
			this,
			&APRFaerinGodFallStaticSwordActor::BeginChargingAfterEntryReturnDelay,
			EntryChargeStartDelayAfterRise,
			false);
		return;
	}

	BeginChargingAfterEntryReturnDelay();
}

void APRFaerinGodFallStaticSwordActor::BeginChargingAfterEntryReturnDelay()
{
	if (!HasAuthority())
	{
		return;
	}

	BeginCharging(EntryDiveChargeSecondsAfterReturn);
}

void APRFaerinGodFallStaticSwordActor::BeginDropping()
{
	if (!HasAuthority() || !IsValid(GodFallData))
	{
		return;
	}

	if (!IsValidAssignedTarget(PatternTarget.Get()))
	{
		BeginReturning();
		return;
	}

	if (!bHasAssignedAttackLocation)
	{
		UE_LOG(LogPRFaerinGodFallSword, Warning,
			TEXT("God Fall sword drop skipped because assigned impact location is missing. Sword=%s, Target=%s"),
			*GetNameSafe(this),
			*GetNameSafe(PatternTarget.Get()));
		return;
	}

	SegmentStartLocation = GetActorLocation();
	SegmentTargetLocation = AssignedImpactLocation;
	SegmentElapsedSeconds = 0.0f;
	SegmentDurationSeconds = FMath::Max(GodFallData->DropSeconds, UE_SMALL_NUMBER);

	SetSwordState(EPRFaerinGodFallStaticSwordState::Dropping);
	MulticastStartSwordPresentationSegment(
		EPRFaerinGodFallStaticSwordState::Dropping,
		SegmentStartLocation,
		SegmentTargetLocation,
		SegmentDurationSeconds);
	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::FinishDrop()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplySwordPresentationLocation(SegmentTargetLocation);
	MulticastSetSwordPresentationLocation(SegmentTargetLocation);
	SetSwordState(EPRFaerinGodFallStaticSwordState::Impact);

	ApplyImpactDamage();
	const FRotator ImpactRotation = IsValid(GodFallData)
		? GodFallData->SwordImpactNiagaraRotationOffset
		: FRotator::ZeroRotator;
	UNiagaraSystem* ImpactNiagaraSystem = IsValid(GodFallData) ? GodFallData->SwordImpactNiagaraSystem : nullptr;
	const FVector ImpactNiagaraScale = IsValid(GodFallData) ? GodFallData->SwordImpactNiagaraScale : FVector::OneVector;
	const float ImpactNiagaraLifeSeconds = IsValid(GodFallData) ? GodFallData->SwordImpactNiagaraLifeSeconds : 0.0f;
	MulticastGodFallSwordImpact(SegmentTargetLocation, ImpactRotation, ImpactNiagaraSystem, ImpactNiagaraScale, ImpactNiagaraLifeSeconds);

	const float HoldSeconds = EntryImpactHoldSeconds;
	if (HoldSeconds <= 0.0f)
	{
		BeginReturning();
		return;
	}

	GetWorldTimerManager().SetTimer(
		ImpactHoldTimerHandle,
		this,
		&APRFaerinGodFallStaticSwordActor::BeginReturning,
		HoldSeconds,
		false);
}

void APRFaerinGodFallStaticSwordActor::BeginReturning()
{
	if (!HasAuthority() || !IsValid(GodFallData))
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ImpactWarningTimerHandle);
	SegmentStartLocation = GetActorLocation();
	SegmentTargetLocation = HomeLocation;
	SegmentElapsedSeconds = 0.0f;
	SegmentDurationSeconds = FMath::Max(GodFallData->RiseSeconds, UE_SMALL_NUMBER);

	SetSwordState(EPRFaerinGodFallStaticSwordState::Returning);
	MulticastStartSwordPresentationSegment(
		EPRFaerinGodFallStaticSwordState::Returning,
		SegmentStartLocation,
		SegmentTargetLocation,
		SegmentDurationSeconds);
	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::FinishReturning()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplySwordPresentationLocation(HomeLocation);
	MulticastSetSwordPresentationLocation(HomeLocation);
	ResetSwordVisualTransform();
	RefreshSwordTickEnabled();
	PatternTarget = nullptr;
	OnAssignedAttackFinished.Broadcast(this);
}

// ===== 이동 / 타겟팅 =====

void APRFaerinGodFallStaticSwordActor::UpdateSegmentMovement(const float DeltaSeconds)
{
	if (!IsValid(GodFallData))
	{
		return;
	}

	SegmentElapsedSeconds += DeltaSeconds;
	const float Alpha = FMath::Clamp(SegmentElapsedSeconds / FMath::Max(SegmentDurationSeconds, UE_SMALL_NUMBER), 0.0f, 1.0f);
	const FVector PresentationLocation = (SwordState == EPRFaerinGodFallStaticSwordState::EntryDiving && bEntryOrbitImpactSpreadActive)
		? ResolveEntryOrbitImpactSpreadLocation(Alpha)
		: FMath::Lerp(SegmentStartLocation, SegmentTargetLocation, Alpha);
	ApplySwordPresentationLocation(PresentationLocation);

	if (Alpha >= 1.0f)
	{
		if (SwordState == EPRFaerinGodFallStaticSwordState::EntryDiving)
		{
			if (bEntryOrbitImpactDropActive)
			{
				FinishEntryOrbitImpactDrop();
			}
			else
			{
				FinishEntryDiveDown();
			}
		}
		else if (SwordState == EPRFaerinGodFallStaticSwordState::EntryDiveReturning)
		{
			FinishEntryDiveReturn();
		}
		else if (SwordState == EPRFaerinGodFallStaticSwordState::Dropping)
		{
			FinishDrop();
		}
		else if (SwordState == EPRFaerinGodFallStaticSwordState::Returning)
		{
			FinishReturning();
		}
	}
}

void APRFaerinGodFallStaticSwordActor::UpdateTargetOverheadMovement(const float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValidAssignedTarget(PatternTarget.Get()))
	{
		BeginReturning();
		return;
	}

	RefreshAssignedAttackLocations();
	OverheadMoveElapsedSeconds += DeltaSeconds;
	OverheadMoveSpeed += OverheadMoveAcceleration * DeltaSeconds;

	const FVector CurrentLocation = GetActorLocation();
	const FVector ToTarget = AssignedOverheadLocation - CurrentLocation;
	const float Distance = ToTarget.Size();
	if (Distance > UE_SMALL_NUMBER && OverheadMoveSpeed > 0.0f)
	{
		const float MoveDistance = FMath::Min(OverheadMoveSpeed * DeltaSeconds, Distance);
		ApplySwordPresentationLocation(CurrentLocation + ToTarget.GetSafeNormal() * MoveDistance);
	}

	if (OverheadMoveElapsedSeconds >= OverheadMoveDurationSeconds)
	{
		RefreshAssignedAttackLocations();
		FinishMoveToTargetOverhead();
	}
}

bool APRFaerinGodFallStaticSwordActor::RefreshAssignedAttackLocations()
{
	FVector ResolvedOverheadLocation = FVector::ZeroVector;
	FVector ResolvedImpactLocation = FVector::ZeroVector;
	if (!ResolveAssignedAttackLocations(ResolvedOverheadLocation, ResolvedImpactLocation))
	{
		return false;
	}

	AssignedOverheadLocation = ResolvedOverheadLocation;
	AssignedImpactLocation = ResolvedImpactLocation;
	bHasAssignedAttackLocation = true;
	return true;
}

bool APRFaerinGodFallStaticSwordActor::IsValidAssignedTarget(AActor* CandidateTarget) const
{
	if (!IsValid(CandidateTarget))
	{
		return false;
	}

	const UAbilitySystemComponent* TargetAbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CandidateTarget);
	return IsValid(TargetAbilitySystem)
		&& !TargetAbilitySystem->HasMatchingGameplayTag(PRGameplayTags::State_Down)
		&& !TargetAbilitySystem->HasMatchingGameplayTag(PRGameplayTags::State_Dead);
}

void APRFaerinGodFallStaticSwordActor::FinishMoveToTargetOverhead()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValidAssignedTarget(PatternTarget.Get()))
	{
		BeginReturning();
		return;
	}

	ApplySwordPresentationLocation(AssignedOverheadLocation);
	MulticastSetSwordPresentationLocation(AssignedOverheadLocation);
	RefreshSwordTickEnabled();
	BeginTelegraph();
}

void APRFaerinGodFallStaticSwordActor::BeginTelegraph()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValidAssignedTarget(PatternTarget.Get()))
	{
		BeginReturning();
		return;
	}

	ApplySwordPresentationLocation(AssignedOverheadLocation);
	MulticastSetSwordPresentationLocation(AssignedOverheadLocation);
	SetSwordState(EPRFaerinGodFallStaticSwordState::Telegraph);
	ScheduleImpactWarning();

	if (AssignedWarningSeconds <= 0.0f)
	{
		BeginDropping();
		return;
	}

	GetWorldTimerManager().SetTimer(
		TelegraphTimerHandle,
		this,
		&APRFaerinGodFallStaticSwordActor::BeginDropping,
		AssignedWarningSeconds,
		false);
}

bool APRFaerinGodFallStaticSwordActor::ResolveAssignedAttackLocations(FVector& OutOverheadLocation, FVector& OutGroundLocation) const
{
	if (!ResolveAssignedImpactLocation(OutGroundLocation))
	{
		return false;
	}

	OutOverheadLocation = FVector(OutGroundLocation.X, OutGroundLocation.Y, HomeLocation.Z);
	return true;
}

bool APRFaerinGodFallStaticSwordActor::ResolveAssignedImpactLocation(FVector& OutGroundLocation) const
{
	if (!IsValid(PatternTarget))
	{
		return false;
	}

	return ProjectTargetLocationToGround(PatternTarget->GetActorLocation(), OutGroundLocation);
}

bool APRFaerinGodFallStaticSwordActor::ProjectTargetLocationToGround(const FVector& TargetLocation, FVector& OutGroundLocation) const
{
	if (!IsValid(GodFallData))
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FaerinGodFallGroundTrace), false, this);
	QueryParams.AddIgnoredActor(this);
	if (IsValid(OwnerBoss))
	{
		QueryParams.AddIgnoredActor(OwnerBoss);
	}
	if (IsValid(PatternTarget))
	{
		QueryParams.AddIgnoredActor(PatternTarget);
	}

	const FVector TraceStart(TargetLocation.X, TargetLocation.Y, HomeLocation.Z);
	const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, GodFallData->GroundTraceDownDistance);

	FHitResult HitResult;
	if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, GodFallData->GroundTraceChannel, QueryParams))
	{
		return false;
	}

	OutGroundLocation = HitResult.ImpactPoint;

	if (GodFallData->bDrawDebug)
	{
		DrawDebugLine(World, TraceStart, TraceEnd, FColor::Blue, false, 1.0f);
		DrawDebugSphere(World, OutGroundLocation, GodFallData->ImpactRadius, 16, FColor::Red, false, 1.0f);
	}

	return true;
}

bool APRFaerinGodFallStaticSwordActor::ResolveEntryOrbitImpactLocation(FVector& OutGroundLocation) const
{
	if (!IsValid(GodFallData))
	{
		return false;
	}

	FVector TargetLocation = GetActorLocation();
	if (GodFallData->bUseEntryOrbitImpactSpread)
	{
		const int32 SwordCount = 10;
		const float BaseAngleDegrees = SwordCount > 0
			? (360.0f / static_cast<float>(SwordCount)) * static_cast<float>(FMath::Max(SwordIndex, 0))
			: 0.0f;
		const float SpinAngleDegrees = ResolveEntryOrbitAngleDegrees(EntryOrbitDurationSeconds);
		const float LandingAngleRadians = FMath::DegreesToRadians(
			BaseAngleDegrees + SpinAngleDegrees + GodFallData->EntryOrbitImpactLandingAngleLeadDegrees);
		const float LandingRadius = FMath::Max(GodFallData->EntryOrbitImpactLandingRadius, 0.0f);
		const FVector LandingOffset(
			FMath::Cos(LandingAngleRadians) * LandingRadius,
			FMath::Sin(LandingAngleRadians) * LandingRadius,
			0.0f);

		TargetLocation = EntryOrbitCenterLocation + LandingOffset;
	}

	if (!ProjectTargetLocationToGround(TargetLocation, OutGroundLocation))
	{
		return false;
	}

	OutGroundLocation.Z += GodFallData->EntryOrbitImpactGroundOffsetZ;
	return true;
}

FVector APRFaerinGodFallStaticSwordActor::ResolveEntryOrbitImpactSpreadLocation(const float Alpha) const
{
	if (!IsValid(GodFallData) || !bEntryOrbitImpactSpreadActive)
	{
		return FMath::Lerp(SegmentStartLocation, SegmentTargetLocation, Alpha);
	}

	const float SpreadStartAlpha = FMath::Clamp(GodFallData->EntryOrbitImpactSpreadStartAlpha, 0.0f, 1.0f);
	const float RawSpreadAlpha = SpreadStartAlpha >= 1.0f - KINDA_SMALL_NUMBER
		? (Alpha >= 1.0f ? 1.0f : 0.0f)
		: FMath::Clamp((Alpha - SpreadStartAlpha) / (1.0f - SpreadStartAlpha), 0.0f, 1.0f);
	const float SpreadAlpha = FMath::Pow(RawSpreadAlpha, FMath::Max(GodFallData->EntryOrbitImpactSpreadExponent, 0.1f));

	FVector Result = FMath::Lerp(SegmentStartLocation, SegmentTargetLocation, Alpha);
	Result.X = FMath::Lerp(SegmentStartLocation.X, SegmentTargetLocation.X, SpreadAlpha);
	Result.Y = FMath::Lerp(SegmentStartLocation.Y, SegmentTargetLocation.Y, SpreadAlpha);
	return Result;
}

FVector APRFaerinGodFallStaticSwordActor::ResolveEntryOrbitGatherLocation(const float ElapsedSeconds) const
{
	if (!IsValid(GodFallData))
	{
		return GetActorLocation();
	}

	const float Duration = FMath::Max(EntryOrbitGatherDurationSeconds, UE_SMALL_NUMBER);
	const float RawAlpha = FMath::Clamp(ElapsedSeconds / Duration, 0.0f, 1.0f);
	const float EaseExponent = FMath::Max(GodFallData->EntryOrbitGatherAccelerationExponent, 0.1f);
	const float Alpha = FMath::Pow(RawAlpha, EaseExponent);
	return FMath::Lerp(EntryOrbitGatherStartLocation, ResolveEntryOrbitLocation(0.0f), Alpha);
}

FVector APRFaerinGodFallStaticSwordActor::ResolveEntryOrbitLocation(const float ElapsedSeconds) const
{
	if (!IsValid(GodFallData))
	{
		return GetActorLocation();
	}

	const int32 SwordCount = 10;
	const float BaseAngleDegrees = SwordCount > 0
		? (360.0f / static_cast<float>(SwordCount)) * static_cast<float>(FMath::Max(SwordIndex, 0))
		: 0.0f;
	const float CurrentAngleRadians = FMath::DegreesToRadians(BaseAngleDegrees + ResolveEntryOrbitAngleDegrees(ElapsedSeconds));
	const float Radius = ResolveEntryOrbitRadius(ElapsedSeconds);

	const FVector CircleOffset(
		FMath::Cos(CurrentAngleRadians) * Radius,
		FMath::Sin(CurrentAngleRadians) * Radius,
		0.0f);
	return EntryOrbitCenterLocation + CircleOffset;
}

float APRFaerinGodFallStaticSwordActor::ResolveEntryOrbitRadius(const float ElapsedSeconds) const
{
	if (!IsValid(GodFallData))
	{
		return 0.0f;
	}

	const float StartRadius = FMath::Max(GodFallData->EntryOrbitRadius, 0.0f);
	const float MinRadius = FMath::Clamp(GodFallData->EntryOrbitImpactMinRadius, 0.0f, StartRadius);
	const float Duration = FMath::Max(EntryOrbitDurationSeconds, UE_SMALL_NUMBER);
	const float NormalizedTime = FMath::Clamp(ElapsedSeconds / Duration, 0.0f, 1.0f);
	const float ShrinkAlpha = ResolveEntryOrbitRangedAlpha(
		NormalizedTime,
		GodFallData->EntryOrbitRadiusShrinkStartAlpha,
		GodFallData->EntryOrbitRadiusShrinkEndAlpha,
		GodFallData->EntryOrbitRadiusShrinkExponent);
	return FMath::Lerp(StartRadius, MinRadius, ShrinkAlpha);
}

float APRFaerinGodFallStaticSwordActor::ResolveEntryOrbitAngleDegrees(const float ElapsedSeconds) const
{
	if (!IsValid(GodFallData))
	{
		return 0.0f;
	}

	const float Duration = FMath::Max(EntryOrbitDurationSeconds, UE_SMALL_NUMBER);
	const float ClampedTime = FMath::Clamp(ElapsedSeconds, 0.0f, Duration);
	const float NormalizedTime = FMath::Clamp(ClampedTime / Duration, 0.0f, 1.0f);
	const float Exponent = FMath::Max(GodFallData->EntryOrbitAccelerationExponent, 0.1f);

	if (GodFallData->EntryOrbitSpinControlMode == EPRFaerinGodFallEntryOrbitSpinControlMode::TotalTurns)
	{
		return FMath::Max(GodFallData->EntryOrbitTotalTurns, 0.0f)
			* 360.0f
			* FMath::Pow(NormalizedTime, Exponent);
	}

	const float StartSpeed = FMath::Max(GodFallData->EntryOrbitStartAngularSpeedDeg, 0.0f);
	const float MaxSpeed = FMath::Max(GodFallData->EntryOrbitMaxAngularSpeedDeg, StartSpeed);

	// Speed(t) = Start + (Max - Start) * pow(t / Duration, Exponent)을 적분한 각도다.
	return StartSpeed * ClampedTime
		+ (MaxSpeed - StartSpeed) * Duration * FMath::Pow(NormalizedTime, Exponent + 1.0f) / (Exponent + 1.0f);
}

float APRFaerinGodFallStaticSwordActor::ResolveEntryOrbitRangedAlpha(
	const float NormalizedTime,
	const float StartAlpha,
	const float EndAlpha,
	const float Exponent) const
{
	const float ClampedStart = FMath::Clamp(StartAlpha, 0.0f, 1.0f);
	const float ClampedEnd = FMath::Clamp(EndAlpha, 0.0f, 1.0f);
	if (ClampedEnd <= ClampedStart + KINDA_SMALL_NUMBER)
	{
		return NormalizedTime >= ClampedStart ? 1.0f : 0.0f;
	}

	const float RawAlpha = FMath::Clamp((NormalizedTime - ClampedStart) / (ClampedEnd - ClampedStart), 0.0f, 1.0f);
	return FMath::Pow(RawAlpha, FMath::Max(Exponent, 0.1f));
}

float APRFaerinGodFallStaticSwordActor::ResolveEntryOrbitSpinElapsedFromTotal(const float TotalElapsedSeconds) const
{
	return FMath::Clamp(
		TotalElapsedSeconds
		- EntryOrbitStartDelaySeconds
		- EntryOrbitGatherDurationSeconds
		- EntryOrbitPreSpinHoldSeconds,
		0.0f,
		FMath::Max(EntryOrbitDurationSeconds, 0.0f));
}

float APRFaerinGodFallStaticSwordActor::ResolveEntryOrbitTimelineDuration() const
{
	return FMath::Max(EntryOrbitStartDelaySeconds, 0.0f)
		+ FMath::Max(EntryOrbitGatherDurationSeconds, 0.0f)
		+ FMath::Max(EntryOrbitPreSpinHoldSeconds, 0.0f)
		+ FMath::Max(EntryOrbitDurationSeconds, 0.0f)
		+ FMath::Max(EntryOrbitPostSpinHoldSeconds, 0.0f);
}

FRotator APRFaerinGodFallStaticSwordActor::ResolveEntryOrbitTiltRotation(const float ElapsedSeconds) const
{
	if (!IsValid(GodFallData))
	{
		return FRotator::ZeroRotator;
	}

	const float Duration = FMath::Max(EntryOrbitDurationSeconds, UE_SMALL_NUMBER);
	const float NormalizedTime = FMath::Clamp(ElapsedSeconds / Duration, 0.0f, 1.0f);
	const float TiltAlpha = ResolveEntryOrbitRangedAlpha(
		NormalizedTime,
		GodFallData->EntryOrbitTiltStartAlpha,
		GodFallData->EntryOrbitTiltEndAlpha,
		GodFallData->EntryOrbitTiltExponent);
	const float TiltDegrees = FMath::Clamp(GodFallData->EntryOrbitMaxOutwardTiltDegrees, 0.0f, 89.0f) * TiltAlpha;
	const FVector ToSword = (GetActorLocation() - EntryOrbitCenterLocation).GetSafeNormal2D();
	const float OutwardYaw = !ToSword.IsNearlyZero()
		? ToSword.Rotation().Yaw
		: 0.0f;

	// Mesh baseline에 상대적으로 적용할 visual tilt다. Actor 위치 회전과 별개로 검이 바깥으로 벌어지는 느낌만 담당한다.
	return FRotator(-TiltDegrees, OutwardYaw, 0.0f);
}

EPRFaerinGodFallStaticSwordState APRFaerinGodFallStaticSwordActor::ResolveCurrentVisualState() const
{
	if (bClientSwordEntryOrbitActive || bClientSwordEntryStraighteningActive || bClientSwordPresentationActive)
	{
		return ClientPresentationState;
	}

	return SwordState;
}

bool APRFaerinGodFallStaticSwordActor::IsEntryOrbitCenterFacingVisualState(
	const EPRFaerinGodFallStaticSwordState VisualState) const
{
	if (!IsValid(GodFallData) || !GodFallData->bUseEntryOrbitCenterFacingConeRotation)
	{
		return false;
	}

	switch (VisualState)
	{
	case EPRFaerinGodFallStaticSwordState::EntryOrbitStartDelay:
	case EPRFaerinGodFallStaticSwordState::EntryOrbitGathering:
	case EPRFaerinGodFallStaticSwordState::EntryOrbitPreSpinHold:
	case EPRFaerinGodFallStaticSwordState::EntryOrbiting:
	case EPRFaerinGodFallStaticSwordState::EntryOrbitPostSpinHold:
	case EPRFaerinGodFallStaticSwordState::EntryStraightening:
		return true;
	case EPRFaerinGodFallStaticSwordState::EntryDiving:
	case EPRFaerinGodFallStaticSwordState::EntryImpact:
		return GodFallData->bKeepEntryOrbitCenterFacingDuringImpactDrop;
	default:
		return false;
	}
}

FVector APRFaerinGodFallStaticSwordActor::ResolveCurrentEntryOrbitPresentationLocationForRotation() const
{
	const EPRFaerinGodFallStaticSwordState VisualState = ResolveCurrentVisualState();
	switch (VisualState)
	{
	case EPRFaerinGodFallStaticSwordState::EntryOrbitStartDelay:
		return EntryOrbitGatherStartLocation;
	case EPRFaerinGodFallStaticSwordState::EntryOrbitGathering:
	{
		const float TotalElapsedSeconds = bClientSwordEntryOrbitActive
			? ClientEntryOrbitElapsedSeconds
			: EntryOrbitElapsedSeconds;
		const float GatherElapsedSeconds = FMath::Max(TotalElapsedSeconds - EntryOrbitStartDelaySeconds, 0.0f);
		return ResolveEntryOrbitGatherLocation(GatherElapsedSeconds);
	}
	case EPRFaerinGodFallStaticSwordState::EntryOrbitPreSpinHold:
		return ResolveEntryOrbitLocation(0.0f);
	case EPRFaerinGodFallStaticSwordState::EntryOrbiting:
	{
		const float TotalElapsedSeconds = bClientSwordEntryOrbitActive
			? ClientEntryOrbitElapsedSeconds
			: EntryOrbitElapsedSeconds;
		return ResolveEntryOrbitLocation(ResolveEntryOrbitSpinElapsedFromTotal(TotalElapsedSeconds));
	}
	case EPRFaerinGodFallStaticSwordState::EntryOrbitPostSpinHold:
		return ResolveEntryOrbitLocation(EntryOrbitDurationSeconds);
	case EPRFaerinGodFallStaticSwordState::EntryDiving:
	case EPRFaerinGodFallStaticSwordState::EntryImpact:
	case EPRFaerinGodFallStaticSwordState::EntryStraightening:
		return GetActorLocation();
	default:
		return GetActorLocation();
	}
}

FQuat APRFaerinGodFallStaticSwordActor::ResolveEntryOrbitCenterFacingConeWorldQuat(
	const FVector& PresentationLocation,
	const float EffectiveSpinElapsedSeconds) const
{
	if (!IsValid(GodFallData))
	{
		return GetActorQuat();
	}

	FVector RadialOut = PresentationLocation - EntryOrbitCenterLocation;
	RadialOut.Z = 0.0f;
	if (!RadialOut.Normalize())
	{
		const int32 SwordCount = 10;
		const float BaseAngleDegrees = SwordCount > 0
			? (360.0f / static_cast<float>(SwordCount)) * static_cast<float>(FMath::Max(SwordIndex, 0))
			: 0.0f;
		const float AngleRadians = FMath::DegreesToRadians(BaseAngleDegrees + ResolveEntryOrbitAngleDegrees(EffectiveSpinElapsedSeconds));
		RadialOut = FVector(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians), 0.0f);
	}

	const float Duration = FMath::Max(EntryOrbitDurationSeconds, UE_SMALL_NUMBER);
	const float NormalizedTime = FMath::Clamp(EffectiveSpinElapsedSeconds / Duration, 0.0f, 1.0f);
	const float TiltAlpha = ResolveEntryOrbitRangedAlpha(
		NormalizedTime,
		GodFallData->EntryOrbitTiltStartAlpha,
		GodFallData->EntryOrbitTiltEndAlpha,
		GodFallData->EntryOrbitTiltExponent);
	const float TiltDegrees = FMath::Clamp(GodFallData->EntryOrbitMaxOutwardTiltDegrees, 0.0f, 89.0f) * TiltAlpha;
	const float TiltRadians = FMath::DegreesToRadians(TiltDegrees);
	const float ConeTiltDirectionSign = GodFallData->bInvertEntryOrbitCenterFacingConeTiltDirection ? -1.0f : 1.0f;
	const FVector DesiredBladeWorld = (FVector::UpVector * FMath::Cos(TiltRadians)
		+ RadialOut * ConeTiltDirectionSign * FMath::Sin(TiltRadians)).GetSafeNormal();
	const FVector DesiredFaceWorld = ProjectDirectionOnPlaneSafe(-RadialOut, DesiredBladeWorld, -RadialOut);

	const FVector LocalBladeAxis = ResolveGodFallLocalAxisVector(GodFallData->EntryOrbitConeLocalBladeAxis);
	const FVector LocalFaceAxis = ResolveGodFallLocalAxisVector(GodFallData->EntryOrbitCenterFacingLocalFaceAxis);
	FQuat WorldQuat = MakeQuatMappingLocalAxesToWorldAxes(
		LocalBladeAxis,
		LocalFaceAxis,
		DesiredBladeWorld,
		DesiredFaceWorld);
	WorldQuat = WorldQuat * GodFallData->EntryOrbitCenterFacingRotationOffset.Quaternion();
	return WorldQuat.GetNormalized();
}

FQuat APRFaerinGodFallStaticSwordActor::ResolveEntryOrbitCenterFacingConeRelativeQuat() const
{
	if (!IsValid(GodFallData)
		|| !GodFallData->bUseEntryOrbitCenterFacingConeRotation
		|| !IsValid(StaticSwordMeshComponent)
		|| !IsEntryOrbitCenterFacingVisualState(ResolveCurrentVisualState()))
	{
		return MeshBaselineRelativeTransform.GetRotation();
	}

	const EPRFaerinGodFallStaticSwordState VisualState = ResolveCurrentVisualState();
	const float TotalEntryOrbitElapsedSeconds = bClientSwordEntryOrbitActive
		? ClientEntryOrbitElapsedSeconds
		: EntryOrbitElapsedSeconds;
	const float EffectiveSpinElapsedSeconds =
		(VisualState == EPRFaerinGodFallStaticSwordState::EntryDiving
			|| VisualState == EPRFaerinGodFallStaticSwordState::EntryImpact
			|| VisualState == EPRFaerinGodFallStaticSwordState::EntryStraightening)
			? EntryOrbitDurationSeconds
			: ResolveEntryOrbitSpinElapsedFromTotal(TotalEntryOrbitElapsedSeconds);
	const FVector PresentationLocation = ResolveCurrentEntryOrbitPresentationLocationForRotation();
	const FQuat DesiredWorldQuat = ResolveEntryOrbitCenterFacingConeWorldQuat(PresentationLocation, EffectiveSpinElapsedSeconds);
	const USceneComponent* AttachParentComponent = StaticSwordMeshComponent->GetAttachParent();
	const FQuat ParentWorldQuat = IsValid(AttachParentComponent)
		? AttachParentComponent->GetComponentQuat()
		: FQuat::Identity;
	const FQuat DesiredRelativeQuat = (ParentWorldQuat.Inverse() * DesiredWorldQuat).GetNormalized();

	if (VisualState == EPRFaerinGodFallStaticSwordState::EntryStraightening)
	{
		const float StraightenElapsed = bClientSwordEntryStraighteningActive
			? ClientEntryStraightenElapsedSeconds
			: EntryStraightenElapsedSeconds;
		const float StraightenDuration = bClientSwordEntryStraighteningActive
			? ClientEntryStraightenDurationSeconds
			: EntryStraightenDurationSeconds;
		const float EaseExponent = bClientSwordEntryStraighteningActive
			? ClientEntryStraightenEaseExponent
			: EntryStraightenEaseExponent;
		const float RawAlpha = FMath::Clamp(StraightenElapsed / FMath::Max(StraightenDuration, UE_SMALL_NUMBER), 0.0f, 1.0f);
		const float Alpha = FMath::Pow(RawAlpha, FMath::Max(EaseExponent, 0.1f));
		return FQuat::Slerp(DesiredRelativeQuat, MeshBaselineRelativeTransform.GetRotation(), Alpha).GetNormalized();
	}

	return DesiredRelativeQuat;
}

// ===== 피해 =====

void APRFaerinGodFallStaticSwordActor::ApplySwordPresentationLocation(const FVector& Location)
{
	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
}

void APRFaerinGodFallStaticSwordActor::SpawnNiagaraAtLocationLocal(UNiagaraSystem* NiagaraSystem,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale,
	const float LifeSeconds) const
{
	if (!IsValid(NiagaraSystem))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		NiagaraSystem,
		Location,
		Rotation,
		Scale,
		true,
		true);
	if (!IsValid(NiagaraComponent) || LifeSeconds <= UE_SMALL_NUMBER)
	{
		return;
	}

	TWeakObjectPtr<UNiagaraComponent> WeakNiagaraComponent = NiagaraComponent;
	FTimerHandle CleanupTimerHandle;
	World->GetTimerManager().SetTimer(
		CleanupTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [WeakNiagaraComponent]()
		{
			if (UNiagaraComponent* ActiveNiagaraComponent = WeakNiagaraComponent.Get())
			{
				ActiveNiagaraComponent->Deactivate();
				ActiveNiagaraComponent->DestroyComponent();
			}
		}),
		LifeSeconds,
		false);
}

void APRFaerinGodFallStaticSwordActor::CaptureMeshBaselineTransform()
{
	if (!IsValid(StaticSwordMeshComponent))
	{
		return;
	}

	MeshBaselineRelativeTransform = StaticSwordMeshComponent->GetRelativeTransform();
	bMeshBaselineCaptured = true;
}

void APRFaerinGodFallStaticSwordActor::ResetSwordVisualTransform()
{
	if (!IsValid(StaticSwordMeshComponent))
	{
		return;
	}

	if (!bMeshBaselineCaptured)
	{
		CaptureMeshBaselineTransform();
	}

	StaticSwordMeshComponent->SetRelativeTransform(MeshBaselineRelativeTransform);
}

void APRFaerinGodFallStaticSwordActor::UpdateSwordVisualPresentation(const float DeltaSeconds)
{
	if (!IsValid(StaticSwordMeshComponent))
	{
		return;
	}

	if (!bMeshBaselineCaptured)
	{
		CaptureMeshBaselineTransform();
	}

	if (!ShouldTickSwordVisual())
	{
		ResetSwordVisualTransform();
		return;
	}

	VisualElapsedSeconds += DeltaSeconds;
	VisualStateElapsedSeconds += DeltaSeconds;

	// 적용 순서: Baseline -> HoverLocationDelta -> ChargeShakeLocation/RotationDelta -> EntryOrbitRotation/CenterFacing -> ImpactSlantRotationDelta -> FinalRelativeTransform.
	FVector FinalLocation = MeshBaselineRelativeTransform.GetLocation();
	FQuat FinalRotation = MeshBaselineRelativeTransform.GetRotation();
	const FVector FinalScale = MeshBaselineRelativeTransform.GetScale3D();

	FinalLocation += ResolveHoverLocationDelta();

	FVector ChargeShakeLocationDelta = FVector::ZeroVector;
	FRotator ChargeShakeRotationDelta = FRotator::ZeroRotator;
	ResolveChargeShakeDelta(ChargeShakeLocationDelta, ChargeShakeRotationDelta);
	FinalLocation += ChargeShakeLocationDelta;
	FinalRotation = FinalRotation * ChargeShakeRotationDelta.Quaternion();

	const EPRFaerinGodFallStaticSwordState VisualState = ResolveCurrentVisualState();
	if (IsEntryOrbitCenterFacingVisualState(VisualState))
	{
		FinalRotation = ResolveEntryOrbitCenterFacingConeRelativeQuat();
	}
	else
	{
		FinalRotation = FinalRotation * ResolveEntryOrbitRotationDelta().Quaternion();
	}
	FinalRotation = FinalRotation * ResolveImpactSlantRotationDelta().Quaternion();

	StaticSwordMeshComponent->SetRelativeTransform(FTransform(FinalRotation, FinalLocation, FinalScale));
}

bool APRFaerinGodFallStaticSwordActor::ShouldTickSwordVisual() const
{
	if (!IsValid(GodFallData) || !IsValid(StaticSwordMeshComponent))
	{
		return false;
	}

	const EPRFaerinGodFallStaticSwordState VisualState = ResolveCurrentVisualState();

	const bool bCanHover = GodFallData->bEnableStaticSwordHoverVisual
		&& (VisualState == EPRFaerinGodFallStaticSwordState::Charging
			|| VisualState == EPRFaerinGodFallStaticSwordState::Charged
			|| VisualState == EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead
			|| VisualState == EPRFaerinGodFallStaticSwordState::Telegraph);
	const bool bCanChargeShake = GodFallData->bEnableChargeShakeVisual
		&& VisualState == EPRFaerinGodFallStaticSwordState::Charging;
	const bool bCanSlant = GodFallData->bEnableImpactVisualSlant
		&& (VisualState == EPRFaerinGodFallStaticSwordState::Dropping
			|| VisualState == EPRFaerinGodFallStaticSwordState::Impact);
	const bool bCanEntryOrbitTilt = GodFallData->bUseEntryOrbitBeforeImpact
		&& (VisualState == EPRFaerinGodFallStaticSwordState::EntryOrbitStartDelay
			|| VisualState == EPRFaerinGodFallStaticSwordState::EntryOrbitGathering
			|| VisualState == EPRFaerinGodFallStaticSwordState::EntryOrbitPreSpinHold
			|| VisualState == EPRFaerinGodFallStaticSwordState::EntryOrbiting
			|| VisualState == EPRFaerinGodFallStaticSwordState::EntryOrbitPostSpinHold
			|| VisualState == EPRFaerinGodFallStaticSwordState::EntryDiving
			|| VisualState == EPRFaerinGodFallStaticSwordState::EntryImpact
			|| VisualState == EPRFaerinGodFallStaticSwordState::EntryStraightening);

	return bCanHover || bCanChargeShake || bCanSlant || bCanEntryOrbitTilt;
}

bool APRFaerinGodFallStaticSwordActor::ShouldTickSwordMovement() const
{
	if (!HasAuthority())
	{
		return bClientSwordPresentationActive || bClientSwordEntryOrbitActive || bClientSwordEntryStraighteningActive;
	}

	return SwordState == EPRFaerinGodFallStaticSwordState::EntryOrbitStartDelay
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryOrbitGathering
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryOrbitPreSpinHold
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryOrbiting
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryOrbitPostSpinHold
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryDiving
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryStraightening
		|| SwordState == EPRFaerinGodFallStaticSwordState::EntryDiveReturning
		|| SwordState == EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead
		|| SwordState == EPRFaerinGodFallStaticSwordState::Dropping
		|| SwordState == EPRFaerinGodFallStaticSwordState::Returning;
}

void APRFaerinGodFallStaticSwordActor::RefreshSwordTickEnabled()
{
	SetActorTickEnabled(ShouldTickSwordMovement() || ShouldTickSwordVisual());
}

FVector APRFaerinGodFallStaticSwordActor::ResolveHoverLocationDelta() const
{
	if (!IsValid(GodFallData) || !GodFallData->bEnableStaticSwordHoverVisual)
	{
		return FVector::ZeroVector;
	}

	const EPRFaerinGodFallStaticSwordState VisualState = ResolveCurrentVisualState();
	if (VisualState != EPRFaerinGodFallStaticSwordState::Charging
		&& VisualState != EPRFaerinGodFallStaticSwordState::Charged
		&& VisualState != EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead
		&& VisualState != EPRFaerinGodFallStaticSwordState::Telegraph)
	{
		return FVector::ZeroVector;
	}

	const float Phase = VisualElapsedSeconds * 2.0f * PI * GodFallData->HoverFrequencyHz
		+ static_cast<float>(SwordIndex) * GodFallData->HoverPhaseOffsetPerSword;
	return FVector(
		FMath::Cos(Phase) * GodFallData->HoverLateralAmplitude,
		FMath::Sin(Phase * 0.67f) * GodFallData->HoverLateralAmplitude,
		FMath::Sin(Phase) * GodFallData->HoverAmplitudeZ);
}

void APRFaerinGodFallStaticSwordActor::ResolveChargeShakeDelta(FVector& OutLocationDelta, FRotator& OutRotationDelta) const
{
	OutLocationDelta = FVector::ZeroVector;
	OutRotationDelta = FRotator::ZeroRotator;

	const EPRFaerinGodFallStaticSwordState VisualState = ResolveCurrentVisualState();
	if (!IsValid(GodFallData)
		|| !GodFallData->bEnableChargeShakeVisual
		|| VisualState != EPRFaerinGodFallStaticSwordState::Charging)
	{
		return;
	}

	const float RampAlpha = GodFallData->ChargeShakeRampInSeconds > UE_SMALL_NUMBER
		? FMath::Clamp(VisualStateElapsedSeconds / GodFallData->ChargeShakeRampInSeconds, 0.0f, 1.0f)
		: 1.0f;
	const float Phase = VisualElapsedSeconds * 2.0f * PI * GodFallData->ChargeShakeFrequencyHz
		+ static_cast<float>(SwordIndex) * GodFallData->ChargeShakePhaseOffsetPerSword;

	OutLocationDelta = FVector(
		FMath::Sin(Phase) * GodFallData->ChargeShakeAmplitudeLocation.X,
		FMath::Sin(Phase * 1.31f) * GodFallData->ChargeShakeAmplitudeLocation.Y,
		FMath::Cos(Phase * 0.73f) * GodFallData->ChargeShakeAmplitudeLocation.Z) * RampAlpha;
	OutRotationDelta = FRotator(
		FMath::Sin(Phase * 0.89f) * GodFallData->ChargeShakeAmplitudeRotation.Pitch,
		FMath::Sin(Phase * 1.17f) * GodFallData->ChargeShakeAmplitudeRotation.Yaw,
		FMath::Cos(Phase * 1.07f) * GodFallData->ChargeShakeAmplitudeRotation.Roll) * RampAlpha;
}

FRotator APRFaerinGodFallStaticSwordActor::ResolveEntryOrbitRotationDelta() const
{
	const EPRFaerinGodFallStaticSwordState VisualState = ResolveCurrentVisualState();
	if (!IsValid(GodFallData)
		|| !GodFallData->bUseEntryOrbitBeforeImpact
		|| (VisualState != EPRFaerinGodFallStaticSwordState::EntryOrbitStartDelay
			&& VisualState != EPRFaerinGodFallStaticSwordState::EntryOrbitGathering
			&& VisualState != EPRFaerinGodFallStaticSwordState::EntryOrbitPreSpinHold
			&& VisualState != EPRFaerinGodFallStaticSwordState::EntryOrbiting
			&& VisualState != EPRFaerinGodFallStaticSwordState::EntryOrbitPostSpinHold
			&& VisualState != EPRFaerinGodFallStaticSwordState::EntryDiving
			&& VisualState != EPRFaerinGodFallStaticSwordState::EntryImpact
			&& VisualState != EPRFaerinGodFallStaticSwordState::EntryStraightening))
	{
		return FRotator::ZeroRotator;
	}

	const float TotalEntryOrbitElapsedSeconds = bClientSwordEntryOrbitActive
		? ClientEntryOrbitElapsedSeconds
		: EntryOrbitElapsedSeconds;
	const float EffectiveElapsedSeconds = (VisualState == EPRFaerinGodFallStaticSwordState::EntryDiving
		|| VisualState == EPRFaerinGodFallStaticSwordState::EntryImpact
		|| VisualState == EPRFaerinGodFallStaticSwordState::EntryStraightening)
		? EntryOrbitDurationSeconds
		: ResolveEntryOrbitSpinElapsedFromTotal(TotalEntryOrbitElapsedSeconds);
	FRotator OrbitTiltRotation = ResolveEntryOrbitTiltRotation(EffectiveElapsedSeconds);
	if (VisualState == EPRFaerinGodFallStaticSwordState::EntryStraightening)
	{
		const float StraightenElapsed = bClientSwordEntryStraighteningActive
			? ClientEntryStraightenElapsedSeconds
			: EntryStraightenElapsedSeconds;
		const float StraightenDuration = bClientSwordEntryStraighteningActive
			? ClientEntryStraightenDurationSeconds
			: EntryStraightenDurationSeconds;
		const float RawAlpha = FMath::Clamp(StraightenElapsed / FMath::Max(StraightenDuration, UE_SMALL_NUMBER), 0.0f, 1.0f);
		const float EaseExponent = bClientSwordEntryStraighteningActive
			? ClientEntryStraightenEaseExponent
			: EntryStraightenEaseExponent;
		const float RemainingAlpha = 1.0f - FMath::Pow(RawAlpha, FMath::Max(EaseExponent, 0.1f));
		OrbitTiltRotation.Pitch *= RemainingAlpha;
		OrbitTiltRotation.Yaw *= RemainingAlpha;
		OrbitTiltRotation.Roll *= RemainingAlpha;
	}
	return OrbitTiltRotation;
}

FRotator APRFaerinGodFallStaticSwordActor::ResolveImpactSlantRotationDelta() const
{
	if (!IsValid(GodFallData) || !GodFallData->bEnableImpactVisualSlant)
	{
		return FRotator::ZeroRotator;
	}

	const float Alpha = ResolveImpactSlantAlpha();
	if (Alpha <= UE_SMALL_NUMBER)
	{
		return FRotator::ZeroRotator;
	}

	const uint32 Seed = GetTypeHash(SourceBoneName) ^ static_cast<uint32>((SwordIndex + 17) * 196613);
	FRandomStream RandomStream(static_cast<int32>(Seed));
	const float MinYaw = FMath::Min(GodFallData->ImpactSlantRandomYawMinDegrees, GodFallData->ImpactSlantRandomYawMaxDegrees);
	const float MaxYaw = FMath::Max(GodFallData->ImpactSlantRandomYawMinDegrees, GodFallData->ImpactSlantRandomYawMaxDegrees);
	const float RandomYaw = RandomStream.FRandRange(MinYaw, MaxYaw);

	return FRotator(
		GodFallData->ImpactSlantPitchDegrees * Alpha,
		RandomYaw * Alpha,
		GodFallData->ImpactSlantRollDegrees * Alpha);
}

float APRFaerinGodFallStaticSwordActor::ResolveImpactSlantAlpha() const
{
	const EPRFaerinGodFallStaticSwordState VisualState = ResolveCurrentVisualState();

	if (VisualState == EPRFaerinGodFallStaticSwordState::Dropping)
	{
		const float ElapsedSeconds = bClientSwordPresentationActive ? ClientSegmentElapsedSeconds : SegmentElapsedSeconds;
		const float DurationSeconds = bClientSwordPresentationActive ? ClientSegmentDurationSeconds : SegmentDurationSeconds;
		const float DropAlpha = FMath::Clamp(ElapsedSeconds / FMath::Max(DurationSeconds, UE_SMALL_NUMBER), 0.0f, 1.0f);
		const float StartAlpha = FMath::Clamp(IsValid(GodFallData) ? GodFallData->ImpactSlantBlendInStartAlpha : 0.0f, 0.0f, 1.0f);
		return DropAlpha <= StartAlpha
			? 0.0f
			: FMath::Clamp((DropAlpha - StartAlpha) / FMath::Max(1.0f - StartAlpha, UE_SMALL_NUMBER), 0.0f, 1.0f);
	}

	if (VisualState == EPRFaerinGodFallStaticSwordState::Impact)
	{
		return 1.0f;
	}

	return 0.0f;
}

void APRFaerinGodFallStaticSwordActor::ScheduleImpactWarning()
{
	if (!HasAuthority()
		|| bImpactWarningSpawned
		|| !bHasAssignedAttackLocation
		|| !IsValid(GodFallData))
	{
		return;
	}

	// 강타 사운드는 warning Niagara 유무와 독립적으로 예약한다.
	ScheduleStrikeSound();

	if (!IsValid(GodFallData->ImpactWarningNiagaraSystem))
	{
		return;
	}

	const float TimeUntilImpact = FMath::Max(AssignedWarningSeconds, 0.0f) + FMath::Max(GodFallData->DropSeconds, 0.0f);
	const float WarningDelaySeconds = FMath::Max(TimeUntilImpact - FMath::Max(GodFallData->ImpactWarningLeadSeconds, 0.0f), 0.0f);
	const float SpawnServerWorldTimeSeconds = ResolveServerWorldTimeSeconds() + WarningDelaySeconds;
	const FVector WarningLocation = AssignedImpactLocation + GodFallData->ImpactWarningLocationOffset;
	const FSoftObjectPath WarningSystemPath(GodFallData->ImpactWarningNiagaraSystem.Get());

	MulticastScheduleGodFallSwordImpactWarning(
		WarningLocation,
		GodFallData->ImpactWarningNiagaraRotationOffset,
		WarningSystemPath,
		GodFallData->ImpactWarningNiagaraScale,
		GodFallData->ImpactWarningNiagaraLifeSeconds,
		SpawnServerWorldTimeSeconds);
	ForceNetUpdate();
}

void APRFaerinGodFallStaticSwordActor::ScheduleStrikeSound()
{
	if (!HasAuthority()
		|| !bHasAssignedAttackLocation
		|| !IsValid(GodFallData)
		|| !IsValid(GodFallData->SwordStrikeSoundCue))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetTimerManager().IsTimerActive(StrikeSoundTimerHandle))
	{
		return;
	}

	// 충돌까지 남은 시간 = (머리 위 경고 시간) + (낙하 시간). 강타 사운드는 그보다 LeadSeconds 앞서 재생한다.
	const float TimeUntilImpact = FMath::Max(AssignedWarningSeconds, 0.0f) + FMath::Max(GodFallData->DropSeconds, 0.0f);
	const float SoundDelay = FMath::Max(TimeUntilImpact - FMath::Max(GodFallData->SwordStrikeSoundLeadSeconds, 0.0f), 0.0f);
	const FVector StrikeLocation = AssignedImpactLocation;
	USoundBase* StrikeSound = GodFallData->SwordStrikeSoundCue;

	if (SoundDelay <= UE_SMALL_NUMBER)
	{
		MulticastPlayGodFallSwordStrikeSound(StrikeLocation, StrikeSound);
		return;
	}

	World->GetTimerManager().SetTimer(
		StrikeSoundTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, StrikeLocation, StrikeSound]()
		{
			MulticastPlayGodFallSwordStrikeSound(StrikeLocation, StrikeSound);
		}),
		SoundDelay,
		false);
}

void APRFaerinGodFallStaticSwordActor::MulticastPlayGodFallSwordStrikeSound_Implementation(FVector_NetQuantize StrikeLocation, USoundBase* StrikeSound)
{
	// 모든 클라이언트에서 충돌 위치에 강타 사운드를 재생한다. (데디케이티드 서버에선 무시)
	if (IsValid(StrikeSound))
	{
		UGameplayStatics::SpawnSoundAtLocation(this, StrikeSound, StrikeLocation);
	}
}

void APRFaerinGodFallStaticSwordActor::ScheduleImpactWarningLocal(
	const FVector& ImpactLocation,
	const float DelaySeconds,
	const FRotator& ImpactRotation,
	const FSoftObjectPath& NiagaraSystemPath,
	const FVector& Scale,
	const float LifeSeconds)
{
	if (bImpactWarningSpawned || !NiagaraSystemPath.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().ClearTimer(ImpactWarningTimerHandle);
		World->GetTimerManager().ClearTimer(EntryReturnChargeDelayTimerHandle);
	if (DelaySeconds <= UE_SMALL_NUMBER)
	{
		SpawnImpactWarningLocal(ImpactLocation, ImpactRotation, NiagaraSystemPath, Scale, LifeSeconds);
		return;
	}

	World->GetTimerManager().SetTimer(
		ImpactWarningTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, ImpactLocation, ImpactRotation, NiagaraSystemPath, Scale, LifeSeconds]()
		{
			SpawnImpactWarningLocal(ImpactLocation, ImpactRotation, NiagaraSystemPath, Scale, LifeSeconds);
		}),
		DelaySeconds,
		false);
}

void APRFaerinGodFallStaticSwordActor::SpawnImpactWarningLocal(
	const FVector& ImpactLocation,
	const FRotator& ImpactRotation,
	const FSoftObjectPath& NiagaraSystemPath,
	const FVector& Scale,
	const float LifeSeconds)
{
	if (bImpactWarningSpawned || !NiagaraSystemPath.IsValid())
	{
		return;
	}

	UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(NiagaraSystemPath.ResolveObject());
	if (!IsValid(NiagaraSystem))
	{
		NiagaraSystem = Cast<UNiagaraSystem>(NiagaraSystemPath.TryLoad());
	}

	if (!IsValid(NiagaraSystem))
	{
		UE_LOG(LogPRFaerinGodFallSword, Warning,
			TEXT("God Fall warning VFX skipped because Niagara failed to resolve. Sword=%s, Path=%s"),
			*GetNameSafe(this),
			*NiagaraSystemPath.ToString());
		return;
	}

	bImpactWarningSpawned = true;
	SpawnNiagaraAtLocationLocal(NiagaraSystem, ImpactLocation, ImpactRotation, Scale, LifeSeconds);
}

float APRFaerinGodFallStaticSwordActor::ResolveServerWorldTimeSeconds() const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return 0.0f;
	}

	const AGameStateBase* GameState = World->GetGameState();
	return IsValid(GameState) ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}

void APRFaerinGodFallStaticSwordActor::UpdateClientSwordPresentation(const float DeltaSeconds)
{
	if (bClientSwordEntryOrbitActive)
	{
		UpdateClientEntryOrbitMovement(DeltaSeconds);
		return;
	}

	if (bClientSwordEntryStraighteningActive)
	{
		UpdateClientEntryStraightening(DeltaSeconds);
		return;
	}

	if (!bClientSwordPresentationActive)
	{
		RefreshSwordTickEnabled();
		return;
	}

	if (ClientPresentationState == EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead)
	{
		UpdateClientTargetOverheadMovement(DeltaSeconds);
		return;
	}

	UpdateClientSegmentMovement(DeltaSeconds);
}

void APRFaerinGodFallStaticSwordActor::UpdateClientEntryOrbitMovement(const float DeltaSeconds)
{
	ClientEntryOrbitElapsedSeconds += DeltaSeconds;
	EntryOrbitCenterLocation = ClientEntryOrbitCenterLocation;
	EntryOrbitGatherStartLocation = ClientEntryOrbitGatherStartLocation;
	EntryOrbitStartDelaySeconds = ClientEntryOrbitStartDelaySeconds;
	EntryOrbitGatherDurationSeconds = ClientEntryOrbitGatherDurationSeconds;
	EntryOrbitPreSpinHoldSeconds = ClientEntryOrbitPreSpinHoldSeconds;
	EntryOrbitDurationSeconds = ClientEntryOrbitDurationSeconds;
	EntryOrbitPostSpinHoldSeconds = ClientEntryOrbitPostSpinHoldSeconds;

	const float TotalElapsedSeconds = ClientEntryOrbitElapsedSeconds;
	if (TotalElapsedSeconds < ClientEntryOrbitStartDelaySeconds)
	{
		EntryOrbitElapsedSeconds = TotalElapsedSeconds;
		ClientPresentationState = EPRFaerinGodFallStaticSwordState::EntryOrbitStartDelay;
		SetActorLocation(ClientEntryOrbitGatherStartLocation, false, nullptr, ETeleportType::TeleportPhysics);
		return;
	}

	const float GatherElapsedSeconds = TotalElapsedSeconds - ClientEntryOrbitStartDelaySeconds;
	if (GatherElapsedSeconds < ClientEntryOrbitGatherDurationSeconds)
	{
		EntryOrbitElapsedSeconds = TotalElapsedSeconds;
		ClientPresentationState = EPRFaerinGodFallStaticSwordState::EntryOrbitGathering;
		SetActorLocation(ResolveEntryOrbitGatherLocation(GatherElapsedSeconds), false, nullptr, ETeleportType::TeleportPhysics);
		return;
	}

	const float PreSpinElapsedSeconds = GatherElapsedSeconds - ClientEntryOrbitGatherDurationSeconds;
	if (PreSpinElapsedSeconds < ClientEntryOrbitPreSpinHoldSeconds)
	{
		EntryOrbitElapsedSeconds = TotalElapsedSeconds;
		ClientPresentationState = EPRFaerinGodFallStaticSwordState::EntryOrbitPreSpinHold;
		SetActorLocation(ResolveEntryOrbitLocation(0.0f), false, nullptr, ETeleportType::TeleportPhysics);
		return;
	}

	const float SpinElapsedSeconds = PreSpinElapsedSeconds - ClientEntryOrbitPreSpinHoldSeconds;
	if (SpinElapsedSeconds < ClientEntryOrbitDurationSeconds)
	{
		EntryOrbitElapsedSeconds = TotalElapsedSeconds;
		ClientPresentationState = EPRFaerinGodFallStaticSwordState::EntryOrbiting;
		SetActorLocation(ResolveEntryOrbitLocation(SpinElapsedSeconds), false, nullptr, ETeleportType::TeleportPhysics);
		return;
	}

	const float PostSpinElapsedSeconds = SpinElapsedSeconds - ClientEntryOrbitDurationSeconds;
	if (PostSpinElapsedSeconds < ClientEntryOrbitPostSpinHoldSeconds)
	{
		EntryOrbitElapsedSeconds = TotalElapsedSeconds;
		ClientPresentationState = EPRFaerinGodFallStaticSwordState::EntryOrbitPostSpinHold;
		SetActorLocation(ResolveEntryOrbitLocation(ClientEntryOrbitDurationSeconds), false, nullptr, ETeleportType::TeleportPhysics);
		return;
	}

	EntryOrbitElapsedSeconds = TotalElapsedSeconds;
	ClientPresentationState = EPRFaerinGodFallStaticSwordState::EntryOrbitPostSpinHold;
	SetActorLocation(ResolveEntryOrbitLocation(ClientEntryOrbitDurationSeconds), false, nullptr, ETeleportType::TeleportPhysics);
}

void APRFaerinGodFallStaticSwordActor::UpdateClientEntryStraightening(const float DeltaSeconds)
{
	ClientEntryStraightenElapsedSeconds += DeltaSeconds;
	const float RawAlpha = FMath::Clamp(
		ClientEntryStraightenElapsedSeconds / FMath::Max(ClientEntryStraightenDurationSeconds, UE_SMALL_NUMBER),
		0.0f,
		1.0f);
	const float Alpha = FMath::Pow(RawAlpha, FMath::Max(ClientEntryStraightenEaseExponent, 0.1f));
	SetActorRotation(FMath::Lerp(ClientEntryStraightenStartRotation, ClientEntryStraightenTargetRotation, Alpha));

	if (RawAlpha >= 1.0f)
	{
		SetActorRotation(ClientEntryStraightenTargetRotation);
		bClientSwordEntryStraighteningActive = false;
		ClientPresentationState = EPRFaerinGodFallStaticSwordState::None;
	}

	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::UpdateClientSegmentMovement(const float DeltaSeconds)
{
	ClientSegmentElapsedSeconds += DeltaSeconds;
	const float Alpha = FMath::Clamp(
		ClientSegmentElapsedSeconds / FMath::Max(ClientSegmentDurationSeconds, UE_SMALL_NUMBER),
		0.0f,
		1.0f);

	SetActorLocation(FMath::Lerp(ClientSegmentStartLocation, ClientSegmentTargetLocation, Alpha), false, nullptr, ETeleportType::TeleportPhysics);

	if (Alpha >= 1.0f)
	{
		bClientSwordPresentationActive = false;
		ClientPresentationState = EPRFaerinGodFallStaticSwordState::None;
		RefreshSwordTickEnabled();
	}
}

void APRFaerinGodFallStaticSwordActor::UpdateClientTargetOverheadMovement(const float DeltaSeconds)
{
	ClientOverheadMoveElapsedSeconds += DeltaSeconds;
	ClientOverheadMoveSpeed += ClientOverheadMoveAcceleration * DeltaSeconds;

	if (AActor* AssignedTarget = ClientAssignedTarget.Get())
	{
		const FVector TargetLocation = AssignedTarget->GetActorLocation();
		ClientAssignedOverheadLocation = FVector(TargetLocation.X, TargetLocation.Y, HomeLocation.Z);
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector ToTarget = ClientAssignedOverheadLocation - CurrentLocation;
	const float Distance = ToTarget.Size();
	if (Distance > UE_SMALL_NUMBER && ClientOverheadMoveSpeed > 0.0f)
	{
		const float MoveDistance = FMath::Min(ClientOverheadMoveSpeed * DeltaSeconds, Distance);
		SetActorLocation(CurrentLocation + ToTarget.GetSafeNormal() * MoveDistance, false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (ClientOverheadMoveElapsedSeconds >= ClientOverheadMoveDurationSeconds)
	{
		SetClientSwordPresentationLocation(ClientAssignedOverheadLocation);
	}
}

void APRFaerinGodFallStaticSwordActor::StartClientSwordEntryOrbit(
	const FVector& OrbitCenterLocation,
	const FVector& GatherStartLocation,
	const float StartDelaySeconds,
	const float GatherDurationSeconds,
	const float PreSpinHoldSeconds,
	const float OrbitDurationSeconds,
	const float PostSpinHoldSeconds,
	const float SpawnServerWorldTimeSeconds)
{
	ClientEntryOrbitCenterLocation = OrbitCenterLocation;
	EntryOrbitCenterLocation = OrbitCenterLocation;
	ClientEntryOrbitGatherStartLocation = GatherStartLocation;
	EntryOrbitGatherStartLocation = GatherStartLocation;
	ClientEntryOrbitStartDelaySeconds = FMath::Max(StartDelaySeconds, 0.0f);
	EntryOrbitStartDelaySeconds = ClientEntryOrbitStartDelaySeconds;
	ClientEntryOrbitGatherDurationSeconds = FMath::Max(GatherDurationSeconds, 0.0f);
	EntryOrbitGatherDurationSeconds = ClientEntryOrbitGatherDurationSeconds;
	ClientEntryOrbitPreSpinHoldSeconds = FMath::Max(PreSpinHoldSeconds, 0.0f);
	EntryOrbitPreSpinHoldSeconds = ClientEntryOrbitPreSpinHoldSeconds;
	ClientEntryOrbitDurationSeconds = FMath::Max(OrbitDurationSeconds, 0.0f);
	EntryOrbitDurationSeconds = ClientEntryOrbitDurationSeconds;
	ClientEntryOrbitPostSpinHoldSeconds = FMath::Max(PostSpinHoldSeconds, 0.0f);
	EntryOrbitPostSpinHoldSeconds = ClientEntryOrbitPostSpinHoldSeconds;
	const float TotalDuration = ResolveEntryOrbitTimelineDuration();
	ClientEntryOrbitElapsedSeconds = FMath::Clamp(
		ResolveServerWorldTimeSeconds() - SpawnServerWorldTimeSeconds,
		0.0f,
		TotalDuration);
	EntryOrbitElapsedSeconds = ClientEntryOrbitElapsedSeconds;
	bClientSwordEntryOrbitActive = true;
	UpdateClientEntryOrbitMovement(0.0f);
}

void APRFaerinGodFallStaticSwordActor::StartClientSwordEntryStraightening(
	const FRotator StartRotation,
	const FRotator TargetRotation,
	const float DurationSeconds,
	const float EaseExponent)
{
	bClientSwordEntryOrbitActive = false;
	bClientSwordPresentationActive = false;
	bClientSwordEntryStraighteningActive = true;
	ClientPresentationState = EPRFaerinGodFallStaticSwordState::EntryStraightening;
	ClientEntryStraightenStartRotation = StartRotation;
	ClientEntryStraightenTargetRotation = TargetRotation;
	ClientEntryStraightenElapsedSeconds = 0.0f;
	ClientEntryStraightenDurationSeconds = FMath::Max(DurationSeconds, 0.0f);
	ClientEntryStraightenEaseExponent = FMath::Max(EaseExponent, 0.1f);
	SetActorRotation(StartRotation);

	if (ClientEntryStraightenDurationSeconds <= UE_SMALL_NUMBER)
	{
		SetActorRotation(TargetRotation);
		bClientSwordEntryStraighteningActive = false;
		ClientPresentationState = EPRFaerinGodFallStaticSwordState::None;
		return;
	}

	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::StartClientSwordPresentationSegment(
	const EPRFaerinGodFallStaticSwordState NewState,
	const FVector& StartLocation,
	const FVector& TargetLocation,
	const float DurationSeconds)
{
	if (NewState == EPRFaerinGodFallStaticSwordState::Returning
		|| NewState == EPRFaerinGodFallStaticSwordState::EntryDiving
		|| NewState == EPRFaerinGodFallStaticSwordState::EntryDiveReturning)
	{
		GetWorldTimerManager().ClearTimer(ImpactWarningTimerHandle);
		bImpactWarningSpawned = false;
	}

	bClientSwordEntryOrbitActive = false;
	bClientSwordEntryStraighteningActive = false;
	ClientPresentationState = NewState;
	ClientSegmentStartLocation = StartLocation;
	ClientSegmentTargetLocation = TargetLocation;
	ClientSegmentElapsedSeconds = 0.0f;
	ClientSegmentDurationSeconds = FMath::Max(DurationSeconds, 0.0f);
	bClientSwordPresentationActive = true;

	SetActorLocation(StartLocation, false, nullptr, ETeleportType::TeleportPhysics);

	if (ClientSegmentDurationSeconds <= UE_SMALL_NUMBER)
	{
		SetClientSwordPresentationLocation(TargetLocation);
		return;
	}

	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::StartClientSwordTargetOverhead(
	AActor* AssignedTarget,
	const FVector& InitialOverheadLocation,
	const float MoveDurationSeconds,
	const float MoveAcceleration)
{
	GetWorldTimerManager().ClearTimer(ImpactWarningTimerHandle);
	bImpactWarningSpawned = false;
	ClientAssignedTarget = AssignedTarget;
	ClientAssignedOverheadLocation = InitialOverheadLocation;
	ClientOverheadMoveElapsedSeconds = 0.0f;
	ClientOverheadMoveDurationSeconds = FMath::Max(MoveDurationSeconds, 0.0f);
	ClientOverheadMoveSpeed = 0.0f;
	ClientOverheadMoveAcceleration = FMath::Max(MoveAcceleration, 0.0f);
	ClientPresentationState = EPRFaerinGodFallStaticSwordState::MovingToTargetOverhead;
	bClientSwordPresentationActive = true;

	if (ClientOverheadMoveDurationSeconds <= UE_SMALL_NUMBER)
	{
		SetClientSwordPresentationLocation(InitialOverheadLocation);
		return;
	}

	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::SetClientSwordPresentationLocation(const FVector& Location)
{
	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	bClientSwordPresentationActive = false;
	bClientSwordEntryOrbitActive = false;
	bClientSwordEntryStraighteningActive = false;
	ClientPresentationState = EPRFaerinGodFallStaticSwordState::None;
	RefreshSwordTickEnabled();
}

void APRFaerinGodFallStaticSwordActor::MulticastStartSwordEntryOrbit_Implementation(
	const FVector OrbitCenterLocation,
	const FVector GatherStartLocation,
	const float StartDelaySeconds,
	const float GatherDurationSeconds,
	const float PreSpinHoldSeconds,
	const float OrbitDurationSeconds,
	const float PostSpinHoldSeconds,
	const float SpawnServerWorldTimeSeconds)
{
	if (HasAuthority())
	{
		return;
	}

	StartClientSwordEntryOrbit(
		OrbitCenterLocation,
		GatherStartLocation,
		StartDelaySeconds,
		GatherDurationSeconds,
		PreSpinHoldSeconds,
		OrbitDurationSeconds,
		PostSpinHoldSeconds,
		SpawnServerWorldTimeSeconds);
}

void APRFaerinGodFallStaticSwordActor::MulticastStartSwordEntryStraightening_Implementation(
	FRotator StartRotation,
	FRotator TargetRotation,
	float DurationSeconds,
	float EaseExponent)
{
	if (HasAuthority())
	{
		return;
	}

	StartClientSwordEntryStraightening(StartRotation, TargetRotation, DurationSeconds, EaseExponent);
}

void APRFaerinGodFallStaticSwordActor::MulticastSetSwordPresentationLocation_Implementation(FVector Location)
{
	if (HasAuthority())
	{
		return;
	}

	SetClientSwordPresentationLocation(Location);
}

void APRFaerinGodFallStaticSwordActor::MulticastStartSwordPresentationSegment_Implementation(
	EPRFaerinGodFallStaticSwordState NewState,
	FVector StartLocation,
	FVector TargetLocation,
	float DurationSeconds)
{
	if (HasAuthority())
	{
		return;
	}

	StartClientSwordPresentationSegment(NewState, StartLocation, TargetLocation, DurationSeconds);
}

void APRFaerinGodFallStaticSwordActor::MulticastStartSwordTargetOverhead_Implementation(
	AActor* AssignedTarget,
	FVector InitialOverheadLocation,
	float MoveDurationSeconds,
	float MoveAcceleration)
{
	if (HasAuthority())
	{
		return;
	}

	StartClientSwordTargetOverhead(AssignedTarget, InitialOverheadLocation, MoveDurationSeconds, MoveAcceleration);
}

void APRFaerinGodFallStaticSwordActor::ApplyImpactDamage()
{
	if (!HasAuthority() || !IsValid(GodFallData) || !IsValid(OwnerBoss))
	{
		return;
	}

	UPRAbilitySystemComponent* SourceASC = OwnerBoss->GetEnemyAbilitySystemComponent();
	if (!IsValid(SourceASC))
	{
		return;
	}

	TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = GodFallData->ImpactDamageEffectClass;
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
		UE_LOG(LogPRFaerinGodFallSword, Warning,
			TEXT("God Fall sword damage skipped because damage effect is missing. Sword=%s"),
			*GetNameSafe(this));
		return;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FaerinGodFallImpactOverlap), false, this);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwnerBoss);

	const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(FMath::Max(GodFallData->ImpactRadius, 0.0f));
	const bool bOverlapped = World->OverlapMultiByChannel(
		OverlapResults,
		GetActorLocation(),
		FQuat::Identity,
		GodFallData->ImpactOverlapChannel,
		CollisionShape,
		QueryParams);

	if (GodFallData->bDrawDebug)
	{
		DrawDebugSphere(World, GetActorLocation(), GodFallData->ImpactRadius, 18, FColor::Orange, false, 1.0f);
	}

	if (!bOverlapped)
	{
		return;
	}

	TSet<TWeakObjectPtr<AActor>> DamagedActors;
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* TargetActor = OverlapResult.GetActor();
		if (!IsValid(TargetActor)
			|| DamagedActors.Contains(TargetActor)
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
			GodFallData->ImpactDamageMultiplier);

		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_PoiseDamage,
			GodFallData->ImpactPoiseDamage);

		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		DamagedActors.Add(TargetActor);
	}
}
