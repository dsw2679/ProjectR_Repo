// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (사망 리스폰 시 지면 투사체 제거 연동)
// Author: 이건주 (지면 충격파 투사체 설계 및 충돌 사운드/이펙트 연동 구현)
#include "PRGroundBoxProjectileBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectR/Character/Enemy/Penitent/PRPenitentCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/System/PRRespawnSubsystem.h"
#include "ProjectR/UI/FloatingText/PRFloatingTextManager.h"
#include "ProjectR/UI/FloatingText/PRFloatingTextTypes.h"

/*~ 초기화 ~*/

APRGroundBoxProjectileBase::APRGroundBoxProjectileBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	SetReplicatingMovement(false);

	BreakableDetectionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BreakableDetectionBox"));
	Root = BreakableDetectionBox;
	SetRootComponent(BreakableDetectionBox);
	BreakableDetectionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	BreakableDetectionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BreakableDetectionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	BreakableDetectionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	BreakableDetectionBox->SetGenerateOverlapEvents(false);
	BreakableDetectionBox->SetNotifyRigidBodyCollision(true);
	BreakableDetectionBox->SetBoxExtent(FVector(40.f, 60.f, 40.f));
	
	TraceStartPoint = CreateDefaultSubobject<USceneComponent>(TEXT("TraceStartPoint"));
	TraceStartPoint->SetupAttachment(Root);
	TraceStartPoint->SetRelativeLocation(FVector(40.0f, 0.0f, 40.0f));
	
	DamageDetectionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageDetectionBox"));
	DamageDetectionBox->SetupAttachment(Root);
	DamageDetectionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageDetectionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageDetectionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DamageDetectionBox->SetGenerateOverlapEvents(true);
	DamageDetectionBox->SetRelativeLocation(FVector(0.0f, 0.0f, -60.0f));

	WallMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMeshComponent"));
	WallMeshComponent->SetupAttachment(Root);
	WallMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WallMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	// 피격 판정 (히트스캔, 플레이어 프로젝타일 블락, Enemy 프로젝타일 무시
	WallMeshComponent->SetCollisionResponseToChannel(PRCollisionChannels::ECC_Combat, ECR_Block);
	WallMeshComponent->SetCollisionResponseToChannel(PRCollisionChannels::ECC_Projectile, ECR_Block);
	WallMeshComponent->SetCollisionResponseToChannel(PRCollisionChannels::ECC_EnemyProjectile, ECR_Ignore);
	// 범위 대미지 차폐 판정
	WallMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	// AI 시야 무시
	WallMeshComponent->SetCollisionResponseToChannel(PRCollisionChannels::ECC_AISight, ECR_Ignore);
	WallMeshComponent->SetRelativeLocation(FVector(40.0f, 0.0f, -160.0f));
	

	AmbientVFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AmbientVFXComponent"));
	AmbientVFXComponent->SetupAttachment(Root);

	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->UpdatedComponent = BreakableDetectionBox;
	MovementComponent->InitialSpeed = 0.0f;
	MovementComponent->MaxSpeed = 0.0f;
	MovementComponent->ProjectileGravityScale = 0.0f;
	MovementComponent->bAutoActivate = false;
}

/*~ AActor Interface ~*/

void APRGroundBoxProjectileBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	constexpr bool bUsePushModel = true;
	FDoRepLifetimeParams RepMovementParams{COND_None, REPNOTIFY_Always, bUsePushModel};
	DOREPLIFETIME_WITH_PARAMS_FAST(APRGroundBoxProjectileBase, ProjectileRepMovement, RepMovementParams);

	FDoRepLifetimeParams GroundSnapParams{COND_None, REPNOTIFY_OnChanged, bUsePushModel};
	DOREPLIFETIME_WITH_PARAMS_FAST(APRGroundBoxProjectileBase, bUseGroundSnap, GroundSnapParams);
}

void APRGroundBoxProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (UPRRespawnSubsystem* RespawnSubsystem = World->GetSubsystem<UPRRespawnSubsystem>())
			{
				// 일회성 지면 투사체 등록
				RespawnSubsystem->RegisterDisposableActor(this);
			}
		}
	}

	// BP 기본 체력 초기화
	CurrentHealth = FMath::Max(MaxHealth, 0.0f);

	if (IsValid(DamageDetectionBox))
	{
		DamageDetectionBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnDamageDetectionBoxBeginOverlap);
	}

	if (IsValid(BreakableDetectionBox))
	{
		BreakableDetectionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnBreakableDetectionBoxHit);
	}
	
	if (IsValid(WallMeshComponent))
	{
		CorrectionZLocation = WallMeshComponent->GetRelativeLocation().Z;
	}
}

void APRGroundBoxProjectileBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	InterpLaunchRepMovement(DeltaSeconds);

	if (!bLaunched || bDestroyRequested)
	{
		return;
	}

	// 런치 중 지면 보간 부착
	if (bUseGroundSnap)
	{
		SnapToGround(DeltaSeconds, false);
	}
}

void APRGroundBoxProjectileBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (UPRRespawnSubsystem* RespawnSubsystem = World->GetSubsystem<UPRRespawnSubsystem>())
			{
				// 일회성 지면 투사체 등록 해제
				RespawnSubsystem->UnregisterDisposableActor(this);
			}
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SafetyLifeTimeTimerHandle);
	}

	ResetTargetCooldowns();

	Super::EndPlay(EndPlayReason);
}

/*~ IPRDestructableInterface Interface ~*/

bool APRGroundBoxProjectileBase::ReceiveDamageContext(const FPRDestructableDamageReceiveContext& Context)
{
	if (!HasAuthority() || bDestroyRequested)
	{
		return false;
	}

	if (Context.DamageAmount <= 0.0f)
	{
		return false;
	}

	const float DamageAmount = Context.DamageAmount;
	CurrentHealth = FMath::Max(CurrentHealth - DamageAmount, 0.0f);

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

	if (CurrentHealth <= 0.0f)
	{
		// 체력 고갈에 따른 단일 소멸 경로
		RequestGroundBoxEnd(EPRProjectileDestroyReason::DamageDepleted);
	}

	return true;
}

/*~ 외부 구동 ~*/

void APRGroundBoxProjectileBase::InitGroundBoxProjectile(const FPRGroundBoxLaunchParams& Params)
{
	if (!HasAuthority())
	{
		return;
	}

	SourceActor = Params.SourceActor;
	SourceTeam = UPRCombatStatics::GetActorTeam(SourceActor);
	DamageEffectSpecHandle = Params.DamageEffectSpec.IsValid()
		? Params.DamageEffectSpec
		: BuildDefaultDamageEffectSpec(SourceActor);

	const float InitialHealth = Params.OverrideMaxHealth > 0.0f
		? Params.OverrideMaxHealth
		: MaxHealth;

	// 스폰 입력값이 있으면 BP 기본 체력보다 우선 적용
	MaxHealth = FMath::Max(InitialHealth, 0.0f);
	CurrentHealth = MaxHealth;
	
	bUseGroundSnap = Params.bUseGroundSnap;

	ResetTargetCooldowns();
	bDamageEnabled = true;

	if (IsValid(BreakableDetectionBox))
	{
		// 발사 전 환경 감지 비활성화
		BreakableDetectionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SafetyLifeTimeTimerHandle);
		if (MaxSafetyLifeTime > 0.0f)
		{
			// 명시 종료 누락에 대비한 안전 제거 예약
			World->GetTimerManager().SetTimer(
				SafetyLifeTimeTimerHandle,
				this,
				&ThisClass::HandleSafetyLifeTimeExpired,
				MaxSafetyLifeTime,
				false);
		}
	}

	MulticastHandleSpawned();

	if (!Params.LaunchDirection.IsNearlyZero() && Params.LaunchSpeed > 0.0f)
	{
		LaunchGroundBoxProjectile(Params.LaunchDirection, Params.LaunchSpeed);
	}
}

void APRGroundBoxProjectileBase::InitializeAttachedBarrier(APRPenitentCharacter* OwnerPenitent)
{
	if (!HasAuthority() || !IsValid(OwnerPenitent) || bDestroyRequested)
	{
		return;
	}

	FPRGroundBoxLaunchParams Params;
	Params.SourceActor = OwnerPenitent;
	InitializeAttachedGroundBox(Params);
}

void APRGroundBoxProjectileBase::InitializeAttachedGroundBox(const FPRGroundBoxLaunchParams& Params)
{
	if (!HasAuthority() || bDestroyRequested)
	{
		return;
	}

	InitGroundBoxProjectile(Params);

	bLaunched = false;
	bLaunchRepInterpActive = false;
	SetActorTickEnabled(false);

	if (IsValid(MovementComponent))
	{
		// 부착 상태 이동 정지
		MovementComponent->StopMovementImmediately();
		MovementComponent->Deactivate();
	}
}

void APRGroundBoxProjectileBase::LaunchGroundBoxProjectile(const FVector& LaunchDirection, float LaunchSpeed)
{
	if (bDestroyRequested)
	{
		return;
	}

	const FVector SafeDirection = LaunchDirection.GetSafeNormal();
	if (SafeDirection.IsNearlyZero() || LaunchSpeed <= 0.0f)
	{
		return;
	}

	ApplyLaunchMovement(SafeDirection, LaunchSpeed);
	
	if (HasAuthority())
	{
		ApplyLaunchAuthorityState();
	}
}

void APRGroundBoxProjectileBase::ResetTargetCooldowns()
{
	TargetASCNextAllowedTimes.Reset();
	TargetActorNextAllowedTimes.Reset();
}

void APRGroundBoxProjectileBase::RequestGroundBoxEnd(EPRProjectileDestroyReason DestroyReason)
{
	if (!HasAuthority() || bDestroyRequested)
	{
		return;
	}

	DestroyGroundBox(DestroyReason);
}

void APRGroundBoxProjectileBase::DestroyEffcectStarted(EPRProjectileDestroyReason DestroyReason)
{
	if (PRShouldPlayProjectileDestroyEffect(DestroyReason) && IsValid(ExplodeSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation(), GetActorRotation(), 1.f, 1.f, 0.f);
	}
}

void APRGroundBoxProjectileBase::HandleSourceOwnerDead()
{
	if (!HasAuthority() || bDestroyRequested)
	{
		return;
	}

	// 소유자 사망 후 잔존 연출만 허용
	bDamageEnabled = false;
	bLaunched = false;
	bLaunchRepInterpActive = false;
	SetActorTickEnabled(false);
	ResetTargetCooldowns();

	if (IsValid(MovementComponent))
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->Deactivate();
	}

	MulticastHandleFadeRequested();
}

void APRGroundBoxProjectileBase::HandleSafetyLifeTimeExpired()
{
	if (!HasAuthority() || bDestroyRequested)
	{
		return;
	}

	// 명시 종료 누락에 따른 안전 제거
	DestroyGroundBox(EPRProjectileDestroyReason::LifeTimeExpired);
}

/*~ 오버랩 처리 ~*/

void APRGroundBoxProjectileBase::
OnDamageDetectionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bDamageEnabled || bDestroyRequested)
	{
		return;
	}

	UAbilitySystemComponent* TargetAbilitySystemComponent =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);

	if (!CanApplyDamageToTarget(OtherActor, TargetAbilitySystemComponent))
	{
		return;
	}

	if (IsTargetOnCooldown(OtherActor, TargetAbilitySystemComponent))
	{
		return;
	}

	ApplyDamageToTarget(OtherActor, TargetAbilitySystemComponent, SweepResult);
}

void APRGroundBoxProjectileBase::OnBreakableDetectionBoxHit(UPrimitiveComponent* HitComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 공통 필터
	if (!IsValid(OtherActor) || OtherActor == this || OtherActor == GetInstigator() || bDestroyRequested)
	{
		return;
	}

	// 플레이어/AI 같은 Pawn 제외
	if (Cast<APawn>(OtherActor))
	{
		return;
	}

	// 발사 상태 검사
	if (!bLaunched)
	{
		return;
	}

	// 벽 충돌 검사
	if (!IsBlockingWallHit(Hit))
	{
		return;
	}

	// 서버 파괴 처리
	if (HasAuthority())
	{
		DestroyGroundBox(EPRProjectileDestroyReason::Impact);
		return;
	}

	// 클라이언트 즉시 숨김
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);

	if (IsValid(MovementComponent))
	{
		// 클라이언트 이동 정지
		MovementComponent->StopMovementImmediately();
		MovementComponent->Deactivate();
	}
}

/*~ 피해 처리 ~*/

bool APRGroundBoxProjectileBase::CanApplyDamageToTarget(AActor* TargetActor, UAbilitySystemComponent* TargetAbilitySystemComponent) const
{
	if (!IsValid(TargetActor) || TargetActor == this || TargetActor == SourceActor)
	{
		return false;
	}

	if (!IsValid(TargetAbilitySystemComponent))
	{
		return false;
	}

	const EPRTeam TargetTeam = UPRCombatStatics::GetActorTeam(TargetActor);
	if (SourceTeam != EPRTeam::Neutral && SourceTeam == TargetTeam)
	{
		return false;
	}

	const IPRCombatInterface* CombatTarget = Cast<IPRCombatInterface>(TargetActor);
	if (CombatTarget != nullptr && CombatTarget->IsDead())
	{
		return false;
	}

	return true;
}

void APRGroundBoxProjectileBase::ApplyDamageToTarget(AActor* TargetActor,
	UAbilitySystemComponent* TargetASC, const FHitResult& HitResult)
{
	if (!IsValid(TargetActor) || !IsValid(TargetASC) || !DamageEffectSpecHandle.IsValid())
	{
		return;
	}

	FGameplayEffectSpec* EffectSpec = DamageEffectSpecHandle.Data.Get();
	if (EffectSpec == nullptr)
	{
		return;
	}

	// 오버랩 지점을 대상 피해 컨텍스트로 전달
	EffectSpec->GetContext().AddHitResult(HitResult, true);
	TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec);

	SetTargetCooldown(TargetActor, TargetASC);
	MulticastHandleTargetHit(TargetActor, HitResult);
}

FGameplayEffectSpecHandle APRGroundBoxProjectileBase::BuildDefaultDamageEffectSpec(AActor* InSourceActor) const
{
	if (!IsValid(InSourceActor))
	{
		return FGameplayEffectSpecHandle();
	}

	UAbilitySystemComponent* SourceAbilitySystemComponent =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InSourceActor);
	if (!IsValid(SourceAbilitySystemComponent))
	{
		return FGameplayEffectSpecHandle();
	}

	if (!IsValid(DamageEffectClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GroundBoxProjectile] DamageEffectClass 설정 없음. Actor=%s"), *GetNameSafe(this));
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectContextHandle EffectContext = SourceAbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(InSourceActor);

	const FGameplayEffectSpecHandle SpecHandle = SourceAbilitySystemComponent->MakeOutgoingSpec(
		DamageEffectClass,
		1.0f,
		EffectContext);

	if (SpecHandle.IsValid())
	{
		// 적 피해 GE SetByCaller
		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_AttackMultiplier,
			FMath::Max(DamageMultiplier, 0.0f));

		SpecHandle.Data->SetSetByCallerMagnitude(
			PRCombatGameplayTags::SetByCaller_PoiseDamage,
			FMath::Max(PoiseDamage, 0.0f));
	}

	return SpecHandle;
}

bool APRGroundBoxProjectileBase::IsTargetOnCooldown(AActor* TargetActor,
	UAbilitySystemComponent* TargetAbilitySystemComponent) const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (IsValid(TargetAbilitySystemComponent))
	{
		const TWeakObjectPtr<UAbilitySystemComponent> TargetKey(TargetAbilitySystemComponent);
		if (const float* NextAllowedTime = TargetASCNextAllowedTimes.Find(TargetKey))
		{
			return *NextAllowedTime > CurrentTime;
		}
	}

	if (IsValid(TargetActor))
	{
		const TWeakObjectPtr<AActor> TargetKey(TargetActor);
		if (const float* NextAllowedTime = TargetActorNextAllowedTimes.Find(TargetKey))
		{
			return *NextAllowedTime > CurrentTime;
		}
	}

	return false;
}

void APRGroundBoxProjectileBase::SetTargetCooldown(AActor* TargetActor, UAbilitySystemComponent* TargetAbilitySystemComponent)
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float NextAllowedTime = World->GetTimeSeconds() + FMath::Max(TargetCooldownTimeSeconds, 0.0f);
	if (IsValid(TargetAbilitySystemComponent))
	{
		TargetASCNextAllowedTimes.Add(TargetAbilitySystemComponent, NextAllowedTime);
		return;
	}

	if (IsValid(TargetActor))
	{
		TargetActorNextAllowedTimes.Add(TargetActor, NextAllowedTime);
	}
}

/*~ 이동 처리 ~*/

void APRGroundBoxProjectileBase::SnapToGround(float DeltaSeconds, bool bInstant)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || GroundSnapTraceDistance <= 0.0f)
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRGroundBoxSnapToGround), false, this);
	if (IsValid(SourceActor))
	{
		QueryParams.AddIgnoredActor(SourceActor);
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector TraceStart = TraceStartPoint->GetComponentLocation();
	const FVector TraceEnd = TraceStart - FVector::UpVector * GroundSnapTraceDistance;

	FHitResult GroundHit;
	if (!World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, PRCollisionChannels::ECC_Ground, QueryParams))
	{
		return;
	}

	if (GroundHit.ImpactNormal.Z < GroundNormalThreshold)
	{
		return;
	}

	const float TargetZ = GroundHit.ImpactPoint.Z;
	float NewZ = TargetZ - CorrectionZLocation;
	// if (!bInstant && GroundSnapInterpSpeed > 0.0f)
	// {
	// 	// 지면 높이 보간 추적
	// 	NewZ = FMath::FInterpTo(CurrentLocation.Z, TargetZ - CorrectionZLocation, DeltaSeconds, GroundSnapInterpSpeed);
	// 	if (FMath::Abs(NewZ - TargetZ) <= GroundSnapTolerance)
	// 	{
	// 		// 미세 떨림 방지
	// 		NewZ = TargetZ;
	// 	}
	// }

	// 수평 이동은 유지하고 높이만 지면에 보정
	const FVector SnappedLocation(CurrentLocation.X, CurrentLocation.Y, NewZ);
	SetActorLocation(SnappedLocation, false);
}

bool APRGroundBoxProjectileBase::IsBlockingWallHit(const FHitResult& HitResult) const
{
	AActor* HitActor = HitResult.GetActor();
	if (!IsValid(HitActor) || HitActor == this || HitActor == SourceActor)
	{
		return false;
	}

	if (HitResult.ImpactNormal.IsNearlyZero())
	{
		// 표면 방향 없는 환경 접촉 제외
		return false;
	}

	if (HitResult.ImpactNormal.Z >= GroundNormalThreshold)
	{
		// 바닥 계열 표면 제외
		return false;
	}

	return true;
}

void APRGroundBoxProjectileBase::OnRep_ProjectileRepMovement()
{
	switch (ProjectileRepMovement.Event)
	{
	case EPRRepMovementEvent::Launch:
		ApplyLaunchRepMovement(ProjectileRepMovement);
		break;
	default:
		break;
	}
}

void APRGroundBoxProjectileBase::PushProjectileRepMovement(EPRRepMovementEvent Event)
{
	if (!HasAuthority())
	{
		return;
	}

	ProjectileRepMovement.Event = Event;
	ProjectileRepMovement.Location = GetActorLocation();
	ProjectileRepMovement.Rotation = GetActorRotation();
	ProjectileRepMovement.Velocity = IsValid(MovementComponent)
		? MovementComponent->Velocity
		: FVector::ZeroVector;

	MARK_PROPERTY_DIRTY_FROM_NAME(APRGroundBoxProjectileBase, ProjectileRepMovement, this);
	ForceNetUpdate();
}

void APRGroundBoxProjectileBase::ApplyLaunchMovement(const FVector& LaunchDirection, float LaunchSpeed)
{
	const FVector SafeDirection = LaunchDirection.GetSafeNormal();
	if (SafeDirection.IsNearlyZero() || LaunchSpeed <= 0.0f)
	{
		return;
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	bLaunched = true;
	bDamageEnabled = true;
	SetActorTickEnabled(true);
	SetActorRotation(SafeDirection.Rotation());

	if (IsValid(BreakableDetectionBox))
	{
		// 발사 중 환경 감지 활성화
		BreakableDetectionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	if (IsValid(MovementComponent))
	{
		// 런치 이동 상태
		MovementComponent->InitialSpeed = LaunchSpeed;
		MovementComponent->MaxSpeed = FMath::Max(MovementComponent->MaxSpeed, LaunchSpeed);
		MovementComponent->Velocity = SafeDirection * LaunchSpeed;
		MovementComponent->Activate(true);
	}

	// 지면 높이 보정
	if (bUseGroundSnap)
	{
		SnapToGround(0.0f, true);
	}

	OnGroundBoxLaunched.Broadcast(this);
}

void APRGroundBoxProjectileBase::ApplyLaunchAuthorityState()
{
	ResetTargetCooldowns();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SafetyLifeTimeTimerHandle);
		if (MaxSafetyLifeTime > 0.0f)
		{
			// 런치 수명 예약
			World->GetTimerManager().SetTimer(
				SafetyLifeTimeTimerHandle,
				this,
				&ThisClass::HandleSafetyLifeTimeExpired,
				LaunchLifeTime,
				false);
		}
	}

	PushProjectileRepMovement(EPRRepMovementEvent::Launch);
}

void APRGroundBoxProjectileBase::ApplyLaunchRepMovement(const FPRProjectileRepMovement& RepMovement)
{
	if (bDestroyRequested || RepMovement.Event != EPRRepMovementEvent::Launch || RepMovement.Velocity.IsNearlyZero())
	{
		return;
	}

	const float LaunchSpeed = RepMovement.Velocity.Size();
	const FVector LaunchDirection = RepMovement.Velocity.GetSafeNormal();
	if (LaunchDirection.IsNearlyZero() || LaunchSpeed <= 0.0f)
	{
		return;
	}

	const float LocationDelta = FVector::Dist(GetActorLocation(), RepMovement.Location);
	if (LocationDelta > LaunchRepSnapDistance)
	{
		SetActorLocationAndRotation(RepMovement.Location, RepMovement.Rotation, false, nullptr, ETeleportType::TeleportPhysics);
		bLaunchRepInterpActive = false;
	}
	else
	{
		PendingLaunchRepMovement = RepMovement;
		LaunchRepInterpStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		bLaunchRepInterpActive = LaunchRepInterpSpeed > 0.0f && LaunchRepInterpMaxTime > 0.0f;
	}

	ApplyLaunchMovement(LaunchDirection, LaunchSpeed);
}

void APRGroundBoxProjectileBase::InterpLaunchRepMovement(float DeltaSeconds)
{
	if (!bLaunchRepInterpActive || bDestroyRequested)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		bLaunchRepInterpActive = false;
		return;
	}

	const float ElapsedSeconds = World->GetTimeSeconds() - LaunchRepInterpStartTime;
	if (ElapsedSeconds >= LaunchRepInterpMaxTime)
	{
		bLaunchRepInterpActive = false;
		return;
	}

	const FVector TargetLocation = PendingLaunchRepMovement.Location + PendingLaunchRepMovement.Velocity * ElapsedSeconds;
	const FRotator TargetRotation = PendingLaunchRepMovement.Rotation;
	const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaSeconds, LaunchRepInterpSpeed);
	const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaSeconds, LaunchRepInterpSpeed);

	SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);

	if (FVector::DistSquared(NewLocation, TargetLocation) <= FMath::Square(GroundSnapTolerance))
	{
		bLaunchRepInterpActive = false;
	}
}

/*~ 소멸 처리 ~*/

void APRGroundBoxProjectileBase::DestroyGroundBox(EPRProjectileDestroyReason DestroyReason)
{
	if (bDestroyRequested)
	{
		return;
	}

	bDestroyRequested = true;
	bDamageEnabled = false;
	bLaunched = false;
	bLaunchRepInterpActive = false;
	SetActorTickEnabled(false);
	ResetTargetCooldowns();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SafetyLifeTimeTimerHandle);
	}

	if (IsValid(MovementComponent))
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->Deactivate();
	}

	MulticastHandleDestroyed(DestroyReason);
	Destroy();
}

/*~ 네트워크 연출 ~*/

void APRGroundBoxProjectileBase::MulticastHandleSpawned_Implementation()
{
	OnGroundBoxSpawned.Broadcast(this);
}

void APRGroundBoxProjectileBase::MulticastHandleTargetHit_Implementation(AActor* TargetActor, const FHitResult& HitResult)
{
	OnGroundBoxTargetHit.Broadcast(this, TargetActor, HitResult);
}

void APRGroundBoxProjectileBase::MulticastHandleGroundBoxDamaged_Implementation(float DamageAmount, float HealthAfterDamage)
{
	OnGroundBoxDamaged.Broadcast(this, DamageAmount, HealthAfterDamage);
}

void APRGroundBoxProjectileBase::MulticastHandleFadeRequested_Implementation()
{
	OnGroundBoxFadeRequested.Broadcast(this);
}

void APRGroundBoxProjectileBase::MulticastHandleDestroyed_Implementation(EPRProjectileDestroyReason DestroyReason)
{
	DestroyEffcectStarted(DestroyReason);
	OnGroundBoxDestroyed.Broadcast(this);
}
