// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (페어린 보스 전용 체력 UI 연동)
// Author: 배유찬 (페어린 보스 조우 상호작용 연동)
// Author: 손승우 (페어린 보스 고유 공격 패턴(GodFall/소환/검격) 및 VFX 연출 구현)
#include "PRFaerinCharacter.h"

#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MotionWarpingComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "ProjectR/AI/Components/PRFaerinCombatDirectorComponent.h"
#include "ProjectR/AI/Components/PRFaerinDebugDrawComponent.h"
#include "ProjectR/AI/Components/PRFaerinGodFallComponent.h"
#include "ProjectR/AI/Components/PRFaerinPatternVFXComponent.h"
#include "ProjectR/AI/Components/PRFaerinTeleportVFXComponent.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/Texture.h"

APRFaerinCharacter::APRFaerinCharacter()
{
	CombatDirectorComponent = CreateDefaultSubobject<UPRFaerinCombatDirectorComponent>(TEXT("CombatDirectorComponent"));
	DebugDrawComponent = CreateDefaultSubobject<UPRFaerinDebugDrawComponent>(TEXT("DebugDrawComponent"));
	GodFallComponent = CreateDefaultSubobject<UPRFaerinGodFallComponent>(TEXT("GodFallComponent"));
	PatternVFXComponent = CreateDefaultSubobject<UPRFaerinPatternVFXComponent>(TEXT("PatternVFXComponent"));
	TeleportVFXComponent = CreateDefaultSubobject<UPRFaerinTeleportVFXComponent>(TEXT("TeleportVFXComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

	bIsRespawnable = false;
	bCanEnterGroggyState = false;

	PhaseThresholdRatios.Add(EPRBossPhase::Phase2, 0.87f);
	PhaseThresholdRatios.Add(EPRBossPhase::Phase3, 0.65f);
	PhaseThresholdRatios.Add(EPRBossPhase::Phase4, 0.25f);
}

/*~ AActor Interface ~*/

void APRFaerinCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APRFaerinCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		SetBossEncounterActive(false);
	}
	else if (bBossEncounterEventBroadcasted)
	{
		BroadcastBossEncounterEnd();
		bBossEncounterEventBroadcasted = false;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(NearTeleportDissolveTickTimerHandle);
	}

	if (IsValid(ActiveNearTeleportDissolveNiagara))
	{
		ActiveNearTeleportDissolveNiagara->Deactivate();
		ActiveNearTeleportDissolveNiagara = nullptr;
	}
	NearTeleportDissolveDynamicMaterials.Reset();

	Super::EndPlay(EndPlayReason);
}

void APRFaerinCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRFaerinCharacter, bBossEncounterActive);
}

// ===== 공개 함수 =====

// 플레이어 전투 교전 상태 진입에 따른 보스 HUD 조우 시작 요청
void APRFaerinCharacter::RequestBossEncounterBegin()
{
	SetBossEncounterActive(true);
}

void APRFaerinCharacter::CustomizeEnemyTargetingConfig(FPREnemyTargetingConfig& InOutTargetingConfig) const
{
	InOutTargetingConfig.bNeverForgetPlayerCandidates = true;
	InOutTargetingConfig.bKeepCurrentTargetDuringPhaseTransition = true;
	InOutTargetingConfig.bRestoreCurrentTargetFromCandidates = true;
	InOutTargetingConfig.bIgnoreEngagementRetainRadiusForPlayerCandidates = true;
}

void APRFaerinCharacter::SeedEncounterTargets(
	const TArray<APRPlayerCharacter*>& Participants,
	APRPlayerCharacter* PrimaryTarget,
	const float ThreatAmount)
{
	if (!HasAuthority() || !IsValid(ThreatComponent))
	{
		return;
	}

	APRPlayerCharacter* ResolvedPrimaryTarget = IsValid(PrimaryTarget) ? PrimaryTarget : nullptr;
	const float ResolvedThreatAmount = FMath::Max(ThreatAmount, 0.0f);
	for (APRPlayerCharacter* Participant : Participants)
	{
		if (!IsValid(Participant))
		{
			continue;
		}

		if (ResolvedThreatAmount > UE_SMALL_NUMBER)
		{
			ThreatComponent->AddThreat(Participant, ResolvedThreatAmount);
		}

		if (!IsValid(ResolvedPrimaryTarget))
		{
			ResolvedPrimaryTarget = Participant;
		}
	}

	if (IsValid(PrimaryTarget) && !Participants.Contains(PrimaryTarget) && ResolvedThreatAmount > UE_SMALL_NUMBER)
	{
		ThreatComponent->AddThreat(PrimaryTarget, ResolvedThreatAmount);
	}

	if (IsValid(ResolvedPrimaryTarget))
	{
		ThreatComponent->ForceCurrentTarget(ResolvedPrimaryTarget);
	}
}

void APRFaerinCharacter::Multicast_SpawnNearTeleportBodyNiagara_Implementation(
	UNiagaraSystem* NiagaraSystem,
	FName AttachSocketName,
	const float LifeSeconds)
{
	SpawnNearTeleportBodyNiagaraLocal(NiagaraSystem, AttachSocketName, LifeSeconds);
}

void APRFaerinCharacter::Multicast_PlayFaerinSoundAtLocation_Implementation(USoundBase* Sound, FVector_NetQuantize Location)
{
	// 모든 클라이언트에서 지정 위치에 SFX를 재생한다. (데디케이티드 서버에선 무시)
	if (IsValid(Sound))
	{
		UGameplayStatics::SpawnSoundAtLocation(this, Sound, Location);
	}
}

void APRFaerinCharacter::Multicast_PlayNearTeleportDissolveVisual_Implementation(
	UNiagaraSystem* DissolveNiagaraSystem,
	FName AttachSocketName,
	const float DissolveDuration,
	const float DissolveStartValue,
	const float DissolveEndValue,
	FName DissolveScalarParameterName,
	FName NiagaraDissolveParameterName,
	UTexture* DissolveTexture,
	FVector2D DissolveTextureUV,
	const float DissolveTickInterval)
{
	StartNearTeleportDissolveVisual(
		DissolveNiagaraSystem,
		AttachSocketName,
		DissolveDuration,
		DissolveStartValue,
		DissolveEndValue,
		DissolveScalarParameterName,
		NiagaraDissolveParameterName,
		DissolveTexture,
		DissolveTextureUV,
		DissolveTickInterval);
}

void APRFaerinCharacter::Multicast_SpawnFaerinWorldNiagara_Implementation(
	FSoftObjectPath NiagaraSystemPath,
	FVector_NetQuantize Location,
	FRotator Rotation,
	FVector Scale,
	float LifeSeconds)
{
	SpawnFaerinWorldNiagaraLocal(NiagaraSystemPath, Location, Rotation, Scale, LifeSeconds);
}

void APRFaerinCharacter::Multicast_SpawnFaerinTrackedWorldNiagara_Implementation(
	FName EffectKey,
	FSoftObjectPath NiagaraSystemPath,
	FVector_NetQuantize Location,
	FRotator Rotation,
	FVector Scale,
	float LifeSeconds)
{
	SpawnFaerinTrackedWorldNiagaraLocal(EffectKey, NiagaraSystemPath, Location, Rotation, Scale, LifeSeconds);
}

void APRFaerinCharacter::Multicast_StopFaerinTrackedNiagara_Implementation(FName EffectKey)
{
	StopFaerinTrackedNiagaraLocal(EffectKey);
}

void APRFaerinCharacter::Multicast_SpawnFaerinAttachedNiagara_Implementation(
	FSoftObjectPath NiagaraSystemPath,
	AActor* AttachActor,
	FName AttachSocketName,
	FVector LocationOffset,
	FRotator RotationOffset,
	FVector Scale,
	float LifeSeconds)
{
	SpawnFaerinAttachedNiagaraLocal(
		NiagaraSystemPath,
		AttachActor,
		AttachSocketName,
		LocationOffset,
		RotationOffset,
		Scale,
		LifeSeconds);
}

void APRFaerinCharacter::Multicast_PlayNearTeleportReappearPresentation_Implementation(
	FVector ReappearLocation,
	FRotator ReappearRotation,
	UNiagaraSystem* TeleportOutNiagaraSystem,
	UNiagaraSystem* ReappearDissolveNiagaraSystem,
	FName AttachSocketName,
	const float TeleportOutLifeSeconds,
	const float ReappearDissolveLifeSeconds)
{
	SetActorLocationAndRotation(ReappearLocation, ReappearRotation, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(false);
	SpawnNearTeleportBodyNiagaraLocal(TeleportOutNiagaraSystem, AttachSocketName, TeleportOutLifeSeconds);
	SpawnNearTeleportBodyNiagaraLocal(ReappearDissolveNiagaraSystem, AttachSocketName, ReappearDissolveLifeSeconds);
}

void APRFaerinCharacter::Multicast_SetNearTeleportHidden_Implementation(bool bShouldHide)
{
	SetActorHiddenInGame(bShouldHide);
}

void APRFaerinCharacter::ApplyFaerinCloneMergeHeal(
	const float HealAmount,
	const float HealMaxHealthRatio,
	UNiagaraSystem* HealNiagaraSystem,
	const FName AttachSocketName,
	const FVector LocationOffset,
	const FRotator RotationOffset,
	const FVector Scale,
	const float LifeSeconds)
{
	if (!HasAuthority() || !IsValid(AbilitySystemComponent) || !IsValid(CommonSet) || HealAmount <= 0.0f)
	{
		return;
	}

	const float MaxHealth = CommonSet->GetMaxHealth();
	if (MaxHealth <= UE_SMALL_NUMBER)
	{
		return;
	}

	const float HealCapRatio = HealMaxHealthRatio > 0.0f
		? FMath::Clamp(HealMaxHealthRatio, 0.0f, 1.0f)
		: 1.0f;
	const float HealCap = MaxHealth * HealCapRatio;
	const float CurrentHealth = CommonSet->GetHealth();
	const float NewHealth = FMath::Min(CurrentHealth + HealAmount, HealCap);
	if (NewHealth <= CurrentHealth + UE_SMALL_NUMBER)
	{
		return;
	}

	AbilitySystemComponent->SetNumericAttributeBase(UPRAttributeSet_Common::GetHealthAttribute(), NewHealth);

	if (IsValid(HealNiagaraSystem))
	{
		Multicast_SpawnFaerinCloneHealNiagara(
			HealNiagaraSystem,
			AttachSocketName,
			LocationOffset,
			RotationOffset,
			Scale,
			LifeSeconds);
	}
}

void APRFaerinCharacter::Multicast_SpawnFaerinCloneHealNiagara_Implementation(
	UNiagaraSystem* HealNiagaraSystem,
	const FName AttachSocketName,
	const FVector LocationOffset,
	const FRotator RotationOffset,
	const FVector Scale,
	const float LifeSeconds)
{
	if (!IsValid(HealNiagaraSystem) || !IsValid(GetMesh()))
	{
		return;
	}

	const FName ResolvedSocketName = AttachSocketName != NAME_None && GetMesh()->DoesSocketExist(AttachSocketName)
		? AttachSocketName
		: NAME_None;
	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		HealNiagaraSystem,
		GetMesh(),
		ResolvedSocketName,
		LocationOffset,
		RotationOffset,
		EAttachLocation::KeepRelativeOffset,
		true,
		true,
		ENCPoolMethod::None,
		false);
	if (!IsValid(NiagaraComponent))
	{
		return;
	}

	NiagaraComponent->SetRelativeScale3D(Scale);
	if (LifeSeconds <= UE_SMALL_NUMBER)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
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

void APRFaerinCharacter::SpawnNearTeleportBodyNiagaraLocal(
	UNiagaraSystem* NiagaraSystem,
	FName AttachSocketName,
	const float LifeSeconds) const
{
	if (!IsValid(NiagaraSystem) || !IsValid(GetMesh()))
	{
		return;
	}

	const FTransform SpawnTransform = AttachSocketName != NAME_None
		? GetMesh()->GetSocketTransform(AttachSocketName)
		: GetMesh()->GetComponentTransform();
	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this,
		NiagaraSystem,
		SpawnTransform.GetLocation(),
		SpawnTransform.Rotator(),
		SpawnTransform.GetScale3D(),
		true,
		true,
		ENCPoolMethod::None,
		false);
	if (!IsValid(NiagaraComponent) || LifeSeconds <= UE_SMALL_NUMBER)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
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

void APRFaerinCharacter::SpawnFaerinWorldNiagaraLocal(const FSoftObjectPath& NiagaraSystemPath,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale,
	const float LifeSeconds) const
{
	if (!NiagaraSystemPath.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
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


void APRFaerinCharacter::SpawnFaerinTrackedWorldNiagaraLocal(
	FName EffectKey,
	const FSoftObjectPath& NiagaraSystemPath,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale,
	const float LifeSeconds)
{
	if (EffectKey == NAME_None || !NiagaraSystemPath.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
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
		return;
	}

	StopFaerinTrackedNiagaraLocal(EffectKey);

	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		NiagaraSystem,
		Location,
		Rotation,
		Scale,
		true,
		true);
	if (!IsValid(NiagaraComponent))
	{
		return;
	}

	TrackedFaerinNiagaraComponents.Add(EffectKey, NiagaraComponent);

	if (LifeSeconds <= UE_SMALL_NUMBER)
	{
		return;
	}

	TWeakObjectPtr<UNiagaraComponent> WeakNiagaraComponent = NiagaraComponent;
	FTimerHandle CleanupTimerHandle;
	World->GetTimerManager().SetTimer(
		CleanupTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, EffectKey, WeakNiagaraComponent]()
		{
			if (UNiagaraComponent* ActiveNiagaraComponent = WeakNiagaraComponent.Get())
			{
				ActiveNiagaraComponent->Deactivate();
				ActiveNiagaraComponent->DestroyComponent();
			}

			const TWeakObjectPtr<UNiagaraComponent>* StoredComponent = TrackedFaerinNiagaraComponents.Find(EffectKey);
			if (StoredComponent != nullptr && *StoredComponent == WeakNiagaraComponent)
			{
				TrackedFaerinNiagaraComponents.Remove(EffectKey);
			}
		}),
		LifeSeconds,
		false);
}

void APRFaerinCharacter::StopFaerinTrackedNiagaraLocal(FName EffectKey)
{
	if (EffectKey == NAME_None)
	{
		return;
	}

	TWeakObjectPtr<UNiagaraComponent> WeakNiagaraComponent;
	if (!TrackedFaerinNiagaraComponents.RemoveAndCopyValue(EffectKey, WeakNiagaraComponent))
	{
		return;
	}

	if (UNiagaraComponent* NiagaraComponent = WeakNiagaraComponent.Get())
	{
		NiagaraComponent->Deactivate();
		NiagaraComponent->DestroyComponent();
	}
}

void APRFaerinCharacter::SpawnFaerinAttachedNiagaraLocal(
	const FSoftObjectPath& NiagaraSystemPath,
	AActor* AttachActor,
	const FName AttachSocketName,
	const FVector& LocationOffset,
	const FRotator& RotationOffset,
	const FVector& Scale,
	const float LifeSeconds) const
{
	if (!NiagaraSystemPath.IsValid() || !IsValid(AttachActor) || LifeSeconds <= UE_SMALL_NUMBER)
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
		return;
	}

	USceneComponent* AttachComponent = AttachActor->GetRootComponent();
	if (const ACharacter* AttachCharacter = Cast<ACharacter>(AttachActor))
	{
		if (IsValid(AttachCharacter->GetMesh()))
		{
			AttachComponent = AttachCharacter->GetMesh();
		}
	}

	if (!IsValid(AttachComponent))
	{
		return;
	}

	const FName ResolvedSocketName = AttachSocketName != NAME_None && AttachComponent->DoesSocketExist(AttachSocketName)
		? AttachSocketName
		: NAME_None;

	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		NiagaraSystem,
		AttachComponent,
		ResolvedSocketName,
		LocationOffset,
		RotationOffset,
		EAttachLocation::KeepRelativeOffset,
		true,
		true,
		ENCPoolMethod::None,
		false);
	if (!IsValid(NiagaraComponent))
	{
		return;
	}

	NiagaraComponent->SetRelativeScale3D(Scale);

	UWorld* World = GetWorld();
	if (!IsValid(World))
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


void APRFaerinCharacter::StartNearTeleportDissolveVisual(
	UNiagaraSystem* DissolveNiagaraSystem,
	FName AttachSocketName,
	const float DissolveDuration,
	const float DissolveStartValue,
	const float DissolveEndValue,
	FName DissolveScalarParameterName,
	FName NiagaraDissolveParameterName,
	UTexture* DissolveTexture,
	const FVector2D& DissolveTextureUV,
	const float DissolveTickInterval)
{
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(NearTeleportDissolveTickTimerHandle);
	}

	if (IsValid(ActiveNearTeleportDissolveNiagara))
	{
		ActiveNearTeleportDissolveNiagara->Deactivate();
		ActiveNearTeleportDissolveNiagara = nullptr;
	}

	NearTeleportDissolveDynamicMaterials.Reset();
	NearTeleportDissolveElapsedTime = 0.0f;
	NearTeleportDissolveDuration = FMath::Max(DissolveDuration, 0.0f);
	NearTeleportDissolveStartValue = DissolveStartValue;
	NearTeleportDissolveEndValue = DissolveEndValue;
	NearTeleportDissolveTickInterval = FMath::Max(DissolveTickInterval, 0.001f);
	NearTeleportDissolveScalarParameterName = DissolveScalarParameterName != NAME_None
		? DissolveScalarParameterName
		: TEXT("DissolveAmount");
	NearTeleportNiagaraDissolveParameterName = NiagaraDissolveParameterName != NAME_None
		? NiagaraDissolveParameterName
		: TEXT("User.DissolveAmount");

	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (IsValid(MeshComponent))
	{
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
				DynamicMaterial->SetScalarParameterValue(
					NearTeleportDissolveScalarParameterName,
					NearTeleportDissolveStartValue);
				NearTeleportDissolveDynamicMaterials.Add(DynamicMaterial);
			}
		}

		if (IsValid(DissolveNiagaraSystem))
		{
			const FName ResolvedSocketName = AttachSocketName != NAME_None && MeshComponent->DoesSocketExist(AttachSocketName)
				? AttachSocketName
				: NAME_None;
			ActiveNearTeleportDissolveNiagara = UNiagaraFunctionLibrary::SpawnSystemAttached(
				DissolveNiagaraSystem,
				MeshComponent,
				ResolvedSocketName,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true,
				true,
				ENCPoolMethod::None,
				false);

			if (IsValid(ActiveNearTeleportDissolveNiagara))
			{
				ActiveNearTeleportDissolveNiagara->SetVariableFloat(
					NearTeleportNiagaraDissolveParameterName,
					NearTeleportDissolveStartValue);

				if (IsValid(DissolveTexture))
				{
					ActiveNearTeleportDissolveNiagara->SetVariableTexture(TEXT("User.DissolveTexture"), DissolveTexture);
				}

				ActiveNearTeleportDissolveNiagara->SetVariableVec2(TEXT("User.DissolveTextureUV"), DissolveTextureUV);
				ActiveNearTeleportDissolveNiagara->Activate(true);
			}
		}
	}

	ApplyNearTeleportDissolveVisualValue(NearTeleportDissolveStartValue);

	if (NearTeleportDissolveDuration <= UE_SMALL_NUMBER)
	{
		ApplyNearTeleportDissolveVisualValue(NearTeleportDissolveEndValue);
		CompleteNearTeleportDissolveVisual();
		return;
	}

	if (IsValid(World))
	{
		World->GetTimerManager().SetTimer(
			NearTeleportDissolveTickTimerHandle,
			this,
			&APRFaerinCharacter::TickNearTeleportDissolveVisual,
			NearTeleportDissolveTickInterval,
			true);
	}
	else
	{
		CompleteNearTeleportDissolveVisual();
	}
}

void APRFaerinCharacter::TickNearTeleportDissolveVisual()
{
	NearTeleportDissolveElapsedTime += NearTeleportDissolveTickInterval;

	const float Duration = FMath::Max(NearTeleportDissolveDuration, UE_SMALL_NUMBER);
	const float Alpha = FMath::Clamp(NearTeleportDissolveElapsedTime / Duration, 0.0f, 1.0f);
	const float DissolveValue = FMath::Lerp(
		NearTeleportDissolveStartValue,
		NearTeleportDissolveEndValue,
		Alpha);

	ApplyNearTeleportDissolveVisualValue(DissolveValue);

	if (Alpha >= 1.0f)
	{
		CompleteNearTeleportDissolveVisual();
	}
}

void APRFaerinCharacter::CompleteNearTeleportDissolveVisual()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(NearTeleportDissolveTickTimerHandle);
	}

	ApplyNearTeleportDissolveVisualValue(NearTeleportDissolveEndValue);

	if (IsValid(ActiveNearTeleportDissolveNiagara))
	{
		ActiveNearTeleportDissolveNiagara->Deactivate();
		ActiveNearTeleportDissolveNiagara = nullptr;
	}
}

void APRFaerinCharacter::ApplyNearTeleportDissolveVisualValue(const float DissolveValue)
{
	for (UMaterialInstanceDynamic* DynamicMaterial : NearTeleportDissolveDynamicMaterials)
	{
		if (IsValid(DynamicMaterial))
		{
			DynamicMaterial->SetScalarParameterValue(NearTeleportDissolveScalarParameterName, DissolveValue);
		}
	}

	if (IsValid(ActiveNearTeleportDissolveNiagara))
	{
		ActiveNearTeleportDissolveNiagara->SetVariableFloat(NearTeleportNiagaraDissolveParameterName, DissolveValue);
	}
}

/*~ 보스 조우 이벤트 브로드캐스트 ~*/

void APRFaerinCharacter::SetBossEncounterActive(bool bActive)
{
	if (!HasAuthority() || bBossEncounterActive == bActive)
	{
		return;
	}

	bBossEncounterActive = bActive;
	HandleBossEncounterActiveChanged();
	ForceNetUpdate();
}

void APRFaerinCharacter::HandleBossEncounterActiveChanged()
{
	if (bBossEncounterActive)
	{
		if (!bBossEncounterEventBroadcasted)
		{
			BroadcastBossEncounterBegin();
			bBossEncounterEventBroadcasted = true;
		}
	}
	else if (bBossEncounterEventBroadcasted)
	{
		BroadcastBossEncounterEnd();
		bBossEncounterEventBroadcasted = false;
	}
}

void APRFaerinCharacter::OnRep_BossEncounterActive()
{
	HandleBossEncounterActiveChanged();
}

void APRFaerinCharacter::BroadcastBossEncounterBegin()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>();
	if (!IsValid(EventMgr))
	{
		return;
	}

	FPRBossEncounterEventPayload Payload;
	Payload.Boss = this;
	EventMgr->BroadcastTyped(PRGameplayTags::Event_Boss_Encounter_Begin, Payload);
}

void APRFaerinCharacter::BroadcastBossEncounterEnd()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>();
	if (!IsValid(EventMgr))
	{
		return;
	}

	EventMgr->BroadcastEmpty(PRGameplayTags::Event_Boss_Encounter_End);
}
