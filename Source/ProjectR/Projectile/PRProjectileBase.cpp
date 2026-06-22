// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (투사체 궤적 바운스(반사), 탄약 소모 연동 및 멀티플레이 동기화 구현)
// Author: 손승우 (적 AI 전용 사격 충돌 물리 및 피격 연동 구현)
// Author: 이건주 (특수 충돌 사운드(SFX) 및 파괴 가능 보스 포털 충돌 판정 구현)
#include "PRProjectileBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemInterface.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "PRProjectileMovementComponent.h"
#include "PRProjectileManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "ProjectR/Combat/PRCombatInterface.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/Combat/PRDestructableInterface.h"
#include "ProjectR/System/PRRespawnSubsystem.h"
#include "ProjectR/Combat/PRDirectDamageReceiverInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRProjectileCorrection, Log, All);

namespace PRProjectileBasePrivate
{
	TAutoConsoleVariable<int32> CVarProjectileDebugCorrection(
		TEXT("pr.Projectile.DebugCorrection"),
		0,
		TEXT("Draws projectile RepMovement correction debug spheres and delta text. 0: off, 1: on."),
		ECVF_Default);

	bool ShouldShowProjectileCorrectionDebug()
	{
		return CVarProjectileDebugCorrection.GetValueOnGameThread() != 0;
	}

	FVector GetProjectileVelocity(const APRProjectileBase* Projectile)
	{
		if (!IsValid(Projectile))
		{
			return FVector::ZeroVector;
		}

		const UPRProjectileMovementComponent* MovementComponent =
			Projectile->FindComponentByClass<UPRProjectileMovementComponent>();
		return IsValid(MovementComponent) ? MovementComponent->Velocity : FVector::ZeroVector;
	}

	float GetProjectileVelocityDelta(const APRProjectileBase* Projectile, const FVector& RepVelocity)
	{
		return (GetProjectileVelocity(Projectile) - RepVelocity).Size();
	}

	void DrawProjectileVelocityArrow(
		UWorld* World,
		const FVector& Location,
		const FVector& Velocity,
		const FColor& Color,
		float ArrowLength,
		float ArrowSize,
		float ArrowThickness,
		float DrawLifeTime)
	{
		if (!IsValid(World) || Velocity.IsNearlyZero())
		{
			return;
		}

		DrawDebugDirectionalArrow(
			World,
			Location,
			Location + Velocity.GetSafeNormal() * ArrowLength,
			ArrowSize,
			Color,
			false,
			DrawLifeTime,
			0,
			ArrowThickness);
	}

	void DrawPredictedProjectileEventDebug(const APRProjectileBase* Projectile)
	{
		if (!ShouldShowProjectileCorrectionDebug() || !IsValid(Projectile))
		{
			return;
		}

		UWorld* World = Projectile->GetWorld();
		if (!IsValid(World))
		{
			return;
		}

		const FVector CurrentLocation = Projectile->GetActorLocation();
		const FVector CurrentVelocity = GetProjectileVelocity(Projectile);
		constexpr float SphereRadius = 16.0f;
		constexpr int32 SphereSegments = 12;
		constexpr float DrawLifeTime = 4.0f;
		constexpr float ArrowLength = 120.0f;
		constexpr float ArrowSize = 20.0f;
		constexpr float ArrowThickness = 2.0f;

		DrawDebugSphere(
			World,
			CurrentLocation,
			SphereRadius,
			SphereSegments,
			FColor::Red,
			false,
			DrawLifeTime);

		DrawProjectileVelocityArrow(
			World,
			CurrentLocation,
			CurrentVelocity,
			FColor::Red,
			ArrowLength,
			ArrowSize,
			ArrowThickness,
			DrawLifeTime);
	}

	void DrawProjectileCorrectionRepDebug(
		const APRProjectileBase* Projectile,
		const FPRProjectileRepMovement& Movement,
		const FString& CorrectionLabel,
		const float LocationDelta,
		const float VelocityDelta)
	{
		if (!ShouldShowProjectileCorrectionDebug() || !IsValid(Projectile))
		{
			return;
		}

		UWorld* World = Projectile->GetWorld();
		if (!IsValid(World))
		{
			return;
		}

		constexpr float SphereRadius = 16.0f;
		constexpr int32 SphereSegments = 12;
		constexpr float DrawLifeTime = 4.0f;
		constexpr float TextOffsetZ = 32.0f;
		constexpr float ArrowLength = 120.0f;
		constexpr float ArrowSize = 20.0f;
		constexpr float ArrowThickness = 2.0f;

		DrawDebugSphere(
			World,
			Movement.Location,
			SphereRadius,
			SphereSegments,
			FColor::Green,
			false,
			DrawLifeTime);

		DrawProjectileVelocityArrow(
			World,
			Movement.Location,
			Movement.Velocity,
			FColor::Green,
			ArrowLength,
			ArrowSize,
			ArrowThickness,
			DrawLifeTime);

		const FString DebugText = FString::Printf(
			TEXT("%s\nLocDelta=%.1f VelDelta=%.1f"),
			*CorrectionLabel,
			LocationDelta,
			VelocityDelta);

		DrawDebugString(
			World,
			Movement.Location + FVector(0.0f, 0.0f, TextOffsetZ),
			DebugText,
			nullptr,
			FColor::White,
			DrawLifeTime,
			true);

		const FVector CurrentLocation = Projectile->GetActorLocation();
		const FVector CurrentVelocity = GetProjectileVelocity(Projectile);
		UE_LOG(
			LogPRProjectileCorrection,
			Log,
			TEXT("%s LocDelta=%.1f VelDelta=%.1f RepLoc=%s RepVel=%s LocalLoc=%s LocalVel=%s"),
			*CorrectionLabel,
			LocationDelta,
			VelocityDelta,
			*Movement.Location.ToCompactString(),
			*Movement.Velocity.ToCompactString(),
			*CurrentLocation.ToCompactString(),
			*CurrentVelocity.ToCompactString());
	}
}

APRProjectileBase::APRProjectileBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bReplicates = true;
	SetReplicatingMovement(false);

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SetRootComponent(SphereComponent);
	SphereComponent->SetSphereRadius(30.f);
	SphereComponent->SetCollisionProfileName(TEXT("Projectile"));
	SphereComponent->SetGenerateOverlapEvents(true);
	SphereComponent->OnComponentHit.AddDynamic(this, &APRProjectileBase::OnSphereHit);
	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &APRProjectileBase::OnSphereBeginOverlap);

	ProjectileMovementComponent = CreateDefaultSubobject<UPRProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = SphereComponent;
}

void APRProjectileBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	constexpr bool bUsePushModel = true;
	
	FDoRepLifetimeParams OwnerOnlyParams{COND_OwnerOnly, REPNOTIFY_Always, bUsePushModel};
	DOREPLIFETIME_WITH_PARAMS_FAST(APRProjectileBase, ProjectileId, OwnerOnlyParams);

	FDoRepLifetimeParams NoneParams{COND_None, REPNOTIFY_Always, bUsePushModel};
	DOREPLIFETIME_WITH_PARAMS_FAST(APRProjectileBase, RepMovement, NoneParams);
}

void APRProjectileBase::InitializeProjectile(EPRProjectileRole InRole, uint32 InProjectileId)
{
	ProjectileRole = InRole;
	ProjectileId = InProjectileId;
	CurrentBounceIndex = 0;
	bHasLastReconciledServerBounceIndex = false;
	LastReconciledServerBounceIndex = 0;
	PredictedBounceRecords.Reset();
}

void APRProjectileBase::InitGameplayEffectSpec(const FGameplayEffectSpecHandle& InEffectSpec)
{
	EffectSpecHandle = InEffectSpec;
	if (EffectSpecHandle.Data)
	{
		InstigatorASC =  EffectSpecHandle.Data->GetEffectContext().GetInstigatorAbilitySystemComponent();	
	}
}

void APRProjectileBase::SetProjectileInitialVelocity(const FVector& Direction, float SpeedOverride)
{
	if (!IsValid(ProjectileMovementComponent))
	{
		return;
	}

	const FVector SafeDirection = Direction.GetSafeNormal();
	if (SafeDirection.IsNearlyZero())
	{
		return;
	}

	const float ResolvedSpeed = SpeedOverride > 0.0f
		? SpeedOverride
		: ProjectileMovementComponent->InitialSpeed;

	ProjectileMovementComponent->InitialSpeed = ResolvedSpeed;
	ProjectileMovementComponent->MaxSpeed = FMath::Max(ProjectileMovementComponent->MaxSpeed, ResolvedSpeed);
	ProjectileMovementComponent->Velocity = SafeDirection * ResolvedSpeed;
	SetActorRotation(SafeDirection.Rotation());
}

float APRProjectileBase::GetProjectileInitialSpeed() const
{
	// BP 기본 투사체 속도 조회
	return IsValid(ProjectileMovementComponent)
		? ProjectileMovementComponent->InitialSpeed
		: 0.0f;
}

void APRProjectileBase::ConfigureProjectileHoming(USceneComponent* HomingTargetComponent, float HomingAcceleration)
{
	if (!IsValid(ProjectileMovementComponent))
	{
		return;
	}

	if (!IsValid(HomingTargetComponent) || HomingAcceleration <= 0.0f)
	{
		ProjectileMovementComponent->bIsHomingProjectile = false;
		ProjectileMovementComponent->HomingTargetComponent = nullptr;
		return;
	}

	ProjectileMovementComponent->bIsHomingProjectile = true;
	ProjectileMovementComponent->HomingTargetComponent = HomingTargetComponent;
	ProjectileMovementComponent->HomingAccelerationMagnitude = HomingAcceleration;
}

void APRProjectileBase::ConfigureProjectileHomingSchedule(
	AActor* HomingTargetActor,
	float HomingAcceleration,
	float StartDelay,
	float Duration)
{
	if (!HasAuthority())
	{
		return;
	}

	PendingHomingSchedule.Reset();
	if (!IsValid(HomingTargetActor) || HomingAcceleration <= 0.0f)
	{
		return;
	}

	PendingHomingSchedule.HomingTargetActor = HomingTargetActor;
	PendingHomingSchedule.HomingAcceleration = FMath::Max(HomingAcceleration, 0.0f);
	PendingHomingSchedule.StartDelay = FMath::Max(StartDelay, 0.0f);
	PendingHomingSchedule.Duration = FMath::Max(Duration, 0.0f);
	++PendingHomingSchedule.Revision;
}

void APRProjectileBase::AddProjectileIgnoredActor(AActor* ActorToIgnore)
{
	if (!IsValid(ActorToIgnore))
	{
		return;
	}

	if (IsValid(SphereComponent))
	{
		SphereComponent->IgnoreActorWhenMoving(ActorToIgnore, true);
	}
}

void APRProjectileBase::ApplyFastForward(float ForwardSeconds)
{
	if (!HasAuthority() || !bUseFastForward || ForwardSeconds <= 0.f)
	{
		return;
	}

	if (!IsValid(ProjectileMovementComponent))
	{
		return;
	}

	// PMC를 강제 틱하여 투사체를 ForwardSeconds 만큼 전진. bForceSubStepping=true로 내부 서브스텝 분할 수행
	ProjectileMovementComponent->TickComponent(ForwardSeconds, LEVELTICK_All, nullptr);

	// 수명 보정. Fast-Forward로 소모된 시간만큼 남은 수명에서 차감
	const float RemainingLife = GetLifeSpan();
	if (RemainingLife > 0.f)
	{
		SetLifeSpan(FMath::Max(RemainingLife - ForwardSeconds, 0.1f));
	}
}

void APRProjectileBase::PushRepMovement(EPRRepMovementEvent Event)
{
	if (!HasAuthority())
	{
		return;
	}

	RepMovement.Event    = Event;
	RepMovement.Location = GetActorLocation();
	RepMovement.Rotation = GetActorRotation();
	RepMovement.Velocity = IsValid(ProjectileMovementComponent)
		? ProjectileMovementComponent->Velocity
		: FVector::ZeroVector;
	RepMovement.BounceIndex = Event == EPRRepMovementEvent::Bounce ? CurrentBounceIndex : 0;
	if (Event == EPRRepMovementEvent::Spawn)
	{
		RepMovement.HomingSchedule = PendingHomingSchedule;
	}
	else
	{
		RepMovement.HomingSchedule.Reset();
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(APRProjectileBase, RepMovement, this);
	ForceNetUpdate();

	if (Event == EPRRepMovementEvent::Spawn)
	{
		StartProjectileHomingSchedule(RepMovement.HomingSchedule);
		if (RepMovement.HomingSchedule.IsEnabled())
		{
			MulticastStartProjectileHomingPresentation(
				RepMovement.HomingSchedule.HomingTargetActor.Get(),
				RepMovement.HomingSchedule.HomingAcceleration,
				RepMovement.HomingSchedule.StartDelay,
				RepMovement.HomingSchedule.Duration,
				RepMovement.HomingSchedule.Revision);
		}
	}
}

void APRProjectileBase::NotifyProjectileBounce()
{
	// 바운스 이벤트 순서 번호
	++CurrentBounceIndex;
	if (CurrentBounceIndex == 0)
	{
		CurrentBounceIndex = 1;
	}

	if (ProjectileRole == EPRProjectileRole::Predicted)
	{
		RecordPredictedBounce();
		PRProjectileBasePrivate::DrawPredictedProjectileEventDebug(this);
	}
}

void APRProjectileBase::OnRep_RepMovement()
{
	switch (RepMovement.Event)
	{
	case EPRRepMovementEvent::Spawn:
		HandleRepSpawn();
		break;
	case EPRRepMovementEvent::Bounce:
	case EPRRepMovementEvent::Detonation:
		HandleRepCorrection();
		break;
	case EPRRepMovementEvent::Stop:
		HandleRepStop();
		break;
	case EPRRepMovementEvent::Fall:
		HandleRepFall();
		break;
	default:
		break;
	}
}

void APRProjectileBase::StartProjectileHomingSchedule(const FPRProjectileRepHomingSchedule& HomingSchedule)
{
	ClearProjectileHomingScheduleTimers();
	if (!HomingSchedule.IsEnabled())
	{
		return;
	}

	if (HomingSchedule.StartDelay <= 0.0f)
	{
		ApplyProjectileHomingScheduleStart(HomingSchedule);
		return;
	}

	GetWorldTimerManager().SetTimer(
		ProjectileHomingStartTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, HomingSchedule]()
		{
			ApplyProjectileHomingScheduleStart(HomingSchedule);
		}),
		HomingSchedule.StartDelay,
		false);
}

void APRProjectileBase::ApplyProjectileHomingScheduleStart(FPRProjectileRepHomingSchedule HomingSchedule)
{
	AActor* HomingTargetActor = HomingSchedule.HomingTargetActor.Get();
	USceneComponent* HomingTargetComponent = IsValid(HomingTargetActor)
		? HomingTargetActor->GetRootComponent()
		: nullptr;
	if (!IsValid(HomingTargetComponent))
	{
		return;
	}

	ConfigureProjectileHoming(HomingTargetComponent, HomingSchedule.HomingAcceleration);
	if (HomingSchedule.Duration <= 0.0f)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		ProjectileHomingStopTimerHandle,
		this,
		&APRProjectileBase::StopProjectileHomingSchedule,
		HomingSchedule.Duration,
		false);
}

void APRProjectileBase::StopProjectileHomingSchedule()
{
	ConfigureProjectileHoming(nullptr, 0.0f);
}

void APRProjectileBase::ClearProjectileHomingScheduleTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ProjectileHomingStartTimerHandle);
		World->GetTimerManager().ClearTimer(ProjectileHomingStopTimerHandle);
	}
}

bool APRProjectileBase::IsRemoteAuthProjectilePresentation() const
{
	if (HasAuthority() || ProjectileRole != EPRProjectileRole::Auth)
	{
		return false;
	}

	const APlayerController* NetOwnerPlayerController = Cast<APlayerController>(GetNetOwner());
	return !IsValid(NetOwnerPlayerController) || !NetOwnerPlayerController->IsLocalController();
}

void APRProjectileBase::StartClientProjectileHomingPresentation(const FPRProjectileRepHomingSchedule& HomingSchedule)
{
	bIsRemoteProjectile = IsRemoteAuthProjectilePresentation();
	if (!bIsRemoteProjectile || !HomingSchedule.IsEnabled())
	{
		return;
	}

	if ((bClientProjectilePresentationActive || bHasPendingClientHomingPresentation)
		&& ClientPresentationRevision == HomingSchedule.Revision)
	{
		return;
	}

	ClientPresentationRevision = HomingSchedule.Revision;
	if (!bHasRepSpawnHandled)
	{
		PendingClientHomingPresentationSchedule = HomingSchedule;
		bHasPendingClientHomingPresentation = true;
		return;
	}

	bHasPendingClientHomingPresentation = false;
	PendingClientHomingPresentationSchedule.Reset();
	ClientPresentationHomingTarget = HomingSchedule.HomingTargetActor;
	ClientPresentationHomingAcceleration = FMath::Max(HomingSchedule.HomingAcceleration, 0.0f);
	ClientPresentationHomingStartDelay = FMath::Max(HomingSchedule.StartDelay, 0.0f);
	ClientPresentationHomingDuration = FMath::Max(HomingSchedule.Duration, 0.0f);
	ClientPresentationElapsedSeconds = 0.0f;
	ClientPresentationHomingElapsedSeconds = 0.0f;
	bClientProjectileHomingStarted = ClientPresentationHomingStartDelay <= UE_SMALL_NUMBER;
	bClientProjectilePresentationActive = true;

	ClientPresentationVelocity = RepMovement.Velocity;
	if (ClientPresentationVelocity.IsNearlyZero() && IsValid(ProjectileMovementComponent))
	{
		ClientPresentationVelocity = ProjectileMovementComponent->Velocity;
	}
	if (ClientPresentationVelocity.IsNearlyZero())
	{
		const float ResolvedSpeed = IsValid(ProjectileMovementComponent)
			? ProjectileMovementComponent->InitialSpeed
			: 0.0f;
		ClientPresentationVelocity = GetActorForwardVector() * ResolvedSpeed;
	}

	if (IsValid(ProjectileMovementComponent))
	{
		ProjectileMovementComponent->Deactivate();
		ConfigureProjectileHoming(nullptr, 0.0f);
	}

	SetActorEnableCollision(false);
	SetActorTickEnabled(true);
}

void APRProjectileBase::UpdateClientProjectileHomingPresentation(const float DeltaSeconds)
{
	if (!bClientProjectilePresentationActive || DeltaSeconds <= 0.0f)
	{
		return;
	}

	ClientPresentationElapsedSeconds += DeltaSeconds;
	if (!bClientProjectileHomingStarted
		&& ClientPresentationElapsedSeconds >= ClientPresentationHomingStartDelay)
	{
		bClientProjectileHomingStarted = true;
	}

	const bool bCanApplyHoming = bClientProjectileHomingStarted
		&& (ClientPresentationHomingDuration <= 0.0f
			|| ClientPresentationHomingElapsedSeconds < ClientPresentationHomingDuration);
	if (bCanApplyHoming)
	{
		if (AActor* HomingTargetActor = ClientPresentationHomingTarget.Get())
		{
			const FVector ToTarget = HomingTargetActor->GetActorLocation() - GetActorLocation();
			if (!ToTarget.IsNearlyZero())
			{
				ClientPresentationVelocity += ToTarget.GetSafeNormal()
					* ClientPresentationHomingAcceleration
					* DeltaSeconds;

				const float MaxSpeed = IsValid(ProjectileMovementComponent)
					? FMath::Max(ProjectileMovementComponent->MaxSpeed, ProjectileMovementComponent->InitialSpeed)
					: ClientPresentationVelocity.Size();
				if (MaxSpeed > UE_SMALL_NUMBER)
				{
					ClientPresentationVelocity = ClientPresentationVelocity.GetClampedToMaxSize(MaxSpeed);
				}
			}
		}

		if (ClientPresentationHomingDuration > 0.0f)
		{
			ClientPresentationHomingElapsedSeconds += DeltaSeconds;
		}
	}

	const FVector MoveDelta = ClientPresentationVelocity * DeltaSeconds;
	if (!MoveDelta.IsNearlyZero())
	{
		const FRotator NextRotation = ClientPresentationVelocity.GetSafeNormal().Rotation();
		SetActorLocationAndRotation(
			GetActorLocation() + MoveDelta,
			NextRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}
}

void APRProjectileBase::StopClientProjectileHomingPresentation()
{
	bClientProjectilePresentationActive = false;
	bClientProjectileHomingStarted = false;
	bHasPendingClientHomingPresentation = false;
	PendingClientHomingPresentationSchedule.Reset();
	ClientPresentationHomingTarget.Reset();
	ClientPresentationVelocity = FVector::ZeroVector;
	ClientPresentationHomingAcceleration = 0.0f;
	ClientPresentationHomingStartDelay = 0.0f;
	ClientPresentationHomingDuration = 0.0f;
	ClientPresentationElapsedSeconds = 0.0f;
	ClientPresentationHomingElapsedSeconds = 0.0f;
}

void APRProjectileBase::MulticastStartProjectileHomingPresentation_Implementation(
	AActor* HomingTargetActor,
	const float HomingAcceleration,
	const float StartDelay,
	const float Duration,
	const uint8 Revision)
{
	if (HasAuthority())
	{
		return;
	}

	FPRProjectileRepHomingSchedule HomingSchedule;
	HomingSchedule.HomingTargetActor = HomingTargetActor;
	HomingSchedule.HomingAcceleration = HomingAcceleration;
	HomingSchedule.StartDelay = StartDelay;
	HomingSchedule.Duration = Duration;
	HomingSchedule.Revision = Revision;
	StartClientProjectileHomingPresentation(HomingSchedule);
}

void APRProjectileBase::HandleRepSpawn()
{
	bIsRemoteProjectile = IsRemoteAuthProjectilePresentation();
	const float LocationDelta = FVector::Dist(GetActorLocation(), RepMovement.Location);
	const float VelocityDelta = PRProjectileBasePrivate::GetProjectileVelocityDelta(this, RepMovement.Velocity);
	if (!bIsRemoteProjectile)
	{
		return;
	}

	PRProjectileBasePrivate::DrawProjectileCorrectionRepDebug(
		this,
		RepMovement,
		TEXT("Spawn Correction"),
		LocationDelta,
		VelocityDelta);
	
	SetActorLocation(RepMovement.Location);
	SetActorRotation(RepMovement.Rotation);

	if (IsValid(ProjectileMovementComponent))
	{
		ProjectileMovementComponent->Velocity = RepMovement.Velocity;
		if (RepMovement.HomingSchedule.IsEnabled() || bHasPendingClientHomingPresentation)
		{
			ProjectileMovementComponent->Deactivate();
		}
		else
		{
			ProjectileMovementComponent->Activate();
		}
	}

	// SimulatedProxy에서 숨김 처리 했던 상태를 복구
	SetActorHiddenInGame(false);
	// 원격 클라이언트는 서버 판정 결과만 따르고, 로컬 충돌로 시각용 궤적이 멈추지 않게 한다.
	SetActorEnableCollision(false);
	
	bHasRepSpawnHandled = true;
	if (bHasPendingClientHomingPresentation)
	{
		const FPRProjectileRepHomingSchedule HomingSchedule = PendingClientHomingPresentationSchedule;
		StopClientProjectileHomingPresentation();
		StartClientProjectileHomingPresentation(HomingSchedule);
	}
	else if (RepMovement.HomingSchedule.IsEnabled())
	{
		StartClientProjectileHomingPresentation(RepMovement.HomingSchedule);
	}
}

void APRProjectileBase::HandleRepCorrection()
{
	// SimulatedProxy가 받아야 할 Spawn이벤트가 곧바로 Correction이벤트로 덮어 씌워진 경우 Spawn을 처리
	if (GetLocalRole() == ROLE_SimulatedProxy && !bHasRepSpawnHandled)
	{
		HandleRepSpawn();
	}

	if (RepMovement.Event == EPRRepMovementEvent::Bounce && LinkedCounterpart.IsValid())
	{
		APRProjectileBase* PredictedProjectile = LinkedCounterpart.Get();
		if (IsValid(PredictedProjectile)
			&& PredictedProjectile->GetProjectileRole() == EPRProjectileRole::Predicted)
		{
			// 소유 클라이언트의 예측 투사체 기준 바운스 보정
			PredictedProjectile->ReconcilePredictedBounceFromServer(RepMovement);
			SyncAuthPresentationToPredicted(PredictedProjectile);
			return;
		}
	}

	const float LocationDelta = FVector::Dist(GetActorLocation(), RepMovement.Location);
	const float VelocityDelta = PRProjectileBasePrivate::GetProjectileVelocityDelta(this, RepMovement.Velocity);
	PRProjectileBasePrivate::DrawProjectileCorrectionRepDebug(
		this,
		RepMovement,
		RepMovement.Event == EPRRepMovementEvent::Bounce
			? TEXT("Bounce Correction")
			: TEXT("Detonation Correction"),
		LocationDelta,
		VelocityDelta);
	
	// Location, Rotation, Velocity를 서버 기준으로 동기화
	SetActorLocation(RepMovement.Location);
	SetActorRotation(RepMovement.Rotation);

	if (IsValid(ProjectileMovementComponent))
	{
		ProjectileMovementComponent->Velocity = RepMovement.Velocity;
	}

	if (RepMovement.Event == EPRRepMovementEvent::Detonation)
	{
		// 링크된 예측 투사체가 아직 남아 있는 경우
		if (LinkedCounterpart.IsValid())
		{
			// 예측 투사체 파괴
			LinkedCounterpart->Destroy();
		
			// 가시성 복구
			SetActorHiddenInGame(false);
			SetActorEnableCollision(true);
		}
		
		DestroyProjectile(EPRProjectileDestroyReason::ReplicatedDetonation);
	}
}

bool APRProjectileBase::ReconcilePredictedBounceFromServer(const FPRProjectileRepMovement& ServerMovement)
{
	if (ProjectileRole != EPRProjectileRole::Predicted || ServerMovement.Event != EPRRepMovementEvent::Bounce)
	{
		return false;
	}

	if (bHasLastReconciledServerBounceIndex && LastReconciledServerBounceIndex == ServerMovement.BounceIndex)
	{
		const float LocationDelta = FVector::Dist(GetActorLocation(), ServerMovement.Location);
		const float VelocityDelta = PRProjectileBasePrivate::GetProjectileVelocityDelta(this, ServerMovement.Velocity);
		PRProjectileBasePrivate::DrawProjectileCorrectionRepDebug(
			this,
			ServerMovement,
			TEXT("Bounce Duplicate Ignored"),
			LocationDelta,
			VelocityDelta);
		return true;
	}

	FPRPredictedProjectileBounceRecord PredictedRecord;
	const EPRPredictedBounceMatchResult MatchResult = ConsumePredictedBounceRecord(
		ServerMovement.BounceIndex,
		ServerMovement.Location,
		BounceCorrectionToleranceDistance,
		PredictedRecord);
	if (MatchResult == EPRPredictedBounceMatchResult::Missing)
	{
		const float LocationDelta = FVector::Dist(GetActorLocation(), ServerMovement.Location);
		const float VelocityDelta = PRProjectileBasePrivate::GetProjectileVelocityDelta(this, ServerMovement.Velocity);
		PRProjectileBasePrivate::DrawProjectileCorrectionRepDebug(
			this,
			ServerMovement,
			TEXT("Bounce Missing Prediction"),
			LocationDelta,
			VelocityDelta);

		// 예측 기록 누락 시 서버 바운스 기준 보정
		ApplyServerBounceCorrection(ServerMovement, true, bUseBounceVelocityCorrection);
		bHasLastReconciledServerBounceIndex = true;
		LastReconciledServerBounceIndex = ServerMovement.BounceIndex;
		return true;
	}

	const float LocationDelta = FVector::Dist(PredictedRecord.Location, ServerMovement.Location);
	const float VelocityDelta = (PredictedRecord.Velocity - ServerMovement.Velocity).Size();
	const bool bNeedsLocationCorrection = LocationDelta > BounceCorrectionToleranceDistance;
	const bool bNeedsVelocityCorrection = bUseBounceVelocityCorrection
		&& VelocityDelta > BounceCorrectionToleranceVelocity;
	const bool bIndexMismatch = MatchResult == EPRPredictedBounceMatchResult::LocationFallback;

	if (!bNeedsLocationCorrection && !bNeedsVelocityCorrection)
	{
		PRProjectileBasePrivate::DrawProjectileCorrectionRepDebug(
			this,
			ServerMovement,
			bIndexMismatch
				? TEXT("Bounce Index Mismatch Within Tolerance")
				: TEXT("Bounce Within Tolerance"),
			LocationDelta,
			VelocityDelta);

		// 허용 오차 이내 예측 유지
		bHasLastReconciledServerBounceIndex = true;
		LastReconciledServerBounceIndex = ServerMovement.BounceIndex;
		return true;
	}

	const FString CorrectionLabel = bIndexMismatch
		? (bNeedsLocationCorrection && bNeedsVelocityCorrection
			? TEXT("Bounce Index Mismatch Location/Velocity Correction")
			: (bNeedsLocationCorrection
				? TEXT("Bounce Index Mismatch Location Correction")
				: TEXT("Bounce Index Mismatch Velocity Correction")))
		: (bNeedsLocationCorrection && bNeedsVelocityCorrection
			? TEXT("Bounce Location/Velocity Correction")
			: (bNeedsLocationCorrection ? TEXT("Bounce Location Correction") : TEXT("Bounce Velocity Correction")));
	PRProjectileBasePrivate::DrawProjectileCorrectionRepDebug(
		this,
		ServerMovement,
		CorrectionLabel,
		LocationDelta,
		VelocityDelta);

	ApplyServerBounceCorrection(ServerMovement, bNeedsLocationCorrection, bNeedsVelocityCorrection);
	bHasLastReconciledServerBounceIndex = true;
	LastReconciledServerBounceIndex = ServerMovement.BounceIndex;
	return true;
}

void APRProjectileBase::RecordPredictedBounce()
{
	FPRPredictedProjectileBounceRecord NewRecord;
	NewRecord.BounceIndex = CurrentBounceIndex;
	NewRecord.Location = GetActorLocation();
	NewRecord.Rotation = GetActorRotation();
	NewRecord.Velocity = IsValid(ProjectileMovementComponent)
		? ProjectileMovementComponent->Velocity
		: FVector::ZeroVector;

	// 최근 바운스 기록 보관
	constexpr int32 MaxPredictedBounceRecordCount = 8;
	PredictedBounceRecords.Add(NewRecord);
	while (PredictedBounceRecords.Num() > MaxPredictedBounceRecordCount)
	{
		PredictedBounceRecords.RemoveAt(0, 1, EAllowShrinking::No);
	}
}

EPRPredictedBounceMatchResult APRProjectileBase::ConsumePredictedBounceRecord(
	const uint8 ServerBounceIndex,
	const FVector& ServerLocation,
	const float LocationTolerance,
	FPRPredictedProjectileBounceRecord& OutRecord)
{
	const int32 FoundIndex = PredictedBounceRecords.IndexOfByPredicate(
		[ServerBounceIndex](const FPRPredictedProjectileBounceRecord& Record)
		{
			return Record.BounceIndex == ServerBounceIndex;
		});

	if (FoundIndex == INDEX_NONE)
	{
		const int32 LocationFallbackIndex = PredictedBounceRecords.IndexOfByPredicate(
			[&ServerLocation, LocationTolerance](const FPRPredictedProjectileBounceRecord& Record)
			{
				return FVector::Dist(Record.Location, ServerLocation) <= LocationTolerance;
			});

		if (LocationFallbackIndex == INDEX_NONE)
		{
			return EPRPredictedBounceMatchResult::Missing;
		}

		OutRecord = PredictedBounceRecords[LocationFallbackIndex];
		PredictedBounceRecords.RemoveAt(0, LocationFallbackIndex + 1, EAllowShrinking::No);
		return EPRPredictedBounceMatchResult::LocationFallback;
	}

	OutRecord = PredictedBounceRecords[FoundIndex];
	PredictedBounceRecords.RemoveAt(0, FoundIndex + 1, EAllowShrinking::No);
	return EPRPredictedBounceMatchResult::ExactIndex;
}

void APRProjectileBase::ApplyServerBounceCorrection(
	const FPRProjectileRepMovement& ServerMovement,
	const bool bCorrectLocation,
	const bool bCorrectVelocity)
{
	if (bCorrectLocation)
	{
		// 서버 바운스 위치 기준 보정
		SetActorLocationAndRotation(
			ServerMovement.Location,
			ServerMovement.Rotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}

	if (bCorrectVelocity && IsValid(ProjectileMovementComponent))
	{
		// 서버 바운스 속도 기준 보정
		ProjectileMovementComponent->Velocity = ServerMovement.Velocity;
	}
}

void APRProjectileBase::SyncAuthPresentationToPredicted(APRProjectileBase* PredictedProjectile)
{
	if (!IsValid(PredictedProjectile))
	{
		return;
	}

	// 숨겨진 Auth 투사체가 예측 투사체를 다시 끌어당기지 않도록 상태 동기화
	SetActorLocation(PredictedProjectile->GetActorLocation());
	SetActorRotation(PredictedProjectile->GetActorRotation());

	if (IsValid(ProjectileMovementComponent))
	{
		if (const UPRProjectileMovementComponent* PredictedMovement =
			PredictedProjectile->FindComponentByClass<UPRProjectileMovementComponent>())
		{
			ProjectileMovementComponent->Velocity = PredictedMovement->Velocity;
		}
	}
}

void APRProjectileBase::HandleRepStop()
{
	// Spawn 선처리
	if (GetLocalRole() == ROLE_SimulatedProxy && !bHasRepSpawnHandled)
	{
		HandleRepSpawn();
	}

	const float LocationDelta = FVector::Dist(GetActorLocation(), RepMovement.Location);
	const float VelocityDelta = PRProjectileBasePrivate::GetProjectileVelocityDelta(this, RepMovement.Velocity);
	PRProjectileBasePrivate::DrawProjectileCorrectionRepDebug(
		this,
		RepMovement,
		TEXT("Stop Correction"),
		LocationDelta,
		VelocityDelta);

	// 서버 최종 위치 보정
	SetActorLocation(RepMovement.Location);
	SetActorRotation(RepMovement.Rotation);

	// 예측 투사체 정리
	if (LinkedCounterpart.IsValid())
	{
		LinkedCounterpart->Destroy();
		LinkedCounterpart.Reset();
	}

	// 정지 상태 적용
	StopProjectileMotion();
	bHasRepSpawnHandled = true;

	// 정지 후 수명 처리
	ApplyFinalImpactStayLifeSpan();
}

void APRProjectileBase::HandleRepFall()
{
	// Spawn 선처리
	if (GetLocalRole() == ROLE_SimulatedProxy && !bHasRepSpawnHandled)
	{
		HandleRepSpawn();
	}

	const float LocationDelta = FVector::Dist(GetActorLocation(), RepMovement.Location);
	const float VelocityDelta = PRProjectileBasePrivate::GetProjectileVelocityDelta(this, RepMovement.Velocity);
	PRProjectileBasePrivate::DrawProjectileCorrectionRepDebug(
		this,
		RepMovement,
		TEXT("Fall Correction"),
		LocationDelta,
		VelocityDelta);

	// 서버 낙하 시작 위치 보정
	SetActorLocation(RepMovement.Location);
	SetActorRotation(RepMovement.Rotation);

	// 예측 투사체 정리
	if (LinkedCounterpart.IsValid())
	{
		LinkedCounterpart->Destroy();
		LinkedCounterpart.Reset();
	}

	// 중력 낙하 시작
	if (!StartFinalImpactFall())
	{
		StopProjectileMotion();
		ApplyFinalImpactStayLifeSpan();
	}
	bHasRepSpawnHandled = true;
}

void APRProjectileBase::HandleFinalImpact(const FHitResult& Hit)
{
	// 기존 파괴 정책
	if (FinalImpactPolicy == EPRProjectileFinalImpactPolicy::Destroy)
	{
		DestroyProjectile(EPRProjectileDestroyReason::Impact);
		return;
	}

	// 정지 유지 정책
	StopAndStayAtImpact(Hit);
}

void APRProjectileBase::StopAndStayAtImpact(const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		return;
	}

	// 최종 충돌 위치 보정
	FVector StopLocation = GetActorLocation();
	if (!Hit.Location.IsNearlyZero())
	{
		StopLocation = FVector(Hit.Location);
	}
	SetActorLocation(StopLocation);

	// 예측 투사체 정리
	if (LinkedCounterpart.IsValid())
	{
		LinkedCounterpart->Destroy();
		LinkedCounterpart.Reset();
	}

	// 중력 낙하 시작
	if (!StartFinalImpactFall())
	{
		StopProjectileMotion();
		ApplyFinalImpactStayLifeSpan();
		PushRepMovement(EPRRepMovementEvent::Stop);
		OnProjectileStoppedAtImpact(Hit);
		return;
	}

	// 낙하 이벤트 동기화
	PushRepMovement(EPRRepMovementEvent::Fall);
}

bool APRProjectileBase::StartFinalImpactFall()
{
	if (!IsValid(SphereComponent) || !IsValid(ProjectileMovementComponent))
	{
		return false;
	}

	// 호밍 정리
	ClearProjectileHomingScheduleTimers();
	StopClientProjectileHomingPresentation();
	ConfigureProjectileHoming(nullptr, 0.0f);

	// 낙하 상태 설정
	bIsFinalImpactFalling = true;
	bShouldSyncToAuth = false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	// 낙하 충돌 복구
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SphereComponent->SetGenerateOverlapEvents(false);
	SphereComponent->SetNotifyRigidBodyCollision(true);

	// 이동 컴포넌트 정지
	ProjectileMovementComponent->StopMovementImmediately();
	ProjectileMovementComponent->Velocity = FVector::ZeroVector;
	ProjectileMovementComponent->Deactivate();
	ProjectileMovementComponent->SetUpdatedComponent(nullptr);
	ProjectileMovementComponent->SetComponentTickEnabled(false);

	// 물리 낙하 시작
	SphereComponent->SetSimulatePhysics(true);
	SphereComponent->SetEnableGravity(true);
	SphereComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
	SphereComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	SphereComponent->WakeAllRigidBodies();

	return true;
}

void APRProjectileBase::SettleFinalImpactFall(const FHitResult& Hit)
{
	if (!bIsFinalImpactFalling)
	{
		return;
	}

	// 바닥성 충돌 판정
	const FVector ImpactNormal = !Hit.ImpactNormal.IsNearlyZero()
		? FVector(Hit.ImpactNormal)
		: FVector(Hit.Normal);
	constexpr float GroundNormalZThreshold = 0.35f;
	if (ImpactNormal.Z < GroundNormalZThreshold)
	{
		// 벽면 접촉 이탈
		if (!ImpactNormal.IsNearlyZero())
		{
			SetActorLocation(GetActorLocation() + ImpactNormal.GetSafeNormal() * 2.0f);
		}

		// 중력 낙하 유지
		if (!StartFinalImpactFall())
		{
			StopProjectileMotion();
			ApplyFinalImpactStayLifeSpan();
			if (HasAuthority())
			{
				PushRepMovement(EPRRepMovementEvent::Stop);
				OnProjectileStoppedAtImpact(Hit);
			}
		}
		return;
	}

	// 최종 정착 위치 보정
	FVector StopLocation = GetActorLocation();
	if (!Hit.Location.IsNearlyZero())
	{
		StopLocation = FVector(Hit.Location);
	}
	SetActorLocation(StopLocation);

	// 정착 상태 적용
	StopProjectileMotion();

	// 정지 후 수명 처리
	ApplyFinalImpactStayLifeSpan();

	if (HasAuthority())
	{
		// 정착 이벤트 동기화
		PushRepMovement(EPRRepMovementEvent::Stop);

		// BP 정지 확장 지점
		OnProjectileStoppedAtImpact(Hit);
	}
}

void APRProjectileBase::StopProjectileMotion()
{
	// 호밍 정리
	ClearProjectileHomingScheduleTimers();
	StopClientProjectileHomingPresentation();
	ConfigureProjectileHoming(nullptr, 0.0f);

	// 이동 정지
	if (IsValid(ProjectileMovementComponent))
	{
		ProjectileMovementComponent->StopMovementImmediately();
		ProjectileMovementComponent->Velocity = FVector::ZeroVector;
		ProjectileMovementComponent->Deactivate();
		ProjectileMovementComponent->SetComponentTickEnabled(false);
	}

	// 추가 충돌 방지
	if (IsValid(SphereComponent))
	{
		SphereComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
		SphereComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		SphereComponent->SetSimulatePhysics(false);
		SphereComponent->SetEnableGravity(false);
		SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SphereComponent->SetGenerateOverlapEvents(false);
	}

	// 정지 프레젠테이션 유지
	bIsFinalImpactFalling = false;
	bShouldSyncToAuth = false;
	SetActorHiddenInGame(false);
}

void APRProjectileBase::ApplyFinalImpactStayLifeSpan()
{
	// 정지 후 수명 처리
	if (FinalImpactStayDuration > 0.0f)
	{
		SetLifeSpan(FinalImpactStayDuration);
	}
	else
	{
		SetLifeSpan(0.0f);
	}
}


void APRProjectileBase::DestroyProjectile(EPRProjectileDestroyReason DestroyReason)
{
	if (ProjectileRole == EPRProjectileRole::Predicted)
	{
		PRProjectileBasePrivate::DrawPredictedProjectileEventDebug(this);
	}

	ClearProjectileHomingScheduleTimers();
	StopClientProjectileHomingPresentation();
	ConfigureProjectileHoming(nullptr, 0.0f);

	OnProjectileDestroyEffectStarted(DestroyReason);

	if (HasAuthority())
	{
		PushRepMovement(EPRRepMovementEvent::Detonation);
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);

		// 리플리케이션 채널이 Detonation RepMovement를 클라이언트에 전달할 시간 확보
		// 2.5 net frame: 1프레임 이상 생존 보장, 불필요하게 길지 않은 최솟값
		const UNetDriver* NetDriver = GetWorld()->GetNetDriver();
		const float NetTickRate = (NetDriver && NetDriver->GetNetServerMaxTickRate() > 0.f)
			? NetDriver->GetNetServerMaxTickRate()
			: 30.f;
		SetLifeSpan(2.5f / NetTickRate);
	}
	else
	{
		Destroy();
	}
	
	OnProjectileDestroyed();
}

void APRProjectileBase::OnProjectileDestroyed_Implementation()
{
}

void APRProjectileBase::OnProjectileDestroyEffectStarted_Implementation(EPRProjectileDestroyReason DestroyReason)
{
	if (PRShouldPlayProjectileDestroyEffect(DestroyReason) && IsValid(ExplodeSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());
	}
}

void APRProjectileBase::OnProjectileStoppedAtImpact_Implementation(const FHitResult& /*Hit*/)
{
	// BP 확장 지점
}

bool APRProjectileBase::HasProjectileAuthority() const
{
	return ProjectileRole == EPRProjectileRole::Auth && HasAuthority();
}

bool APRProjectileBase::GetShouldBounce() const
{
	return IsValid(ProjectileMovementComponent)
		? ProjectileMovementComponent->GetShouldBounce()
		: false;
}

void APRProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	// 투사체 궤적은 RepMovement 이벤트와 로컬 시뮬레이션으로 맞춘다.
	SetReplicatingMovement(false);

	// 비행 사운드: 루트에 attach해 투사체를 따라 재생한다. (각 머신에서 로컬 재생, 데디케이티드 서버에선 무시)
	// bStopWhenAttachedToDestroyed=true: 투사체가 파괴/소멸되면 사운드도 함께 정지·정리된다. (루프 큐 잔류 방지)
	if (IsValid(FlightSoundCue) && IsValid(SphereComponent))
	{
		UGameplayStatics::SpawnSoundAttached(
			FlightSoundCue,
			SphereComponent,
			NAME_None,
			FVector(ForceInit),
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			/*bStopWhenAttachedToDestroyed=*/true);
	}

	// 발사자 충돌 제외
	AActor* IgnoredInstigator = GetInstigator();
	if (!IsValid(IgnoredInstigator))
	{
		if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			IgnoredInstigator = OwnerPawn;
		}
		else if (AController* OwnerController = Cast<AController>(GetOwner()))
		{
			IgnoredInstigator = OwnerController->GetPawn();
		}
	}

	if (IsValid(IgnoredInstigator))
	{
		if (IsValid(SphereComponent))
		{
			SphereComponent->IgnoreActorWhenMoving(IgnoredInstigator, true);
		}
	}
	
	// 클라이언트 원격 투사체는 Spawn payload를 받은 뒤 프레젠테이션을 시작한다.
	bIsRemoteProjectile = IsRemoteAuthProjectilePresentation();
	
	if (bIsRemoteProjectile)
	{
		// RepMovement가 BeginPlay 이전에 이미 도착한 경우 즉시 처리
		if (RepMovement.Event == EPRRepMovementEvent::Spawn && !RepMovement.Location.IsZero())
		{
			HandleRepSpawn();
		}
		else if (RepMovement.Event == EPRRepMovementEvent::Stop && !RepMovement.Location.IsZero())
		{
			HandleRepStop();
		}
		else if (RepMovement.Event == EPRRepMovementEvent::Fall && !RepMovement.Location.IsZero())
		{
			HandleRepFall();
		}
		else
		{
			// 스폰 이벤트 RepMovement 수신 전까지 숨김. OnRep_RepMovement에서 언히든
			SetActorHiddenInGame(true);
			SetActorEnableCollision(false);
			if (IsValid(ProjectileMovementComponent))
			{
				ProjectileMovementComponent->Deactivate();
			}
			SetLifeSpan(0.3f);
		}
		return;
	}

	if (ProjectileRole == EPRProjectileRole::Predicted)
	{
		PRProjectileBasePrivate::DrawPredictedProjectileEventDebug(this);
	}

	if (HasProjectileAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (UPRRespawnSubsystem* RespawnSubsystem = World->GetSubsystem<UPRRespawnSubsystem>())
			{
				// 일회성 투사체 등록
				RespawnSubsystem->RegisterDisposableActor(this);
			}
		}
	}

#if WITH_EDITOR
	DrawDebugs(0);
#endif
}

void APRProjectileBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if WITH_EDITOR
	DrawDebugs(DeltaSeconds);
#endif

	if (bIsRemoteProjectile && bClientProjectilePresentationActive)
	{
		UpdateClientProjectileHomingPresentation(DeltaSeconds);
		return;
	}
	
	if (bShouldSyncToAuth && LinkedCounterpart.IsValid())
	{
		const FVector AuthLocation = LinkedCounterpart->GetActorLocation();
		const FVector Delta = AuthLocation - GetActorLocation();

		// 현재 이동 방향 기준으로 Auth가 앞/뒤 판별
		// Dot > 0: Auth가 진행 방향 앞: 빠르게 따라잡음
		// Dot < 0: Auth가 진행 방향 뒤: 천천히 늦춤
		const FVector TravelDir = IsValid(ProjectileMovementComponent)
			? ProjectileMovementComponent->Velocity.GetSafeNormal()
			: GetActorForwardVector();
		const float Dot = FVector::DotProduct(Delta, TravelDir);
		const float InterpSpeed = Dot >= 0.f ? SyncInterpSpeedCatchUp : SyncInterpSpeedSlowDown;

		const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), AuthLocation, DeltaSeconds, InterpSpeed);
		const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), LinkedCounterpart->GetActorRotation(), DeltaSeconds, InterpSpeed);
		SetActorLocationAndRotation(NewLocation, NewRotation);
	}
}

void APRProjectileBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasProjectileAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (UPRRespawnSubsystem* RespawnSubsystem = World->GetSubsystem<UPRRespawnSubsystem>())
			{
				// 일회성 투사체 등록 해제
				RespawnSubsystem->UnregisterDisposableActor(this);
			}
		}
	}

	ClearProjectileHomingScheduleTimers();
	StopClientProjectileHomingPresentation();
	Super::EndPlay(EndPlayReason);

	if (ProjectileRole == EPRProjectileRole::Predicted)
	{
		UPRProjectileManagerComponent* Manager = UPRProjectileManagerComponent::FindForActor(GetWorld()->GetFirstPlayerController());
		if (!Manager)
		{
			return;
		}
		Manager->ClearProjectile(ProjectileId);	
	}
}

void APRProjectileBase::OnAuthLinked(APRProjectileBase* AuthProjectile)
{
	// 이 투사체를 Auth 위치로 보간 시작
	if (!IsValid(AuthProjectile))
	{
		return;
	}
	bShouldSyncToAuth = true;
}

void APRProjectileBase::OnRep_ProjectileId()
{
	// COND_OwnerOnly이므로 소유 클라이언트에서만 도달.
	ProjectileRole = EPRProjectileRole::Auth;
	TryLinkToPredictedOnClient();
	// HasActorBegunPlay()가 false면 BeginPlay에서 처리
}

void APRProjectileBase::ApplyEffectToTarget(AActor* TargetActor)
{
	if (GetLocalRole() != ROLE_Authority || ProjectileRole == EPRProjectileRole::Predicted)
	{
		return;
	}

	FGameplayEffectSpec* EffectSpec = EffectSpecHandle.Data.Get();
	if (EffectSpec == nullptr)
	{
		return;
	}

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor);
	if (ASI)
	{
		UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent();
		if (TargetASC)
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec);
		}
		return;
	}

	// 2026.06.04 이건주_ 주석처리
	// if (IPRDirectDamageReceiverInterface* DirectDamageReceiver = Cast<IPRDirectDamageReceiverInterface>(TargetActor))
	// {
	// 	DirectDamageReceiver->ApplyDirectDamageFromSpec(*EffectSpec, FHitResult());
	// }
}

void APRProjectileBase::ApplyEffectToTargetWithHit(AActor* TargetActor, const FHitResult& InHitResult)
{
	if (GetLocalRole() != ROLE_Authority || ProjectileRole == EPRProjectileRole::Predicted)
	{
		return;
	}
	
	IAbilitySystemInterface* InitialASI = Cast<IAbilitySystemInterface>(TargetActor);
	if (InitialASI)
	{
		UAbilitySystemComponent* TargetASC = InitialASI->GetAbilitySystemComponent();
		FGameplayEffectSpec* EffectSpec = EffectSpecHandle.Data.Get();
		if (TargetASC && EffectSpec)
		{
			EffectSpec->GetContext().AddHitResult(InHitResult,true);
			TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec);	
		}
		return;
	}
	
	// 2026.05.31 이건주_ ASC 미보유 대상 대미지 적용 브릿지
	IPRDestructableInterface* DestructableTarget = Cast<IPRDestructableInterface>(TargetActor);
	if (DestructableTarget)
	{
		// 단순 피해 해석
		const FPRDamageAppliedContext DamageContext = UPRCombatStatics::BuildSimpleDamageAppliedContext(
			InstigatorASC,
			EffectSpecHandle,
			&InHitResult);
		if (DamageContext.FinalDamage <= 0.0f)
		{
			return;
		}

		// 파괴 가능 피해 컨텍스트
		FPRDestructableDamageReceiveContext DestructableDamageContext;
		DestructableDamageContext.Instigator = DamageContext.Instigator;
		DestructableDamageContext.DamageAmount = DamageContext.FinalDamage;
		DestructableDamageContext.HitResult = DamageContext.HitResult;

		DestructableTarget->ReceiveDamageContext(DestructableDamageContext);
		return;
	}

	FGameplayEffectSpec* EffectSpec = EffectSpecHandle.Data.Get();
	if (EffectSpec == nullptr)
	{
		return;
	}

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor);
	if (ASI)
	{
		UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent();
		if (TargetASC)
		{
			EffectSpec->GetContext().AddHitResult(InHitResult, true);
			TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec);
		}
		return;
	}

	if (IPRDirectDamageReceiverInterface* DirectDamageReceiver = Cast<IPRDirectDamageReceiverInterface>(TargetActor))
	{
		DirectDamageReceiver->ApplyDirectDamageFromSpec(*EffectSpec, InHitResult);
	}
}

void APRProjectileBase::LinkCounterpart(APRProjectileBase* InCounterpart)
{
	if (!IsValid(InCounterpart))
	{
		return;
	}

	LinkedCounterpart = InCounterpart;

	// 양방향 링크 보장
	if (InCounterpart->LinkedCounterpart.Get() != this)
	{
		InCounterpart->LinkCounterpart(this);
	}
	bIsLinked = true;
	
	if (GetProjectileRole() == EPRProjectileRole::Auth && LinkedCounterpart.IsValid())
	{
		// 예측 클라측은 감춤
		SetActorHiddenInGame(true);	
	}
}

void APRProjectileBase::TryLinkToPredictedOnClient()
{
	if (ProjectileId == 0 || LinkedCounterpart.IsValid())
	{
		return;
	}
	
	UPRProjectileManagerComponent* Manager = UPRProjectileManagerComponent::FindForActor(GetWorld()->GetFirstPlayerController());
	if (!Manager)
	{
		return;
	}
	
	// Auth 측을 매니저 맵의 Auth 슬롯에 등록
	Manager->NotifyAuthArrived(ProjectileId, this);

	APRProjectileBase* Predicted = Manager->FindPredictedProjectile(ProjectileId);
	if (IsValid(Predicted))
	{
		LinkCounterpart(Predicted);
		Predicted->OnAuthLinked(this);
	}
}

void APRProjectileBase::OnSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 자기 자신이나 발사자에 의한 히트 무시
	if (!IsValid(OtherActor) || OtherActor == this || OtherActor == GetInstigator())
	{
		return;
	}

	if (bIsFinalImpactFalling)
	{
		SettleFinalImpactFall(Hit);
		return;
	}

	if (ShouldIgnoreProjectileHit(OtherActor, OtherComp, Hit))
	{
		if (IsValid(SphereComponent))
		{
			SphereComponent->IgnoreActorWhenMoving(OtherActor, true);
		}
		return;
	}
	
	if (GetProjectileRole() == EPRProjectileRole::Predicted)
	{
		if (!ProjectileMovementComponent->ShouldBounce(Hit))
		{
			// 예측 투사체가 먼저 Hit에 성공한 경우
			if (LinkedCounterpart.IsValid())
			{
				//LinkedCounterpart->SetActorHiddenInGame(false);
				PRProjectileBasePrivate::DrawPredictedProjectileEventDebug(this);
				Destroy();
			}
		}
	}
	else if (GetProjectileRole() == EPRProjectileRole::Auth)
	{
		if (!ProjectileMovementComponent->ShouldBounce(Hit))
		{
			// 예측 투사체가 존재하고, 권위 투사체가 먼저 Hit에 성공한 경우
			if (LinkedCounterpart.IsValid())
			{
				// 가시성 복구
				SetActorHiddenInGame(false);
				SetActorEnableCollision(true);
			
				// 예측 투사체를 즉시 파괴
				LinkedCounterpart->Destroy();
			}
		}
		
		if (HasAuthority())
		{
			if (!HitActors.Contains(OtherActor))
			{
				HandleHit(HitComponent,OtherActor, OtherComp, NormalImpulse, Hit);
			}
			HitActors.Add(OtherActor);
			
			if (!ProjectileMovementComponent->ShouldBounce(Hit))
			{
				HandleFinalImpact(Hit);
			}
		}
		// Replicate된 투사체인 경우
		else
		{
			SetActorHiddenInGame(true);
			SetActorEnableCollision(false);
			ProjectileMovementComponent->SetComponentTickEnabled(false);
		}
		
		
		// TODO: 권위 투사체의 첫 복제 전 바로 파괴되어 버린 경우 Remote의 파괴 이펙트 보장 필요
	}
}

void APRProjectileBase::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	OnSphereHit(OverlappedComponent, OtherActor, OtherComp, FVector::ZeroVector, SweepResult);
}

void APRProjectileBase::HandleHit_Implementation(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	
}

bool APRProjectileBase::ShouldIgnoreProjectileHit(AActor* /*OtherActor*/, UPrimitiveComponent* /*OtherComp*/, const FHitResult& /*Hit*/) const
{
	return false;
}

void APRProjectileBase::DrawDebugs(float DeltaSeconds)
{
#if WITH_EDITORONLY_DATA
	if (HasAuthority() && ProjectileRole == EPRProjectileRole::Auth)
	{
		return;
	}

	if (!bDrawDebugSphere)
	{
		return;
	}

	DebugDrawAccumulator += DeltaSeconds;
	if (DebugDrawInterval > 0.f && DebugDrawAccumulator < DebugDrawInterval)
	{
		return;
	}
	DebugDrawAccumulator = 0.f;

	const FColor& DrawColor = (ProjectileRole == EPRProjectileRole::Predicted) ? DebugColorPredicted : DebugColorAuth;
	DrawDebugSphere(GetWorld(), GetActorLocation(), DebugSphereRadius, 12, DrawColor, false, DebugDrawLifetime);
#endif
}
