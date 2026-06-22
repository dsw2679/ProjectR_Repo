// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (Support Drone Actor 구현)
#include "PRSupportDroneActor.h"

#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatInterface.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/Drone/PRSupportDroneDataAsset.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Projectile/PRProjectileBase.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/System/PREventTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "TimerManager.h"

namespace
{
	const FName PRDroneDissolveRateParameterName(TEXT("DissolveRate"));
	constexpr float PRDroneDissolveTickInterval = 0.016f;
}

APRSupportDroneActor::APRSupportDroneActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bReplicates = true;
	SetReplicateMovement(false);
	SetNetUpdateFrequency(30.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DroneMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneMeshComponent"));
	DroneMeshComponent->SetupAttachment(SceneRoot);
	DroneMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DroneMeshComponent->SetGenerateOverlapEvents(false);

	MissileSpawnComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MissileSpawnComponent"));
	MissileSpawnComponent->SetupAttachment(SceneRoot);
}

void APRSupportDroneActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		StartServerTimers();
		BindAttackTargetEvent();
	}
}

void APRSupportDroneActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 위치 보간은 각 로컬 컴퓨터에서 한다
	if (!IsValid(OwningPlayer) || !IsValid(DroneData) || DeltaSeconds <= 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const FVector FollowLocation = CalculateFollowLocation(World->GetTimeSeconds());
	const float DistanceToFollow = FVector::Dist(GetActorLocation(), FollowLocation);
	const FVector NextLocation = DistanceToFollow > DroneData->TeleportDistance
		? FollowLocation
		: FMath::VInterpTo(GetActorLocation(), FollowLocation, DeltaSeconds, DroneData->FollowInterpSpeed);

	SetActorLocation(NextLocation);
	
	AActor* FacingTarget = IsValid(CurrentTarget) ? CurrentTarget.Get() : OwningPlayer.Get();
	if (IsValid(FacingTarget))
	{
		const FVector FacingDirection = FacingTarget->GetActorLocation() - GetActorLocation();
		if (!FacingDirection.IsNearlyZero())
		{
			SetActorRotation(FMath::RInterpTo(
				GetActorRotation(),
				FacingDirection.GetSafeNormal().Rotation(),
				DeltaSeconds,
				DroneData->FollowInterpSpeed));
		}
	}
}

void APRSupportDroneActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		StopServerTimers();
		UnbindAttackTargetEvent();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DissolveTickTimerHandle);
	}
	DissolveDynamicMaterials.Reset();

	Super::EndPlay(EndPlayReason);
}

void APRSupportDroneActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRSupportDroneActor, OwningPlayer);
	DOREPLIFETIME(APRSupportDroneActor, CurrentTarget);
	DOREPLIFETIME(APRSupportDroneActor, DroneData);
}

/*~ 초기화 ~*/

void APRSupportDroneActor::InitializeDrone(APawn* InOwningPlayer, UPRSupportDroneDataAsset* InDroneData)
{
	if (!HasAuthority() || !IsValid(InOwningPlayer))
	{
		return;
	}

	OwningPlayer = InOwningPlayer;
	if (IsValid(InDroneData))
	{
		DroneData = InDroneData;
	}

	SetOwner(InOwningPlayer);
	SetInstigator(InOwningPlayer);

	if (IsValid(DroneData))
	{
		SetActorLocation(CalculateFollowLocation(IsValid(GetWorld()) ? GetWorld()->GetTimeSeconds() : 0.0f));
		if (DroneData->LifeSpan > 0.0f)
		{
			SetLifeSpan(DroneData->LifeSpan);
		}
	}

	StopServerTimers();
	StartServerTimers();
	BindAttackTargetEvent();
	RefreshTarget();
}

void APRSupportDroneActor::SetAssistTarget(AActor* InTarget)
{
	if (!HasAuthority() || !IsValid(DroneData) || !IsValidDroneTarget(InTarget))
	{
		return;
	}

	AssistTarget = InTarget;
	AssistTargetExpireWorldTimeSeconds = IsValid(GetWorld())
		? GetWorld()->GetTimeSeconds() + DroneData->AssistTargetHoldDuration
		: 0.0f;
	CurrentTarget = InTarget;
}

/*~ 디졸브 ~*/

void APRSupportDroneActor::StartDissolve(float Duration, EPRDroneDissolveMode DissolveMode)
{
	if (!HasAuthority())
	{
		return;
	}

	if (DissolveMode == EPRDroneDissolveMode::Disappear)
	{
		// 퇴장 중 전투 중단
		StopServerTimers();
		UnbindAttackTargetEvent();
	}

	Multicast_StartDissolve(Duration, DissolveMode);
	ForceNetUpdate();
}

void APRSupportDroneActor::Multicast_StartDissolve_Implementation(float Duration, EPRDroneDissolveMode DissolveMode)
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(DissolveTickTimerHandle);
	}

	ActiveDissolveMode = DissolveMode;
	DissolveDuration = FMath::Max(Duration, 0.0f);
	DissolveElapsedTime = 0.0f;

	if (DissolveMode == EPRDroneDissolveMode::Appear)
	{
		// 등장 표시
		SetActorHiddenInGame(false);
		DissolveStartValue = 1.0f;
		DissolveEndValue = 0.0f;
	}
	else
	{
		// 사라짐 표시
		DissolveStartValue = 0.0f;
		DissolveEndValue = 1.0f;
	}

	PrepareDissolveMaterialsLocal();
	ApplyDissolveValueLocal(DissolveStartValue);

	if (DissolveDuration <= UE_SMALL_NUMBER)
	{
		// 즉시 완료
		CompleteDissolveLocal();
		return;
	}

	if (IsValid(World))
	{
		World->GetTimerManager().SetTimer(
			DissolveTickTimerHandle,
			this,
			&APRSupportDroneActor::TickDissolveLocal,
			PRDroneDissolveTickInterval,
			true);
	}
	else
	{
		CompleteDissolveLocal();
	}
}

void APRSupportDroneActor::PrepareDissolveMaterialsLocal()
{
	DissolveDynamicMaterials.Reset();

	if (!IsValid(DroneMeshComponent))
	{
		return;
	}

	const int32 MaterialCount = DroneMeshComponent->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		UMaterialInterface* CurrentMaterial = DroneMeshComponent->GetMaterial(MaterialIndex);
		if (!IsValid(CurrentMaterial))
		{
			continue;
		}

		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(CurrentMaterial);
		if (!IsValid(DynamicMaterial))
		{
			DynamicMaterial = DroneMeshComponent->CreateDynamicMaterialInstance(MaterialIndex, CurrentMaterial);
		}

		if (IsValid(DynamicMaterial))
		{
			DissolveDynamicMaterials.Add(DynamicMaterial);
		}
	}
}

void APRSupportDroneActor::TickDissolveLocal()
{
	DissolveElapsedTime += PRDroneDissolveTickInterval;

	const float Duration = FMath::Max(DissolveDuration, UE_SMALL_NUMBER);
	const float Alpha = FMath::Clamp(DissolveElapsedTime / Duration, 0.0f, 1.0f);
	const float DissolveValue = FMath::Lerp(DissolveStartValue, DissolveEndValue, Alpha);

	ApplyDissolveValueLocal(DissolveValue);

	if (Alpha >= 1.0f)
	{
		CompleteDissolveLocal();
	}
}

void APRSupportDroneActor::CompleteDissolveLocal()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DissolveTickTimerHandle);
	}

	ApplyDissolveValueLocal(DissolveEndValue);

	if (ActiveDissolveMode == EPRDroneDissolveMode::Disappear)
	{
		// 사라짐 완료
		SetActorHiddenInGame(true);
		if (HasAuthority())
		{
			Destroy();
		}
	}
}

void APRSupportDroneActor::ApplyDissolveValueLocal(float DissolveValue)
{
	for (UMaterialInstanceDynamic* DynamicMaterial : DissolveDynamicMaterials)
	{
		if (IsValid(DynamicMaterial))
		{
			DynamicMaterial->SetScalarParameterValue(PRDroneDissolveRateParameterName, DissolveValue);
		}
	}
}

/*~ 타이머 ~*/

void APRSupportDroneActor::StartServerTimers()
{
	if (!HasAuthority() || !IsValid(GetWorld()) || !IsValid(DroneData))
	{
		return;
	}

	StopServerTimers();

	GetWorldTimerManager().SetTimer(
		TargetRefreshTimerHandle,
		this,
		&APRSupportDroneActor::RefreshTarget,
		FMath::Max(DroneData->TargetRefreshInterval, 0.05f),
		true);

	GetWorldTimerManager().SetTimer(
		AttackTimerHandle,
		this,
		&APRSupportDroneActor::TryFireMissile,
		FMath::Max(DroneData->AttackInterval, 0.05f),
		true);
}

void APRSupportDroneActor::StopServerTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TargetRefreshTimerHandle);
		World->GetTimerManager().ClearTimer(AttackTimerHandle);
	}
}

/*~ 이벤트 ~*/

void APRSupportDroneActor::BindAttackTargetEvent()
{
	if (!HasAuthority() || AttackTargetEventHandle.IsValid())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (UPREventManagerSubsystem* EventManager = World->GetSubsystem<UPREventManagerSubsystem>())
		{
			AttackTargetEventHandle = EventManager->Listen(
				PRGameplayTags::Event_Player_AttackTarget,
				FPREventMulticast::FDelegate::CreateUObject(this, &ThisClass::HandlePlayerAttackTargetEvent));
		}
	}
}

void APRSupportDroneActor::UnbindAttackTargetEvent()
{
	if (!AttackTargetEventHandle.IsValid())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (UPREventManagerSubsystem* EventManager = World->GetSubsystem<UPREventManagerSubsystem>())
		{
			EventManager->Unlisten(PRGameplayTags::Event_Player_AttackTarget, AttackTargetEventHandle);
		}
	}

	AttackTargetEventHandle.Reset();
}

void APRSupportDroneActor::HandlePlayerAttackTargetEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	if (EventTag != PRGameplayTags::Event_Player_AttackTarget || !IsValid(OwningPlayer))
	{
		return;
	}

	const FPRPlayerAttackTargetPayload* AttackPayload = Payload.GetPtr<FPRPlayerAttackTargetPayload>();
	if (AttackPayload == nullptr || AttackPayload->Attacker.Get() != OwningPlayer.Get())
	{
		return;
	}

	SetAssistTarget(AttackPayload->Target.Get());
}

/*~ 타겟 ~*/

void APRSupportDroneActor::RefreshTarget()
{
	if (!HasAuthority() || !IsValid(DroneData))
	{
		return;
	}

	UWorld* World = GetWorld();
	const float CurrentTimeSeconds = IsValid(World) ? World->GetTimeSeconds() : 0.0f;
	if (AssistTarget.IsValid()
		&& IsValidDroneTarget(AssistTarget.Get())
		&& (DroneData->AssistTargetHoldDuration <= 0.0f || CurrentTimeSeconds <= AssistTargetExpireWorldTimeSeconds))
	{
		CurrentTarget = AssistTarget.Get();
		return;
	}

	AssistTarget.Reset();
	CurrentTarget = FindClosestEnemyTarget();
}

AActor* APRSupportDroneActor::FindClosestEnemyTarget() const
{
	if (!IsValid(OwningPlayer) || !IsValid(DroneData))
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRSupportDroneTargetSearch), false);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwningPlayer.Get());

	World->OverlapMultiByObjectType(
		Overlaps,
		OwningPlayer->GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(DroneData->TargetSearchRadius),
		QueryParams);

	AActor* ClosestTarget = nullptr;
	float ClosestDistanceSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!IsValidDroneTarget(Candidate))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(OwningPlayer->GetActorLocation(), Candidate->GetActorLocation());
		if (DistanceSq < ClosestDistanceSq)
		{
			ClosestTarget = Candidate;
			ClosestDistanceSq = DistanceSq;
		}
	}

	return ClosestTarget;
}

bool APRSupportDroneActor::IsValidDroneTarget(const AActor* TargetActor) const
{
	if (!IsValid(TargetActor) || TargetActor == this || TargetActor == OwningPlayer.Get())
	{
		return false;
	}

	if (UPRCombatStatics::GetActorTeam(TargetActor) != EPRTeam::Enemy)
	{
		return false;
	}

	const IPRCombatInterface* CombatTarget = Cast<IPRCombatInterface>(TargetActor);
	return CombatTarget == nullptr || !CombatTarget->IsDead();
}

bool APRSupportDroneActor::HasLineOfSightToTarget(const AActor* TargetActor) const
{
	if (!IsValid(DroneData) || !DroneData->bRequireLineOfSight || !IsValid(TargetActor))
	{
		return true;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRSupportDroneLineOfSight), false);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwningPlayer.Get());

	const FVector StartLocation = GetMissileSpawnLocation();
	const FVector EndLocation = TargetActor->GetActorLocation();
	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		DroneData->LineOfSightChannel,
		QueryParams);

	return !bHit || HitResult.GetActor() == TargetActor;
}

/*~ 이동 ~*/

FVector APRSupportDroneActor::CalculateFollowLocation(float WorldTimeSeconds) const
{
	if (!IsValid(OwningPlayer) || !IsValid(DroneData))
	{
		return GetActorLocation();
	}

	const FVector BaseLocation = OwningPlayer->GetActorLocation()
		+ OwningPlayer->GetActorRotation().RotateVector(DroneData->FollowOffset);
	const float HoverOffset = DroneData->HoverAmplitude > 0.0f && DroneData->HoverFrequency > 0.0f
		? FMath::Sin(WorldTimeSeconds * DroneData->HoverFrequency * UE_TWO_PI) * DroneData->HoverAmplitude
		: 0.0f;

	return BaseLocation + FVector(0.0f, 0.0f, HoverOffset);
}

/*~ 공격 ~*/

void APRSupportDroneActor::TryFireMissile()
{
	if (!HasAuthority() || !IsValid(DroneData) || !IsValid(DroneData->MissileProjectileClass))
	{
		return;
	}

	if (!IsValidDroneTarget(CurrentTarget.Get()))
	{
		RefreshTarget();
	}

	if (!IsValidDroneTarget(CurrentTarget.Get()))
	{
		return;
	}

	if (DroneData->AttackRange > 0.0f
		&& FVector::DistSquared(GetActorLocation(), CurrentTarget->GetActorLocation()) > FMath::Square(DroneData->AttackRange))
	{
		return;
	}

	if (!HasLineOfSightToTarget(CurrentTarget.Get()))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const FVector SpawnLocation = GetMissileSpawnLocation();
	const FVector AimDirection = (CurrentTarget->GetActorLocation() - SpawnLocation).GetSafeNormal();
	if (AimDirection.IsNearlyZero())
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = OwningPlayer.Get();
	SpawnParameters.Instigator = OwningPlayer.Get();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const uint32 ProjectileId = NextMissileProjectileId++;
	if (NextMissileProjectileId == 0)
	{
		NextMissileProjectileId = 1;
	}

	SpawnParameters.CustomPreSpawnInitalization = [ProjectileId](AActor* SpawnedActor)
	{
		if (APRProjectileBase* Projectile = Cast<APRProjectileBase>(SpawnedActor))
		{
			Projectile->InitializeProjectile(EPRProjectileRole::Auth, ProjectileId);
		}
	};

	APRProjectileBase* SpawnedProjectile = World->SpawnActor<APRProjectileBase>(
		DroneData->MissileProjectileClass,
		SpawnLocation,
		AimDirection.Rotation(),
		SpawnParameters);
	if (!IsValid(SpawnedProjectile))
	{
		return;
	}

	SpawnedProjectile->AddProjectileIgnoredActor(this);
	SpawnedProjectile->AddProjectileIgnoredActor(OwningPlayer.Get());
	SpawnedProjectile->SetProjectileInitialVelocity(AimDirection, DroneData->MissileSpeedOverride);
	SpawnedProjectile->ConfigureProjectileHomingSchedule(
		CurrentTarget.Get(),
		DroneData->MissileHomingAcceleration,
		DroneData->MissileHomingStartDelay,
		DroneData->MissileHomingDuration);

	const FGameplayEffectSpecHandle SpecHandle = BuildMissileEffectSpec();
	if (SpecHandle.IsValid())
	{
		SpawnedProjectile->InitGameplayEffectSpec(SpecHandle);
	}

	SpawnedProjectile->PushRepMovement(EPRRepMovementEvent::Spawn);
}

FVector APRSupportDroneActor::GetMissileSpawnLocation() const
{
	return IsValid(MissileSpawnComponent)
		? MissileSpawnComponent->GetComponentLocation()
		: GetActorLocation();
}

FGameplayEffectSpecHandle APRSupportDroneActor::BuildMissileEffectSpec() const
{
	if (!IsValid(OwningPlayer) || !IsValid(DroneData))
	{
		return FGameplayEffectSpecHandle();
	}

	UAbilitySystemComponent* SourceASC = UPRCombatStatics::FindAbilitySystemComponent(OwningPlayer.Get());
	if (!IsValid(SourceASC))
	{
		return FGameplayEffectSpecHandle();
	}

	const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
	if (!IsValid(Registry) || !IsValid(Registry->DamageGE_FromMod))
	{
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddInstigator(OwningPlayer.Get(), OwningPlayer->GetController());
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
		Registry->DamageGE_FromMod,
		1.0f,
		EffectContext);
	if (!SpecHandle.IsValid())
	{
		return FGameplayEffectSpecHandle();
	}

	SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Damage, DroneData->MissileDamage);
	SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_GroggyDamage, DroneData->MissileGroggyDamage);
	SpecHandle.Data->AddDynamicAssetTag(PRCombatGameplayTags::Ability_Source_Drone);

	return SpecHandle;
}
