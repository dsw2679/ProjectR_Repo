// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (사격 피격 데칼 및 FX 연동)
// Author: 손승우 (페어린 보스 전투 루프 내 포털 소환 및 투사체 발사 제어 구현)
// Author: 이건주 (포털 액터 파괴 인터페이스 및 대미지 연출 연동 구현)
#include "PRBossPortalActor.h"

#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AI/Boss/Faerin/PRFaerinRainProjectileManager.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Projectile/PRBossProjectileActor.h"
#include "ProjectR/Projectile/PRProjectileBase.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/UI/FloatingText/PRFloatingTextManager.h"
#include "ProjectR/UI/FloatingText/PRFloatingTextTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRBossPortal, Log, All);

APRBossPortalActor::APRBossPortalActor()
{
	PatternLifeSpan = 0.0f;

	PortalDamageCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PortalDamageCollision"));
	PortalDamageCollision->SetupAttachment(SceneRoot);
	PortalDamageCollision->SetSphereRadius(120.0f);
	PortalDamageCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PortalDamageCollision->SetCollisionObjectType(ECC_WorldDynamic);
	PortalDamageCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	PortalDamageCollision->SetCollisionResponseToChannel(PRCollisionChannels::ECC_Combat, ECR_Block);
	PortalDamageCollision->SetCollisionResponseToChannel(PRCollisionChannels::ECC_Projectile, ECR_Block);
	PortalDamageCollision->SetGenerateOverlapEvents(false);
}

void APRBossPortalActor::InitializeBossPatternActor(APRBossBaseCharacter* InOwnerBoss, AActor* InPatternTarget)
{
	Super::InitializeBossPatternActor(InOwnerBoss, InPatternTarget);
	SetAndLockPortalTarget(InPatternTarget);
}

void APRBossPortalActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRBossPortalActor, PortalState);
	DOREPLIFETIME(APRBossPortalActor, LockedTarget);
	DOREPLIFETIME(APRBossPortalActor, bPortalActive);
	DOREPLIFETIME(APRBossPortalActor, CurrentPortalHealth);
}

void APRBossPortalActor::RequestPatternActorExpire()
{
	ExpirePortal();
}

void APRBossPortalActor::CancelPatternActor()
{
	ForceExpirePortal();
}

// 2026.06.04 이건주_ 주석처리
// bool APRBossPortalActor::ApplyDirectDamageFromSpec(const FGameplayEffectSpec& DamageSpec, const FHitResult& HitResult)
// {
// 	return ApplyPortalDamage(ResolvePortalDamageAmountFromSpec(DamageSpec), HitResult);
// }

/*~ IPRDestructableInterface Interface ~*/
bool APRBossPortalActor::ReceiveDamageContext(const FPRDestructableDamageReceiveContext& Context)
{
	if (!HasAuthority()
		|| !bCanTakePlayerWeaponDamage
		|| bPortalHealthDepleted
		|| PortalState == EPRBossPortalState::Expired
		|| PortalState == EPRBossPortalState::Expiring)
	{
		return false;
	}

	const float ResolvedDamageAmount = FMath::Max(Context.DamageAmount, 0.0f);
	if (ResolvedDamageAmount <= 0.0f)
	{
		return false;
	}

	const float PreviousHealth = CurrentPortalHealth;
	CurrentPortalHealth = FMath::Clamp(CurrentPortalHealth - ResolvedDamageAmount, 0.0f, MaxPortalHealth);

	const FVector HitLocation = Context.HitResult.bBlockingHit ? FVector(Context.HitResult.ImpactPoint) : GetActorLocation();
	MulticastPortalDamaged(CurrentPortalHealth, PreviousHealth, ResolvedDamageAmount, HitLocation);
	ForceNetUpdate();
	
	// 대미지 후처리
	if (Context.DamageAmount > 0.0f)
	{
		if (APawn* InstigatorPawn = Cast<APawn>(Context.Instigator))
		{
			if (APRPlayerController* PC = Cast<APRPlayerController>(InstigatorPawn->GetController()))
			{
				UPRFloatingTextManager* FloatingTextManager = PC->GetFloatingTextManager();
				
				// 텍스트 타입 결정
				EPRFloatingTextType TextType = EPRFloatingTextType::NormalDamage;
				FPRFloatingTextRequest Request;
				Request.Text = FText::AsNumber(FMath::CeilToInt(Context.DamageAmount));
				Request.TextType = TextType;
				Request.WorldLocation = Context.HitResult.ImpactPoint;

				FloatingTextManager->ClientShowFloatingText_Unreliable(Request);
			}
		}
	}

	if (CurrentPortalHealth <= 0.0f)
	{
		HandlePortalHealthDepleted(Context.HitResult);
	}

	return true;
}

void APRBossPortalActor::SetAndLockPortalTarget(AActor* InTarget)
{
	if (!HasAuthority())
	{
		return;
	}

	LockedTarget = InTarget;

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal target locked. Portal=%s, Target=%s"),
		*GetNameSafe(this),
		*GetNameSafe(LockedTarget.Get()));
}

void APRBossPortalActor::SetPortalProjectileEffectSpec(const FGameplayEffectSpecHandle& InEffectSpec)
{
	ProjectileEffectSpecHandle = InEffectSpec;
}

void APRBossPortalActor::StartPortalTelegraphAfterDelay(float Delay)
{
	if (!HasAuthority())
	{
		return;
	}

	if (PortalState != EPRBossPortalState::None)
	{
		return;
	}

	if (Delay <= 0.0f)
	{
		StartPortalTelegraph();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PortalTelegraphStartTimerHandle);
		World->GetTimerManager().SetTimer(
			PortalTelegraphStartTimerHandle,
			this,
			&APRBossPortalActor::StartPortalTelegraph,
			Delay,
			false);
	}
}

void APRBossPortalActor::StartPortalTelegraph()
{
	if (!HasAuthority())
	{
		return;
	}

	if (PortalState == EPRBossPortalState::Telegraphing
		|| PortalState == EPRBossPortalState::Active
		|| PortalState == EPRBossPortalState::Firing
		|| PortalState == EPRBossPortalState::Paused
		|| PortalState == EPRBossPortalState::Expiring)
	{
		return;
	}

	if (!IsValid(LockedTarget) && IsValid(PatternTarget))
	{
		SetAndLockPortalTarget(PatternTarget);
	}

	ClearPortalLifecycleTimers();
	CurrentProjectileFireCount = 0;
	bPortalActive = false;
	PortalState = EPRBossPortalState::Telegraphing;

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal telegraph started. Portal=%s, ActivationDelay=%.2f"),
		*GetNameSafe(this),
		ActivationDelay);

	MulticastPortalTelegraphStarted();

	if (ActivationDelay <= 0.0f)
	{
		ActivatePortal();
		return;
	}

	GetWorldTimerManager().SetTimer(
		PortalActivationTimerHandle,
		this,
		&APRBossPortalActor::ActivatePortal,
		ActivationDelay,
		false);
}

void APRBossPortalActor::ActivatePortal()
{
	if (!HasAuthority())
	{
		return;
	}

	if (PortalState == EPRBossPortalState::Expiring
		|| PortalState == EPRBossPortalState::Expired
		|| bPortalActive)
	{
		return;
	}

	bPortalActive = true;
	PortalState = EPRBossPortalState::Active;

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal activated. Portal=%s, ActiveDuration=%.2f"),
		*GetNameSafe(this),
		ActiveDuration);

	MulticastPortalActivated();
	ScheduleNextPortalFire();

	if (ActiveDuration <= 0.0f)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		PortalExpireTimerHandle,
		this,
		&APRBossPortalActor::ExpirePortal,
		ActiveDuration,
		false);
}

void APRBossPortalActor::ExpirePortal()
{
	ForceExpirePortal();
}

void APRBossPortalActor::ForceExpirePortal()
{
	if (!HasAuthority())
	{
		return;
	}

	if (PortalState == EPRBossPortalState::Expired
		|| PortalState == EPRBossPortalState::Expiring)
	{
		return;
	}

	ClearPortalLifecycleTimers();

	PortalState = EPRBossPortalState::Expiring;
	bPortalActive = false;

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal expired. Portal=%s, FiredCount=%d"),
		*GetNameSafe(this),
		CurrentProjectileFireCount);

	MulticastPortalExpired();
	PortalState = EPRBossPortalState::Expired;

	if (bDestroyWhenExpired)
	{
		ExpirePatternActor();
	}
}

void APRBossPortalActor::PausePortal()
{
	if (!HasAuthority())
	{
		return;
	}

	if (PortalState != EPRBossPortalState::Active
		&& PortalState != EPRBossPortalState::Firing)
	{
		return;
	}

	ClearPortalFireTimer();
	PortalState = EPRBossPortalState::Paused;

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal paused. Portal=%s"),
		*GetNameSafe(this));

	MulticastPortalPaused();
}

void APRBossPortalActor::UnpausePortal()
{
	if (!HasAuthority() || PortalState != EPRBossPortalState::Paused)
	{
		return;
	}

	PortalState = EPRBossPortalState::Active;

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal unpaused. Portal=%s"),
		*GetNameSafe(this));

	MulticastPortalUnpaused();
	ScheduleNextPortalFire();
}

void APRBossPortalActor::FirePortalProjectile()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValid(LockedTarget))
	{
		UE_LOG(LogPRBossPortal, Verbose,
			TEXT("Portal fire cancelled because target is invalid. Portal=%s"),
			*GetNameSafe(this));
		ForceExpirePortal();
		return;
	}

	if (!CanFirePortalProjectile())
	{
		if (CurrentProjectileFireCount >= MaxProjectilesToFire)
		{
			CompletePortalFireSequence();
		}
		return;
	}

	if (!ProjectileClass->IsChildOf(APRBossProjectileActor::StaticClass()))
	{
		UE_LOG(LogPRBossPortal, Warning,
			TEXT("Portal ProjectileClass should inherit PRBossProjectileActor for default boss damage handling. Portal=%s, ProjectileClass=%s"),
			*GetNameSafe(this),
			*GetNameSafe(ProjectileClass.Get()));
	}

	const FVector LaunchDirection = CalculateProjectileLaunchDirection();
	FTransform ProjectileSpawnTransform;
	if (!BuildProjectileSpawnTransformFromDirection(LaunchDirection, ProjectileSpawnTransform))
	{
		UE_LOG(LogPRBossPortal, Warning,
			TEXT("Portal fire failed because spawn transform could not be built. Portal=%s"),
			*GetNameSafe(this));
		CompletePortalFireSequence();
		return;
	}

	// 경량 rain 투사체 경로: 액터/PMC/복제 없이 매니저에 등록한다. (토글 OFF면 이 분기는 건너뜀)
	if (bUseLightweightRainProjectile && RainProjectileManager.IsValid())
	{
		const float ResolvedSpeed = ProjectileSpeedOverride > 0.0f ? ProjectileSpeedOverride : 3500.0f;
		const FVector LaunchVelocity = LaunchDirection * ResolvedSpeed;
		const FGameplayEffectSpecHandle LightweightEffectSpec = ProjectileEffectSpecHandle.IsValid()
			? ProjectileEffectSpecHandle
			: BuildProjectileEffectSpec();

		PortalState = EPRBossPortalState::Firing;
		MulticastPortalFireStarted();

		RainProjectileManager->ServerEmitRainProjectile(
			ProjectileSpawnTransform.GetLocation(),
			LaunchVelocity,
			LightweightProjectileLifetime,
			LightweightEffectSpec,
			IsValid(OwnerBoss) ? static_cast<AActor*>(OwnerBoss) : static_cast<AActor*>(this));

		++CurrentProjectileFireCount;
		if (CurrentProjectileFireCount >= MaxProjectilesToFire)
		{
			CompletePortalFireSequence();
			return;
		}

		PortalState = EPRBossPortalState::Active;
		ScheduleNextPortalFire();
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const uint32 ProjectileId = NextPortalProjectileId++;
	if (NextPortalProjectileId == 0)
	{
		NextPortalProjectileId = 1;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = IsValid(OwnerBoss) ? static_cast<AActor*>(OwnerBoss) : static_cast<AActor*>(this);
	SpawnParameters.Instigator = IsValid(OwnerBoss) ? OwnerBoss : GetInstigator();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.CustomPreSpawnInitalization = [ProjectileId](AActor* Actor)
	{
		if (APRProjectileBase* Projectile = Cast<APRProjectileBase>(Actor))
		{
			Projectile->InitializeProjectile(EPRProjectileRole::Auth, ProjectileId);
		}
	};

	PortalState = EPRBossPortalState::Firing;
	MulticastPortalFireStarted();

	APRProjectileBase* SpawnedProjectile = World->SpawnActor<APRProjectileBase>(
		ProjectileClass,
		ProjectileSpawnTransform,
		SpawnParameters);

	if (!IsValid(SpawnedProjectile))
	{
		UE_LOG(LogPRBossPortal, Warning,
			TEXT("Portal projectile spawn failed. Portal=%s, ProjectileClass=%s"),
			*GetNameSafe(this),
			*GetNameSafe(ProjectileClass.Get()));
		PortalState = EPRBossPortalState::Active;
		ScheduleNextPortalFire();
		return;
	}

	SpawnedProjectile->SetProjectileInitialVelocity(LaunchDirection, ProjectileSpeedOverride);
	ConfigureSpawnedPortalProjectile(SpawnedProjectile);
	ConfigurePortalProjectileHomingSchedule(SpawnedProjectile);

	const FGameplayEffectSpecHandle EffectSpecHandle = ProjectileEffectSpecHandle.IsValid()
		? ProjectileEffectSpecHandle
		: BuildProjectileEffectSpec();
	if (EffectSpecHandle.IsValid())
	{
		SpawnedProjectile->InitGameplayEffectSpec(EffectSpecHandle);
	}

	SpawnedProjectile->PushRepMovement(EPRRepMovementEvent::Spawn);

	++CurrentProjectileFireCount;
	MulticastPortalProjectileSpawned(SpawnedProjectile);

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal projectile fired. Portal=%s, Projectile=%s, Count=%d/%d"),
		*GetNameSafe(this),
		*GetNameSafe(SpawnedProjectile),
		CurrentProjectileFireCount,
		MaxProjectilesToFire);

	if (CurrentProjectileFireCount >= MaxProjectilesToFire)
	{
		CompletePortalFireSequence();
		return;
	}

	PortalState = EPRBossPortalState::Active;
	ScheduleNextPortalFire();
}

void APRBossPortalActor::ClearPortalFireTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PortalFireTimerHandle);
	}
}

void APRBossPortalActor::SetLightweightRainProjectile(bool bEnable, APRFaerinRainProjectileManager* InManager, float InLifetime)
{
	bUseLightweightRainProjectile = bEnable;
	RainProjectileManager = InManager;
	if (InLifetime > 0.0f)
	{
		LightweightProjectileLifetime = InLifetime;
	}
}

bool APRBossPortalActor::BuildProjectileSpawnTransform(FTransform& OutTransform) const
{
	return BuildProjectileSpawnTransformFromDirection(CalculateProjectileAimDirection(), OutTransform);
}

FVector APRBossPortalActor::CalculateProjectileAimDirection() const
{
	const FVector SpawnLocation = GetActorTransform().TransformPositionNoScale(ProjectileSpawnLocalOffset);
	if (bUseFixedProjectileDirection)
	{
		return GetActorTransform().TransformVectorNoScale(FixedProjectileDirectionLocal).GetSafeNormal();
	}

	const AActor* TargetActor = IsValid(LockedTarget) ? LockedTarget.Get() : PatternTarget.Get();
	if (!IsValid(TargetActor))
	{
		return GetActorForwardVector();
	}

	FVector AimLocation = TargetActor->GetActorLocation();
	const float ResolvedSpeed = ProjectileSpeedOverride > 0.0f ? ProjectileSpeedOverride : 3500.0f;
	const float Distance = FVector::Distance(SpawnLocation, AimLocation);
	if (ResolvedSpeed > 0.0f && ProjectileTargetLead > 0.0f)
	{
		const float TravelTime = Distance / ResolvedSpeed;
		const float LeadScale = ProjectileTargetLead * 0.01f;
		AimLocation += TargetActor->GetVelocity() * TravelTime * LeadScale;
	}

	return (AimLocation - SpawnLocation).GetSafeNormal();
}

FVector APRBossPortalActor::CalculateProjectileLaunchDirection() const
{
	if (bUseFixedProjectileDirection || !bUseTrackingProjectile || !bUseTrackingConeLaunch)
	{
		return CalculateProjectileAimDirection();
	}

	FVector ConeForward = GetActorForwardVector();
	if (ConeForward.IsNearlyZero())
	{
		ConeForward = CalculateProjectileAimDirection();
	}

	if (ConeForward.IsNearlyZero())
	{
		return FVector::ForwardVector;
	}

	const float MinAngleDegrees = FMath::Clamp(TrackingLaunchConeMinAngleDegrees, 0.0f, 90.0f);
	const float MaxAngleDegrees = FMath::Clamp(TrackingLaunchConeMaxAngleDegrees, MinAngleDegrees, 90.0f);
	const float LaunchAngleRadians = FMath::DegreesToRadians(FMath::RandRange(MinAngleDegrees, MaxAngleDegrees));
	const float LaunchAzimuthRadians = FMath::RandRange(0.0f, UE_TWO_PI);

	const FVector Forward = ConeForward.GetSafeNormal();
	FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
	if (Right.IsNearlyZero())
	{
		Right = FVector::RightVector;
	}
	const FVector Up = FVector::CrossProduct(Forward, Right).GetSafeNormal();

	return ((Forward * FMath::Cos(LaunchAngleRadians))
		+ (Right * FMath::Cos(LaunchAzimuthRadians) * FMath::Sin(LaunchAngleRadians))
		+ (Up * FMath::Sin(LaunchAzimuthRadians) * FMath::Sin(LaunchAngleRadians))).GetSafeNormal();
}

bool APRBossPortalActor::BuildProjectileSpawnTransformFromDirection(
	const FVector& LaunchDirection,
	FTransform& OutTransform) const
{
	if (LaunchDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector SpawnLocation = GetActorTransform().TransformPositionNoScale(ProjectileSpawnLocalOffset);
	OutTransform = FTransform((LaunchDirection.Rotation() + ProjectileRotationOffset), SpawnLocation);
	return true;
}

bool APRBossPortalActor::CanFirePortalProjectile() const
{
	if (!HasAuthority()
		|| !bPortalActive
		|| bPortalHealthDepleted
		|| CurrentPortalHealth <= 0.0f
		|| !IsValid(ProjectileClass)
		|| !IsValid(LockedTarget)
		|| MaxProjectilesToFire <= 0
		|| CurrentProjectileFireCount >= MaxProjectilesToFire)
	{
		return false;
	}

	if (PortalState == EPRBossPortalState::Paused && !bCanFireWhilePaused)
	{
		return false;
	}

	return PortalState == EPRBossPortalState::Active
		|| PortalState == EPRBossPortalState::Firing
		|| (PortalState == EPRBossPortalState::Paused && bCanFireWhilePaused);
}

float APRBossPortalActor::GetPortalHealthRatio() const
{
	return MaxPortalHealth > 0.0f ? CurrentPortalHealth / MaxPortalHealth : 0.0f;
}

// // 2026.06.04 이건주_ 주석처리
// bool APRBossPortalActor::ApplyPortalDamage(float DamageAmount, const FHitResult& HitResult)
// {
// 	if (!HasAuthority()
// 		|| !bCanTakePlayerWeaponDamage
// 		|| bPortalHealthDepleted
// 		|| PortalState == EPRBossPortalState::Expired
// 		|| PortalState == EPRBossPortalState::Expiring)
// 	{
// 		return false;
// 	}
//
// 	const float ResolvedDamageAmount = FMath::Max(DamageAmount, 0.0f);
// 	if (ResolvedDamageAmount <= 0.0f)
// 	{
// 		return false;
// 	}
//
// 	const float PreviousHealth = CurrentPortalHealth;
// 	CurrentPortalHealth = FMath::Clamp(CurrentPortalHealth - ResolvedDamageAmount, 0.0f, MaxPortalHealth);
//
// 	const FVector HitLocation = HitResult.bBlockingHit ? FVector(HitResult.ImpactPoint) : GetActorLocation();
// 	MulticastPortalDamaged(CurrentPortalHealth, PreviousHealth, ResolvedDamageAmount, HitLocation);
// 	ForceNetUpdate();
//
// 	if (CurrentPortalHealth <= 0.0f)
// 	{
// 		HandlePortalHealthDepleted(HitResult);
// 	}
//
// 	return true;
// }

void APRBossPortalActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		MaxPortalHealth = FMath::Max(MaxPortalHealth, 1.0f);
		CurrentPortalHealth = MaxPortalHealth;
		bPortalHealthDepleted = false;
	}

	if (HasAuthority() && bAutoStartPortal)
	{
		StartPortalTelegraph();
	}
}

void APRBossPortalActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPortalLifecycleTimers();
	Super::EndPlay(EndPlayReason);
}

void APRBossPortalActor::OnRep_CurrentPortalHealth(float PreviousHealth)
{
	(void)PreviousHealth;
}

void APRBossPortalActor::MulticastPortalTelegraphStarted_Implementation()
{
	// 포털 생성(텔레그래프 시작) 위치에 SFX 재생. 모든 클라이언트에서 실행되며, 데디케이티드 서버에선 사운드가 무시된다.
	if (IsValid(PortalSpawnSoundCue))
	{
		UGameplayStatics::SpawnSoundAtLocation(this, PortalSpawnSoundCue, GetActorLocation());
	}

	BP_OnPortalTelegraphStarted();
}

void APRBossPortalActor::MulticastPortalActivated_Implementation()
{
	BP_OnPortalActivated();
}

void APRBossPortalActor::MulticastPortalExpired_Implementation()
{
	// 포털 만료(사라짐) SFX를 포털 위치에 재생한다. (모든 클라이언트)
	if (IsValid(PortalExpireSoundCue))
	{
		UGameplayStatics::SpawnSoundAtLocation(this, PortalExpireSoundCue, GetActorLocation());
	}

	BP_OnPortalExpired();
}

void APRBossPortalActor::MulticastPortalFireStarted_Implementation()
{
	BP_OnPortalFireStarted();
}

void APRBossPortalActor::MulticastPortalProjectileSpawned_Implementation(APRProjectileBase* SpawnedProjectile)
{
	BP_OnPortalProjectileSpawned(SpawnedProjectile);
}

void APRBossPortalActor::MulticastPortalPaused_Implementation()
{
	BP_OnPortalPaused();
}

void APRBossPortalActor::MulticastPortalUnpaused_Implementation()
{
	BP_OnPortalUnpaused();
}

void APRBossPortalActor::MulticastPortalFireSequenceCompleted_Implementation()
{
	BP_OnPortalFireSequenceCompleted();
}

void APRBossPortalActor::MulticastPortalDamaged_Implementation(
	float NewHealth,
	float PreviousHealth,
	float DamageAmount,
	FVector_NetQuantize HitLocation)
{
	BP_OnPortalHealthChanged(NewHealth, PreviousHealth, MaxPortalHealth, DamageAmount, HitLocation);
}

void APRBossPortalActor::MulticastPortalDestroyedByDamage_Implementation(FVector_NetQuantize HitLocation)
{
	// 포털 파괴(피해 소진) SFX를 피격 위치에 재생한다. (모든 클라이언트)
	if (IsValid(PortalDestroyedSoundCue))
	{
		UGameplayStatics::SpawnSoundAtLocation(this, PortalDestroyedSoundCue, HitLocation);
	}

	BP_OnPortalDestroyedByDamage(HitLocation);
}

void APRBossPortalActor::ScheduleNextPortalFire()
{
	if (!HasAuthority() || !CanFirePortalProjectile())
	{
		return;
	}

	ClearPortalFireTimer();

	const float FireDelay = CurrentProjectileFireCount == 0 ? InitialFireDelay : FireInterval;
	if (FireDelay <= 0.0f)
	{
		FirePortalProjectile();
		return;
	}

	GetWorldTimerManager().SetTimer(
		PortalFireTimerHandle,
		this,
		&APRBossPortalActor::FirePortalProjectile,
		FireDelay,
		false);
}

void APRBossPortalActor::CompletePortalFireSequence()
{
	if (!HasAuthority())
	{
		return;
	}

	ClearPortalFireTimer();
	MulticastPortalFireSequenceCompleted();

	UE_LOG(LogPRBossPortal, Verbose,
		TEXT("Portal fire sequence completed. Portal=%s, FiredCount=%d"),
		*GetNameSafe(this),
		CurrentProjectileFireCount);

	if (bDestroyAfterFireComplete)
	{
		ForceExpirePortal();
		return;
	}

	if (PortalState != EPRBossPortalState::Expired && PortalState != EPRBossPortalState::Expiring)
	{
		PortalState = EPRBossPortalState::Active;
	}
}

void APRBossPortalActor::ClearPortalLifecycleTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PortalTelegraphStartTimerHandle);
		World->GetTimerManager().ClearTimer(PortalActivationTimerHandle);
		World->GetTimerManager().ClearTimer(PortalExpireTimerHandle);
		World->GetTimerManager().ClearTimer(PortalFireTimerHandle);
	}
}

void APRBossPortalActor::ConfigureSpawnedPortalProjectile(APRProjectileBase* SpawnedProjectile)
{
	if (!IsValid(SpawnedProjectile))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APRBossPortalActor> PortalIt(World); PortalIt; ++PortalIt)
		{
			SpawnedProjectile->AddProjectileIgnoredActor(*PortalIt);
		}
	}

	if (bIgnoreEnemyActorsForProjectile)
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<APREnemyBaseCharacter> EnemyIt(World); EnemyIt; ++EnemyIt)
			{
				SpawnedProjectile->AddProjectileIgnoredActor(*EnemyIt);
			}
		}
	}

}

void APRBossPortalActor::ConfigurePortalProjectileHomingSchedule(APRProjectileBase* SpawnedProjectile)
{
	if (!bUseTrackingProjectile || !IsValid(SpawnedProjectile) || !IsValid(LockedTarget))
	{
		return;
	}

	AActor* HomingTargetActor = LockedTarget.Get();
	if (!IsValid(HomingTargetActor) || !IsValid(HomingTargetActor->GetRootComponent()))
	{
		return;
	}

	const float ResolvedHomingAcceleration = FMath::Max(ProjectileHomingAcceleration, 0.0f);
	SpawnedProjectile->ConfigureProjectileHomingSchedule(
		HomingTargetActor,
		ResolvedHomingAcceleration,
		ProjectileHomingStartDelay,
		ProjectileHomingDuration);
}

FGameplayEffectSpecHandle APRBossPortalActor::BuildProjectileEffectSpec() const
{
	if (!IsValid(OwnerBoss))
	{
		UE_LOG(LogPRBossPortal, Warning,
			TEXT("Portal projectile damage spec build failed because OwnerBoss is invalid. Portal=%s"),
			*GetNameSafe(this));
		return FGameplayEffectSpecHandle();
	}

	UPRAbilitySystemComponent* SourceASC = OwnerBoss->GetEnemyAbilitySystemComponent();
	if (!IsValid(SourceASC))
	{
		UE_LOG(LogPRBossPortal, Warning,
			TEXT("Portal projectile damage spec build failed because SourceASC is invalid. Portal=%s, OwnerBoss=%s"),
			*GetNameSafe(this),
			*GetNameSafe(OwnerBoss.Get()));
		return FGameplayEffectSpecHandle();
	}

	TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = ProjectileDamageEffectClass;
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
		UE_LOG(LogPRBossPortal, Warning,
			TEXT("Portal projectile damage spec build failed because DamageEffectClass is invalid. Portal=%s"),
			*GetNameSafe(this));
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(const_cast<APRBossPortalActor*>(this));

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
		ResolvedDamageEffectClass,
		1.0f,
		EffectContext);

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_AttackMultiplier,
			ProjectileDamageMultiplier);

		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_PoiseDamage,
			ProjectilePoiseDamage);
	}

	return SpecHandle;
}

float APRBossPortalActor::ResolvePortalDamageAmountFromSpec(const FGameplayEffectSpec& DamageSpec) const
{
	float DamageAmount = DamageSpec.GetSetByCallerMagnitude(
		PRCombatGameplayTags::SetByCaller_CurrentWeapon_BaseDamage,
		false,
		0.0f);

	if (DamageAmount <= 0.0f)
	{
		DamageAmount = DamageSpec.GetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_Damage,
			false,
			0.0f);
	}

	return FMath::Max(DamageAmount * PlayerWeaponDamageToPortalMultiplier, 0.0f);
}

void APRBossPortalActor::HandlePortalHealthDepleted(const FHitResult& HitResult)
{
	if (bPortalHealthDepleted)
	{
		return;
	}

	bPortalHealthDepleted = true;
	ClearPortalLifecycleTimers();
	bPortalActive = false;

	const FVector HitLocation = HitResult.bBlockingHit ? FVector(HitResult.ImpactPoint) : GetActorLocation();
	MulticastPortalDestroyedByDamage(HitLocation);

	const bool bPreviousDestroyWhenExpired = bDestroyWhenExpired;
	bDestroyWhenExpired = true;
	ForceExpirePortal();
	bDestroyWhenExpired = bPreviousDestroyWhenExpired;
}
