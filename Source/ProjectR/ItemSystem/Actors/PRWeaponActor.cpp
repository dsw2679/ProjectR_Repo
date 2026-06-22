// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (무기 스켈레탈 메시 조작 애니메이션 연동)
// Author: 배유찬 (무기 사격 이펙트(Tracer), 재장전 애니메이션 및 투사체 발사 제어 구현)
// Author: 손승우 (보스용 특수 타격 판정 무기 연동)
// Author: 이건주 (인벤토리 연동 및 3D 프리뷰 연동 구현)
#include "PRWeaponActor.h"

#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectR/Animation/Weapon/PRWeaponAnimInstance.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

void FPRWeaponFireFXParams::AddTrailEnd(const FVector& InEndLocation)
{
	// 다중 Trail 종료 위치 누적
	TrailEnds.Add(InEndLocation);
}

FVector FPRWeaponFireFXParams::GetPrimaryTrailEnd() const
{
	// 단일 Trail 연출에서 사용할 대표 종료 위치
	return TrailEnds.Num() > 0 ? TrailEnds[0] : FVector::ZeroVector;
}

APRWeaponActor::APRWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMeshComponent"));
	WeaponMeshComponent->SetupAttachment(SceneRoot);
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComponent->SetGenerateOverlapEvents(false);
}

void APRWeaponActor::BeginPlay()
{
	Super::BeginPlay();
}

void APRWeaponActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (IKSuppressedTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(IKSuppressedTimerHandle);
	}

	if (TrailTriggerResetTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(TrailTriggerResetTimerHandle);
		TrailTriggerResetTimerHandle.Invalidate();
	}

	PendingTrailTriggerComponent.Reset();
}

void APRWeaponActor::AttachToOwnerMesh(ACharacter* OwnerCharacter, FName SocketName)
{
	// 캐릭터, 메시, 소켓 이름 중 하나라도 유효하지 않으면 부착을 진행하지 않는다.
	if (!IsValid(OwnerCharacter) || !IsValid(OwnerCharacter->GetMesh()) || SocketName.IsNone())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[WeaponActor] Attach failed. Actor=%s Owner=%s Socket=%s"),
			*GetNameSafe(this),
			*GetNameSafe(OwnerCharacter),
			*SocketName.ToString());
		return;
	}

	if (!OwnerCharacter->GetMesh()->DoesSocketExist(SocketName))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[WeaponActor] Attach failed. Socket missing. Actor=%s Owner=%s Socket=%s"),
			*GetNameSafe(this),
			*GetNameSafe(OwnerCharacter),
			*SocketName.ToString());
		return;
	}

    // 무기 메시 부착
	AttachToComponent(
		OwnerCharacter->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		SocketName);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[WeaponActor] Attached. Actor=%s Owner=%s Socket=%s"),
		*GetNameSafe(this),
		*GetNameSafe(OwnerCharacter),
		*SocketName.ToString());
}

UPRWeaponAnimInstance* APRWeaponActor::GetWeaponAnimInstance() const
{
	if (!IsValid(WeaponMeshComponent))
	{
		return nullptr;
	}

	return Cast<UPRWeaponAnimInstance>(WeaponMeshComponent->GetAnimInstance());
}

bool APRWeaponActor::RequestWeaponAnimation(EPRWeaponAnimationState AnimationState)
{
	UPRWeaponAnimInstance* WeaponAnimInstance = GetWeaponAnimInstance();
	if (!IsValid(WeaponAnimInstance))
	{
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("[WeaponActor] 무기 애니메이션 요청 실패. Actor=%s State=%s"),
			*GetNameSafe(this),
			*UEnum::GetValueAsString(AnimationState));
		return false;
	}

	switch (AnimationState)
	{
	case EPRWeaponAnimationState::Idle:
		WeaponAnimInstance->SetToIdle();
		break;
	case EPRWeaponAnimationState::Shoot:
		WeaponAnimInstance->PlayShoot();
		break;
	case EPRWeaponAnimationState::Reload:
		WeaponAnimInstance->PlayReload();
		break;
	default:
		return false;
	}

	return true;
}

bool APRWeaponActor::RequestShootAnimation()
{
	return RequestWeaponAnimation(EPRWeaponAnimationState::Shoot);
}

bool APRWeaponActor::RequestReloadAnimation()
{
	return RequestWeaponAnimation(EPRWeaponAnimationState::Reload);
}

bool APRWeaponActor::SetWeaponAnimationIdle()
{
	return RequestWeaponAnimation(EPRWeaponAnimationState::Idle);
}

bool APRWeaponActor::ShouldApplyLeftHandIK() const
{
	return bUseLeftHandIK && LeftHandIKSocketName.IsValid() && !bIsIKSuppressed && !IsHidden();
}

void APRWeaponActor::SetIsIKSuppressed(bool bInSuppressed, float InitialDelay)
{
	if (IKSuppressedTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(IKSuppressedTimerHandle);
	}
	
	bPendingIKSuppressed = bInSuppressed;
	
	if (FMath::IsNearlyZero(InitialDelay))
	{
		Internal_SetIsIKSuppressed();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(
			IKSuppressedTimerHandle,
			this,
			&APRWeaponActor::Internal_SetIsIKSuppressed,
			InitialDelay,
			false);
	}
}

void APRWeaponActor::Internal_SetIsIKSuppressed()
{
	bIsIKSuppressed = bPendingIKSuppressed;
}

void APRWeaponActor::TriggerFireFX(const FPRWeaponFireFXParams& Params)
{
	// 발사 FX 단계별 호출
	PlayFireSFX(Params);
	PlayMuzzleFlashVFX(Params);
	
	if (Params.HasTrail())
	{
		PlayTrailVFX(Params);	
	}
}

void APRWeaponActor::PlayFireSFX_Implementation(const FPRWeaponFireFXParams& Params)
{
	if (!IsValid(FireSFX))
	{
		return;
	}

	// 총구 위치 기준 발사음 재생
	const FTransform MuzzleTransform = GetMuzzleTransform();
	UGameplayStatics::PlaySoundAtLocation(this, FireSFX, MuzzleTransform.GetLocation());
}

void APRWeaponActor::PlayMuzzleFlashVFX_Implementation(const FPRWeaponFireFXParams& Params)
{
	if (!IsValid(MuzzleFlashVFX))
	{
		return;
	}

	// 총구 트랜스폼 기준 화염 재생
	const FTransform MuzzleTransform = GetMuzzleTransform();
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this,
		MuzzleFlashVFX,
		MuzzleTransform.GetLocation(),
		MuzzleTransform.Rotator(),
		MuzzleTransform.GetScale3D(),
		true,
		true,
		ENCPoolMethod::AutoRelease);
}

void APRWeaponActor::PlayTrailVFX_Implementation(const FPRWeaponFireFXParams& Params)
{
	// Params 방향 기준 Trail 회전 계산
	const FVector TrailDirection = Params.Direction.IsNearlyZero()
		? FVector::ForwardVector
		: Params.Direction;

	UNiagaraComponent* TrailComponent = GetCachedTrailComponent();
	if (!IsValid(TrailComponent))
	{
		if (!IsValid(TrailVFX))
		{
			return;
		}

		// Niagara 파라미터 주입 전 비활성 상태로 Trail 생성
		TrailComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			TrailVFX,
			Params.StartLocation,
			TrailDirection.Rotation(),
			FVector::OneVector,
			false,
			true,
			ENCPoolMethod::AutoRelease);
	}
	else
	{
		// 캐시 컴포넌트 월드 트랜스폼 갱신
		TrailComponent->SetWorldLocation(Params.StartLocation);
		TrailComponent->SetWorldRotation(TrailDirection.Rotation());
	}

	if (!IsValid(TrailComponent))
	{
		return;
	}

	// Trail 시작점 파라미터 주입
	if (!TrailStartParamName.IsNone())
	{
		TrailComponent->SetVariableVec3(TrailStartParamName, Params.StartLocation);
	}

	// Trail 끝점 파라미터 주입
	if (!TrailEndParamName.IsNone())
	{
		if (TrailEndParamType == EPRTrailEndParamType::ArrayFloat3)
		{
			// Niagara Array Float 3 파라미터 주입
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
				TrailComponent,
				TrailEndParamName,
				Params.TrailEnds);
		}
		else
		{
			// Niagara Vector 파라미터 주입
			TrailComponent->SetVariableVec3(TrailEndParamName, Params.GetPrimaryTrailEnd());
		}
	}

	// Trigger 미사용 재생은 Activate로 시스템 재시작
	if (!TrailComponent->IsActive() || !bUseTrailTriggerParam)
	{
		TrailComponent->Activate(true);
	}

	if (bUseTrailTriggerParam && !TrailTriggerParamName.IsNone())
	{
		if (TrailTriggerResetTimerHandle.IsValid())
		{
			// 이전 Trigger 초기화 예약 제거
			GetWorldTimerManager().ClearTimer(TrailTriggerResetTimerHandle);
			TrailTriggerResetTimerHandle.Invalidate();
		}

		// Trail Spawn 트리거 상승
		PendingTrailTriggerComponent = TrailComponent;
		TrailComponent->SetVariableBool(TrailTriggerParamName, true);

		if (TrailTriggerHoldTime > 0.0f)
		{
			// 유지 시간 이후 Trigger 초기화
			GetWorldTimerManager().SetTimer(
				TrailTriggerResetTimerHandle,
				this,
				&APRWeaponActor::ResetTrailTriggerParam,
				TrailTriggerHoldTime,
				false);
		}
		else
		{
			// 다음 틱 Trigger 초기화
			TrailTriggerResetTimerHandle = GetWorldTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &APRWeaponActor::ResetTrailTriggerParam));
		}
	}
}

UNiagaraComponent* APRWeaponActor::GetCachedTrailComponent_Implementation() const
{
	return nullptr;
}

void APRWeaponActor::ResetTrailTriggerParam()
{
	if (PendingTrailTriggerComponent.IsValid() && !TrailTriggerParamName.IsNone())
	{
		// Trail Spawn 트리거 하강
		PendingTrailTriggerComponent->SetVariableBool(TrailTriggerParamName, false);
	}

	PendingTrailTriggerComponent.Reset();
	TrailTriggerResetTimerHandle.Invalidate();
}

void APRWeaponActor::PlayNiagaraEffect_Implementation(EPRWeaponEffectType EffectType, UNiagaraSystem* InNiagaraSystem)
{
	if (!IsValid(InNiagaraSystem))
	{
		return;
	}
	
	if (EffectType == EPRWeaponEffectType::MuzzleFlash || EffectType == EPRWeaponEffectType::ProjectileLaunch)
	{
		// 레거시 Niagara 직접 재생 경로
		FTransform MuzzleTransform = GetMuzzleTransform();
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this,InNiagaraSystem,
			MuzzleTransform.GetLocation(),
			MuzzleTransform.Rotator(),
			MuzzleTransform.GetScale3D(),
			true,
			true,
			ENCPoolMethod::AutoRelease);
	}
}

FTransform APRWeaponActor::GetMuzzleTransform_Implementation() const
{
	return GetTransform();
}
