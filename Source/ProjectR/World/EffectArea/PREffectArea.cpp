// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (이펙트 영역 액터 기초 설계 및 리스폰 연동 구현)
// Author: 이건주 (드론 회복 및 Penitent 몬스터용 특수 이펙트 영역 구현)
#include "PREffectArea.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ProjectR/Combat/PRCombatInterface.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "ProjectR/Combat/PRDestructableInterface.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/System/PRRespawnSubsystem.h"

DEFINE_LOG_CATEGORY(LogPREffectArea);

APREffectArea::APREffectArea()
{
	PrimaryActorTick.bCanEverTick = false; // 서버에서만 타이머 실행
	bReplicates = true; // 스폰은 복제

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->SetSphereRadius(200.f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetGenerateOverlapEvents(true);

	ApplyPolicy = EEffectAreaApplyPolicy::Once;
	RemainingTime = 0.f;
}

/*~ APREffectArea Interface ~*/

void APREffectArea::InitEffectArea(const FGameplayEffectSpecHandle& InEffectSpec, UAbilitySystemComponent* InSourceASC,
                                   float InDuration)
{
	if (!HasAuthority())
	{
		return;
	}

	EffectSpecHandle = InEffectSpec;
	SourceASC = InSourceASC;
	RemainingTime = InDuration;

	ValidatePolicyAgainstEffect();

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// 수명 타이머 등록
	if (InDuration > 0.f)
	{
		World->GetTimerManager().SetTimer(LifetimeTimerHandle, this,
			&ThisClass::OnLifetimeExpired, InDuration, false);
	}

	// WhileOverlap 정책일 때 주기적 재적용 타이머 등록
	if (ApplyPolicy == EEffectAreaApplyPolicy::WhileOverlap && ReapplyInterval > 0.f)
	{
		World->GetTimerManager().SetTimer(ReapplyTimerHandle, this,
			&ThisClass::ReapplyEffectToOverlappingActors, ReapplyInterval, true);
	}
}

void APREffectArea::AddGroundSnapComponent(USceneComponent* Component)
{
	if (IsValid(Component))
	{
		GroundSnapComponents.AddUnique(Component);
	}
}

/*~ AActor Interface ~*/

void APREffectArea::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (UPRRespawnSubsystem* RespawnSubsystem = World->GetSubsystem<UPRRespawnSubsystem>())
			{
				// 일회성 이펙트 영역 등록
				RespawnSubsystem->RegisterDisposableActor(this);
			}
		}
	}

	if (IsValid(CollisionComponent))
	{
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnAreaBeginOverlap);
		CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnAreaEndOverlap);
	}

	// 등록된 컴포넌트들을 지표면 높이로 스냅
	SnapComponentsToGround();
}

void APREffectArea::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (UPRRespawnSubsystem* RespawnSubsystem = World->GetSubsystem<UPRRespawnSubsystem>())
			{
				// 일회성 이펙트 영역 등록 해제
				RespawnSubsystem->UnregisterDisposableActor(this);
			}
		}
	}

	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(LifetimeTimerHandle);
			World->GetTimerManager().ClearTimer(ReapplyTimerHandle);
		}

		// WhileOverlap 정책으로 적용된 활성 GE는 영역 종료 시 함께 해제
		RemoveAllTrackedEffects();
	}

	Super::EndPlay(EndPlayReason);
}

/*~ APREffectArea Interface ~*/

void APREffectArea::OnAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValid(OtherActor) || OtherActor == this)
	{
		return;
	}

	if (!EffectSpecHandle.IsValid() || !EffectSpecHandle.Data.IsValid())
	{
		return;
	}

	switch (ApplyPolicy)
	{
	case EEffectAreaApplyPolicy::Once:
		{
			if (OnceAppliedActors.Contains(OtherActor))
			{
				return;
			}

			// 핸들 유효성과 무관하게 적용 시도 자체를 마킹 (Instant GE도 1회 적용 보장)
			ApplyEffectToActor(OtherActor);
			OnceAppliedActors.Add(OtherActor);
			break;
		}
	case EEffectAreaApplyPolicy::WhileOverlap:
		{
			// 진입 즉시 1회 적용. 이후 ReapplyTimerHandle이 주기적으로 재적용
			const FActiveGameplayEffectHandle ActiveHandle = ApplyEffectToActor(OtherActor);
			FPREffectAreaActiveHandles& HandleSet = AppliedEffectHandles.FindOrAdd(OtherActor);
			if (ActiveHandle.IsValid())
			{
				HandleSet.Add(ActiveHandle);
			}
			break;
		}
	case EEffectAreaApplyPolicy::KeepDuration:
		{
			// GE 자체 duration, stacking으로 누적 관리. 영역에서는 추적하지 않음
			ApplyEffectToActor(OtherActor);
			break;
		}
	default:
		break;
	}
}

void APREffectArea::OnAreaEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	if (ApplyPolicy != EEffectAreaApplyPolicy::WhileOverlap)
	{
		return;
	}

	RemoveTrackedEffectsForActor(OtherActor);
}

void APREffectArea::SnapComponentsToGround()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const float TraceDistance = 1000.f;

	for (USceneComponent* Component : GroundSnapComponents)
	{
		if (!IsValid(Component))
		{
			continue;
		}

		const FVector Origin = Component->GetComponentLocation();
		const FVector TraceStart = FVector(Origin.X, Origin.Y, Origin.Z + TraceDistance);
		const FVector TraceEnd = FVector(Origin.X, Origin.Y, Origin.Z - TraceDistance);

		FHitResult HitResult;
		if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd,
			PRCollisionChannels::ECC_Ground, QueryParams))
		{
			Component->SetWorldLocation(HitResult.ImpactPoint);
		}
	}
}

/*~ Internals ~*/

void APREffectArea::OnLifetimeExpired()
{
	RemainingTime = 0.f;
	Destroy();
}

void APREffectArea::ReapplyEffectToOverlappingActors()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValid(CollisionComponent) || !EffectSpecHandle.IsValid() || !EffectSpecHandle.Data.IsValid())
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	CollisionComponent->GetOverlappingActors(OverlappingActors);

	for (AActor* TargetActor : OverlappingActors)
	{
		if (!IsValid(TargetActor) || TargetActor == this)
		{
			continue;
		}

		const FActiveGameplayEffectHandle ActiveHandle = ApplyEffectToActor(TargetActor);
		if (!ActiveHandle.IsValid())
		{
			continue;
		}

		FPREffectAreaActiveHandles& HandleSet = AppliedEffectHandles.FindOrAdd(TargetActor);
		HandleSet.Handles.Add(ActiveHandle);
	}
}

FActiveGameplayEffectHandle APREffectArea::ApplyEffectToActorWithOcclusion(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return FActiveGameplayEffectHandle();
	}

	FHitResult BlockHit;
	if (IsAreaDamageBlocked(TargetActor, BlockHit))
	{
		// 차폐 대상 우선 대미지 전달
		TryApplyEffectToCombatTarget(BlockHit.GetActor(), &BlockHit);
		return FActiveGameplayEffectHandle();
	}

	return ApplyEffectToActor(TargetActor);
}

bool APREffectArea::IsAreaDamageBlocked(AActor* TargetActor, FHitResult& OutBlockHit) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PREffectAreaDamageOcclusion), false, this);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(TargetActor);

	if (SourceASC.IsValid())
	{
		AActor* SourceActor = SourceASC->GetAvatarActor();
		if (IsValid(SourceActor))
		{
			QueryParams.AddIgnoredActor(SourceActor);
		}
	}

	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	// 단일 라인 차폐 판정
	return World->LineTraceSingleByChannel(
		OutBlockHit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams);
}

FActiveGameplayEffectHandle APREffectArea::ApplyEffectToActor(AActor* TargetActor) const
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!IsValid(TargetASC))
	{
		// ASC 미보유 전투 대상 브릿지 경로
		TryApplyEffectToCombatTarget(TargetActor, nullptr);
		return FActiveGameplayEffectHandle();
	}

	// Source가 있으면 Source->Target 경로로, 없으면 (월드 배치 영역 등) Target Self 적용으로 폴백
	if (SourceASC.IsValid())
	{
		return SourceASC->ApplyGameplayEffectSpecToTarget(*EffectSpecHandle.Data.Get(), TargetASC);
	}

	return TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
}

bool APREffectArea::TryApplyEffectToCombatTarget(AActor* TargetActor, const FHitResult* HitResult) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (IsValid(TargetASC))
	{
		return false;
	}

	IPRDestructableInterface* DestructableTarget = Cast<IPRDestructableInterface>(TargetActor);
	if (!DestructableTarget)
	{
		return false;
	}

	// Instigator, DamageAmount만 전달
	FGameplayEffectSpec* EffectSpec = EffectSpecHandle.Data.Get();
	const FGameplayEffectContextHandle& EffectContext = EffectSpec->GetEffectContext();
	
	const float BaseDamage = EffectSpec->GetSetByCallerMagnitude(
	PRCombatGameplayTags::SetByCaller_CurrentWeapon_BaseDamage,
	false,
	0.0f);
	
	// 파괴 가능 대상 전용 컨텍스트
	FPRDestructableDamageReceiveContext DestructableDamageContext;
	DestructableDamageContext.Instigator = EffectContext.GetInstigator();
	DestructableDamageContext.DamageAmount = BaseDamage;

	DestructableTarget->ReceiveDamageContext(DestructableDamageContext);

	// ASC 미보유 대상 대미지 컨텍스트 전달
	return DestructableTarget->ReceiveDamageContext(DestructableDamageContext);
}

void APREffectArea::RemoveAllTrackedEffects()
{
	for (TPair<AActor*, FPREffectAreaActiveHandles>& Pair : AppliedEffectHandles)
	{
		AActor* TargetActor = Pair.Key;
		if (!IsValid(TargetActor))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (!IsValid(TargetASC))
		{
			continue;
		}

		for (const FActiveGameplayEffectHandle& Handle : Pair.Value.Handles)
		{
			if (Handle.IsValid())
			{
				TargetASC->RemoveActiveGameplayEffect(Handle);
			}
		}
	}

	AppliedEffectHandles.Reset();
}

void APREffectArea::RemoveTrackedEffectsForActor(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	FPREffectAreaActiveHandles HandleSet;
	if (!AppliedEffectHandles.RemoveAndCopyValue(TargetActor, HandleSet))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!IsValid(TargetASC))
	{
		return;
	}

	for (const FActiveGameplayEffectHandle& Handle : HandleSet.Handles)
	{
		if (Handle.IsValid())
		{
			TargetASC->RemoveActiveGameplayEffect(Handle);
		}
	}
}

void APREffectArea::ValidatePolicyAgainstEffect() const
{
	if (!EffectSpecHandle.IsValid() || !EffectSpecHandle.Data.IsValid())
	{
		return;
	}

	const UGameplayEffect* GE = EffectSpecHandle.Data->Def;
	if (!IsValid(GE))
	{
		return;
	}

	const EGameplayEffectDurationType DurationType = GE->DurationPolicy;

	switch (ApplyPolicy)
	{
	case EEffectAreaApplyPolicy::Once:
		{
			if (DurationType == EGameplayEffectDurationType::Infinite)
			{
				UE_LOG(LogPREffectArea, Warning,
					TEXT("[%s] ApplyPolicy=Once + GE(%s) DurationPolicy=Infinite. 영역에서 해제하지 않으므로 영구 적용 우려"),
					*GetName(), *GetNameSafe(GE));
			}
			break;
		}
	case EEffectAreaApplyPolicy::WhileOverlap:
		{
			if (GE->StackingType == EGameplayEffectStackingType::None)
			{
				UE_LOG(LogPREffectArea, Warning,
					TEXT("[%s] ApplyPolicy=WhileOverlap + GE(%s) StackingType=None. 주기 재적용 시 활성 GE 누적됨"),
					*GetName(), *GetNameSafe(GE));
			}
			break;
		}
	case EEffectAreaApplyPolicy::KeepDuration:
		{
			if (DurationType != EGameplayEffectDurationType::Infinite)
			{
				UE_LOG(LogPREffectArea, Warning,
					TEXT("[%s] ApplyPolicy=KeepDuration + GE(%s) DurationPolicy != Infinite. 영역 이탈 후 GE가 자연 만료됨"),
					*GetName(), *GetNameSafe(GE));
			}
			break;
		}
	default:
		break;
	}
}
