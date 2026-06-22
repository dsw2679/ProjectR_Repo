// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (적 체력 UI 및 아이템 drop 연동 구현)
// Author: 배유찬 (어그로 마커(핑) 연동, 대미지 팝업 및 ASC 태그 이벤트 바인딩 구현)
// Author: 손승우 (적 AI 상태 제어, 사망 디졸브 연출 및 아머드 솔저 적 캐릭터 구현)
// Author: 이건주 (Penitent 및 드론/배리어 패턴 지원 적 캐릭터 구현)
#include "PREnemyBaseCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "ProjectR/AI/Controllers/PREnemyAIController.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BrainComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "ProjectR/AI/Components/PREnemyCombatEventRelayComponent.h"
#include "ProjectR/AI/Components/PREnemyThreatComponent.h"
#include "ProjectR/AI/Data/PREnemyCombatDataAsset.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Common.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Enemy.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/Combat/PRCombatStatics.h"
#include "Engine/DataTable.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/System/PREventTypes.h"
#include "ProjectR/System/PRRespawnSubsystem.h"
#include "ProjectR/System/PRWorldTickOptimizerSubsystem.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/UI/FloatingText/PRFloatingTextManager.h"
#include "ProjectR/UI/HUD/PREnemyWorldHealthBarComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "ProjectR/AbilitySystem/PRGameplayAbility.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Mod.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"

namespace
{
	AActor* ResolveEnemyThreatSourceActor(const FPRDamageAppliedContext& Context)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Context.InstigatorController.Get()))
		{
			return PlayerController->GetPawn();
		}

		AActor* InstigatorActor = Context.Instigator.Get();
		if (AController* InstigatorController = Cast<AController>(InstigatorActor))
		{
			return InstigatorController->GetPawn();
		}

		if (APawn* InstigatorPawn = Cast<APawn>(InstigatorActor))
		{
			return InstigatorPawn;
		}

		return InstigatorActor;
	}
}

APREnemyBaseCharacter::APREnemyBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(60.0f);

	AIControllerClass = APREnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);

	AbilitySystemComponent = CreateDefaultSubobject<UPRAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// 적 ASC는 서버 판단이 중심이므로 Minimal 복제로 태그/필수 상태만 공유한다.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	CommonSet = CreateDefaultSubobject<UPRAttributeSet_Common>(TEXT("CommonSet"));
	EnemySet = CreateDefaultSubobject<UPRAttributeSet_Enemy>(TEXT("EnemySet"));
	ThreatComponent = CreateDefaultSubobject<UPREnemyThreatComponent>(TEXT("ThreatComponent"));
	CombatEventRelayComponent = CreateDefaultSubobject<UPREnemyCombatEventRelayComponent>(TEXT("CombatEventRelayComponent"));
	EnemyWorldHealthBarComponent = CreateDefaultSubobject<UPREnemyWorldHealthBarComponent>(TEXT("EnemyWorldHealthBarComponent"));
	EnemyWorldHealthBarComponent->SetupAttachment(GetRootComponent());
}

void APREnemyBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	BindTagChangeEvent();
	
	InitializeHomeLocation();
	InitializeEnemyWorldHealthBar();

	if (bUseTickOptimization)
	{
		// 렌더 컬링 경계 흔들림 완화 거리
		constexpr float EnemyCullDistancePad = 500.f;

		// Tick 최적화 대상 전용 렌더 컬링 거리
		const float RenderCullDistance = FMath::Max(TickOptimizationConfig.TickDeactivationRadius + EnemyCullDistancePad, 0.0f);
		if (USkeletalMeshComponent* MeshComponent = GetMesh())
		{
			MeshComponent->SetCullDistance(RenderCullDistance);
		}
		if (IsValid(EnemyWorldHealthBarComponent))
		{
			EnemyWorldHealthBarComponent->SetCullDistance(RenderCullDistance);
		}
	}

	if (HasAuthority() && bIsRespawnable)
	{
		if (UWorld* World = GetWorld())
		{
			if (UPRRespawnSubsystem* RespawnSubsystem = World->GetSubsystem<UPRRespawnSubsystem>())
			{
				// 리스폰 시스템 등록
				RespawnSubsystem->RegisterRespawnableActor(this);
			}
		}
	}

	if (bUseTickOptimization)
	{
		if (UWorld* World = GetWorld())
		{
			if (UPRWorldTickOptimizerSubsystem* TickOptimizer = World->GetSubsystem<UPRWorldTickOptimizerSubsystem>())
			{
				TickOptimizer->RegisterTarget(this);
			}
		}

		if (!HasAuthority())
		{
			return;
		}

		// CVar 비활성화 상태에서 초기 최적화 적용 방지
		const bool bCanApplyInitialTickOptimization = UPRWorldTickOptimizerSubsystem::IsOptimizationEnabled();
		if (bCanApplyInitialTickOptimization && !TickOptimizationConfig.bStartTickActive)
		{
			SetTickActive(false);
		}
	}
}

void APREnemyBaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindPlayerCountChanged();

	if (bUseTickOptimization)
	{
		if (UWorld* World = GetWorld())
		{
			if (UPRWorldTickOptimizerSubsystem* TickOptimizer = World->GetSubsystem<UPRWorldTickOptimizerSubsystem>())
			{
				TickOptimizer->UnregisterTarget(this);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void APREnemyBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APREnemyBaseCharacter, bMaintainCombatMoveFocus);
	DOREPLIFETIME(APREnemyBaseCharacter, bUseCombatMovePose);
	DOREPLIFETIME(APREnemyBaseCharacter, bUseCombatAimOffset);
	DOREPLIFETIME(APREnemyBaseCharacter, bUseCombatStrafeState);
}

void APREnemyBaseCharacter::PossessedBy(AController* NewController)
{
	InitializeHomeLocation();
	Super::PossessedBy(NewController);

	InitializeEnemyAbilitySystem();

	if (bUseTickOptimization && !bTickActive)
	{
		ApplyTickOptimizationState(false);
	}
}

UPRAbilitySystemComponent* APREnemyBaseCharacter::GetPRAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UPRAbilitySystemComponent* APREnemyBaseCharacter::GetEnemyAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UPREnemyThreatComponent* APREnemyBaseCharacter::GetEnemyThreatComponent() const
{
	return ThreatComponent;
}

UPRPatternDataAsset* APREnemyBaseCharacter::GetPatternDataAsset() const
{
	return PatternDataAsset;
}

UPRCombatMoveDataAsset* APREnemyBaseCharacter::GetCombatDataAsset() const
{
	return CombatDataAsset;
}

UPRPerceptionConfig* APREnemyBaseCharacter::GetPerceptionConfig() const
{
	return PerceptionConfig;
}

UBehaviorTree* APREnemyBaseCharacter::GetBehaviorTreeAsset() const
{
	return BehaviorTreeAsset;
}

FVector APREnemyBaseCharacter::GetHomeLocation() const
{
	return HomeLocation;
}

void APREnemyBaseCharacter::InitializeEnemyBlackboard(UBlackboardComponent* BlackboardComponent) const
{
}

void APREnemyBaseCharacter::ApplyCombatMovePresentationContext(const FPREnemyMovePresentationConfig& PresentationConfig)
{
	CacheMovementPresentationDefaults();
	ApplyMovementPresentationConfig(PresentationConfig);

	bMaintainCombatMoveFocus = PresentationConfig.bMaintainTargetFocus;
	bUseCombatMovePose = PresentationConfig.bUseCombatMovePose;
	bUseCombatAimOffset = PresentationConfig.bUseCombatAimOffset;
	bUseCombatStrafeState = PresentationConfig.bUseCombatStrafeState;
}

void APREnemyBaseCharacter::ClearCombatMovePresentationContext()
{
	RestoreMovementPresentationDefaults();
	bMaintainCombatMoveFocus = false;
	bUseCombatMovePose = false;
	bUseCombatAimOffset = false;
	bUseCombatStrafeState = false;
}

void APREnemyBaseCharacter::RequestDeathDissolveVisual(
	UAnimMontage* InDeathMontage,
	float InMontagePlayRate,
	bool bInDissolveDelayAfterMontageEnd,
	float InDissolveDelay,
	float InDissolveDuration,
	float InDissolveStartValue,
	float InDissolveEndValue,
	FName InDissolveScalarParameterName,
	UNiagaraSystem* InDissolveNiagaraSystem,
	FName InNiagaraDissolveParameterName,
	UTexture* InDissolveTexture,
	FVector2D InDissolveTextureUV,
	float InDissolveTickInterval)
{
	if (!HasAuthority())
	{
		return;
	}

	Multicast_RequestDeathDissolveVisual(
		InDeathMontage,
		InMontagePlayRate,
		bInDissolveDelayAfterMontageEnd,
		InDissolveDelay,
		InDissolveDuration,
		InDissolveStartValue,
		InDissolveEndValue,
		InDissolveScalarParameterName,
		InDissolveNiagaraSystem,
		InNiagaraDissolveParameterName,
		InDissolveTexture,
		InDissolveTextureUV,
		InDissolveTickInterval);
}

void APREnemyBaseCharacter::Multicast_RequestDeathDissolveVisual_Implementation(
	UAnimMontage* InDeathMontage,
	float InMontagePlayRate,
	bool bInDissolveDelayAfterMontageEnd,
	float InDissolveDelay,
	float InDissolveDuration,
	float InDissolveStartValue,
	float InDissolveEndValue,
	FName InDissolveScalarParameterName,
	UNiagaraSystem* InDissolveNiagaraSystem,
	FName InNiagaraDissolveParameterName,
	UTexture* InDissolveTexture,
	FVector2D InDissolveTextureUV,
	float InDissolveTickInterval)
{
	// 서버는 Death Ability가 Actor 제거까지 소유하므로, 멀티캐스트는 클라이언트 시각 연출만 담당한다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathDissolveStartTimerHandle);
		World->GetTimerManager().ClearTimer(DeathDissolveTickTimerHandle);
	}

	DeathDissolveDynamicMaterials.Reset();
	if (IsValid(ActiveDeathDissolveNiagara))
	{
		ActiveDeathDissolveNiagara->Deactivate();
	}
	ActiveDeathDissolveNiagara = nullptr;
	DeathDissolveElapsedTime = 0.0f;
	bDeathDissolveVisualStarted = false;

	PendingDeathDissolveMontage = InDeathMontage;
	PendingDeathDissolveMontagePlayRate = FMath::Max(InMontagePlayRate, UE_SMALL_NUMBER);
	PendingDeathDissolveDelay = FMath::Max(InDissolveDelay, 0.0f);
	bPendingDeathDissolveDelayAfterMontageEnd = bInDissolveDelayAfterMontageEnd;
	PendingDeathDissolveDuration = FMath::Max(InDissolveDuration, 0.0f);
	PendingDeathDissolveStartValue = InDissolveStartValue;
	PendingDeathDissolveEndValue = InDissolveEndValue;
	PendingDeathDissolveScalarParameterName = InDissolveScalarParameterName != NAME_None
		? InDissolveScalarParameterName
		: TEXT("DissolveAmount");
	PendingDeathDissolveNiagaraSystem = InDissolveNiagaraSystem;
	PendingDeathDissolveNiagaraParameterName = InNiagaraDissolveParameterName != NAME_None
		? InNiagaraDissolveParameterName
		: TEXT("User.DissolveAmount");
	PendingDeathDissolveTexture = InDissolveTexture;
	PendingDeathDissolveTextureUV = InDissolveTextureUV;
	PendingDeathDissolveTickInterval = FMath::Max(InDissolveTickInterval, 0.001f);

	// Death는 곧바로 Dead 태그, AI 정지, Dissolve 예약이 이어지므로 클라이언트 시각 몽타주를 한 번 더 보장한다.
	if (IsValid(PendingDeathDissolveMontage) && !HasAuthority())
	{
		PlayDeathDissolveMontageIfNeeded();
	}

	const float MontageDelay = bPendingDeathDissolveDelayAfterMontageEnd && IsValid(PendingDeathDissolveMontage)
		? (PendingDeathDissolveMontage->GetPlayLength() / PendingDeathDissolveMontagePlayRate) + 0.1f
		: 0.0f;
	const float InitialDelay = bPendingDeathDissolveDelayAfterMontageEnd
		? MontageDelay
		: PendingDeathDissolveDelay;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DeathDissolveStartTimerHandle,
			this,
			&APREnemyBaseCharacter::BeginDeathDissolveVisual,
			FMath::Max(InitialDelay, 0.0f),
			false);
	}
	else
	{
		BeginDeathDissolveVisual();
	}
}

void APREnemyBaseCharacter::PlayDeathDissolveMontageIfNeeded()
{
	if (!IsValid(PendingDeathDissolveMontage))
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!IsValid(MeshComponent))
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!IsValid(AnimInstance))
	{
		return;
	}

	if (AnimInstance->Montage_IsPlaying(PendingDeathDissolveMontage))
	{
		return;
	}

	AnimInstance->Montage_Play(PendingDeathDissolveMontage, PendingDeathDissolveMontagePlayRate);
}

void APREnemyBaseCharacter::BeginDeathDissolveVisual()
{
	if (bDeathDissolveVisualStarted)
	{
		return;
	}

	bDeathDissolveVisualStarted = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathDissolveStartTimerHandle);
	}

	// 몽타주 종료 후 Dissolve 모드에서는 마지막 자세를 고정한 뒤 Dissolve를 시작한다.
	// 몽타주 시작 후 Dissolve 모드에서는 Dissolve가 몽타주 도중에 겹쳐야 하므로 애니메이션을 멈추면 안 된다.
	if (bPendingDeathDissolveDelayAfterMontageEnd)
	{
		FreezeDeathDissolvePose();
	}

	if (!bPendingDeathDissolveDelayAfterMontageEnd || PendingDeathDissolveDelay <= 0.0f)
	{
		StartDeathDissolveVisual();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DeathDissolveStartTimerHandle,
			this,
			&APREnemyBaseCharacter::StartDeathDissolveVisual,
			PendingDeathDissolveDelay,
			false);
	}
	else
	{
		StartDeathDissolveVisual();
	}
}

void APREnemyBaseCharacter::StartDeathDissolveVisual()
{
	DeathDissolveElapsedTime = 0.0f;
	DeathDissolveDynamicMaterials.Reset();

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
					PendingDeathDissolveScalarParameterName,
					PendingDeathDissolveStartValue);
				DeathDissolveDynamicMaterials.Add(DynamicMaterial);
			}
		}

		if (IsValid(PendingDeathDissolveNiagaraSystem))
		{
			ActiveDeathDissolveNiagara = UNiagaraFunctionLibrary::SpawnSystemAttached(
				PendingDeathDissolveNiagaraSystem,
				MeshComponent,
				NAME_None,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true,
				true);

			if (IsValid(ActiveDeathDissolveNiagara))
			{
				ActiveDeathDissolveNiagara->SetVariableFloat(
					PendingDeathDissolveNiagaraParameterName,
					PendingDeathDissolveStartValue);

				if (IsValid(PendingDeathDissolveTexture))
				{
					ActiveDeathDissolveNiagara->SetVariableTexture(TEXT("User.DissolveTexture"), PendingDeathDissolveTexture);
				}

				ActiveDeathDissolveNiagara->SetVariableVec2(TEXT("User.DissolveTextureUV"), PendingDeathDissolveTextureUV);
				ActiveDeathDissolveNiagara->Activate(true);
			}
		}
	}

	ApplyDeathDissolveVisualValue(PendingDeathDissolveStartValue);

	if (PendingDeathDissolveDuration <= UE_SMALL_NUMBER)
	{
		ApplyDeathDissolveVisualValue(PendingDeathDissolveEndValue);
		CompleteDeathDissolveVisual();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DeathDissolveTickTimerHandle,
			this,
			&APREnemyBaseCharacter::TickDeathDissolveVisual,
			PendingDeathDissolveTickInterval,
			true);
	}
	else
	{
		CompleteDeathDissolveVisual();
	}
}

void APREnemyBaseCharacter::TickDeathDissolveVisual()
{
	DeathDissolveElapsedTime += PendingDeathDissolveTickInterval;

	const float Duration = FMath::Max(PendingDeathDissolveDuration, UE_SMALL_NUMBER);
	const float Alpha = FMath::Clamp(DeathDissolveElapsedTime / Duration, 0.0f, 1.0f);
	const float DissolveValue = FMath::Lerp(
		PendingDeathDissolveStartValue,
		PendingDeathDissolveEndValue,
		Alpha);

	ApplyDeathDissolveVisualValue(DissolveValue);

	if (Alpha >= 1.0f)
	{
		CompleteDeathDissolveVisual();
	}
}

void APREnemyBaseCharacter::CompleteDeathDissolveVisual()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathDissolveTickTimerHandle);
	}

	ApplyDeathDissolveVisualValue(PendingDeathDissolveEndValue);

	if (IsValid(ActiveDeathDissolveNiagara))
	{
		ActiveDeathDissolveNiagara->Deactivate();
		ActiveDeathDissolveNiagara = nullptr;
	}
}

void APREnemyBaseCharacter::ApplyDeathDissolveVisualValue(float DissolveValue)
{
	for (UMaterialInstanceDynamic* DynamicMaterial : DeathDissolveDynamicMaterials)
	{
		if (IsValid(DynamicMaterial))
		{
			DynamicMaterial->SetScalarParameterValue(PendingDeathDissolveScalarParameterName, DissolveValue);
		}
	}

	if (IsValid(ActiveDeathDissolveNiagara))
	{
		ActiveDeathDissolveNiagara->SetVariableFloat(PendingDeathDissolveNiagaraParameterName, DissolveValue);
	}
}

void APREnemyBaseCharacter::FreezeDeathDissolvePose()
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!IsValid(MeshComponent))
	{
		return;
	}

	MeshComponent->bPauseAnims = true;
	MeshComponent->GlobalAnimRateScale = 0.0f;
}

void APREnemyBaseCharacter::GiveModGauge(const FPRDamageAppliedContext& Context) const
{
	// 서버에서만 실행. InstigatorController에게 Unreliable RPC로 데미지 텍스트를 전송한다
	if (!HasAuthority())
	{
		return;
	}

    // 드론이 준 피해로는 모드 게이지를 충전하지 않음
	if (Context.EffectTags.HasTagExact(PRCombatGameplayTags::Ability_Source_Drone))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(
		Context.Instigator.Get());
	if (!IsValid(TargetASC))
	{
		return;
	}
	
	TSubclassOf<UGameplayEffect> ModGaugeEffect = UPRAssetManager::Get().GetAbilitySystemRegistry()->ModGaugeGrantGE;
	if (!ensureMsgf(IsValid(ModGaugeEffect), TEXT("ModGaugeGrantGE가 AbilitySystemRegistry에 없음")))
	{
		return;
	}
	
	FGameplayEffectContextHandle EffectContext = TargetASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(ModGaugeEffect,1.0f,EffectContext);
	
	// Source 무기의 탄약 배수에 따라 모드 게이지 충전량 적용
	UPRGameplayAbility* GA = Cast<UPRGameplayAbility>(Context.SourceObject.Get());
	UPRWeaponDataAsset* WeaponData = nullptr;
	
	// 1. SourceObject가 GA인 경우
	if (IsValid(GA))
	{
		WeaponData = GA->GetCurrentWeaponData();
	}
	else
	{
		// 2. SourceObject가 WeaponItem인 경우
		if (UPRItemInstance_Weapon* WeaponItem = Cast<UPRItemInstance_Weapon>(Context.SourceObject.Get()))
		{
			WeaponData = WeaponItem->GetWeaponData();
		}
		// 3. SourceObject가 ModItem인 경우
		if (UPRItemInstance_Mod* ModItem = Cast<UPRItemInstance_Mod>(Context.SourceObject.Get()))
		{
			if (UPRItemInstance_Weapon* WeaponItem =  ModItem->GetEquippedWeaponItem())
			{
				WeaponData = WeaponItem->GetWeaponData();
			}
		}
	}
	
	if (IsValid(WeaponData))
	{
		// TODO: 게이지 충전 공식 적용. 현재는 탄창 크기 기준 (한 탄창에 500)
		float MagazineSize = WeaponData->MaxMagazineAmmo;
		if (FMath::IsNearlyZero(MagazineSize))
		{
			MagazineSize = 100.f;
		}
			
		float IncomingModGaugeAmount = 500.0f / MagazineSize;
			
		if (Context.EffectTags.HasTagExact(PRCombatGameplayTags::Ability_Source_Weapon_Primary))
		{
			SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_ModGauge_Primary, IncomingModGaugeAmount);
		}
		else if (Context.EffectTags.HasTagExact(PRCombatGameplayTags::Ability_Source_Weapon_Secondary))
		{
			SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_ModGauge_Secondary, IncomingModGaugeAmount);
		}
	}
	
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void APREnemyBaseCharacter::CacheMovementPresentationDefaults()
{
	if (bHasCachedMovementPresentation)
	{
		return;
	}

	if (!IsValid(GetCharacterMovement()))
	{
		return;
	}

	bHasCachedMovementPresentation = true;
	CachedMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	CachedRotationRate = GetCharacterMovement()->RotationRate;
	bCachedOrientRotationToMovement = GetCharacterMovement()->bOrientRotationToMovement;
	bCachedUseControllerDesiredRotation = GetCharacterMovement()->bUseControllerDesiredRotation;
	bCachedUseControllerRotationYaw = bUseControllerRotationYaw;
}

void APREnemyBaseCharacter::ApplyMovementPresentationConfig(const FPREnemyMovePresentationConfig& PresentationConfig)
{
	if (!IsValid(GetCharacterMovement()))
	{
		return;
	}

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = PresentationConfig.bOrientRotationToMovement;
	GetCharacterMovement()->bUseControllerDesiredRotation = PresentationConfig.bUseControllerDesiredRotation;

	if (PresentationConfig.RotationYawRate > 0.0f)
	{
		GetCharacterMovement()->RotationRate = FRotator(0.0f, PresentationConfig.RotationYawRate, 0.0f);
	}
	else if (bHasCachedMovementPresentation)
	{
		GetCharacterMovement()->RotationRate = CachedRotationRate;
	}

	if (PresentationConfig.MoveSpeedOverride > 0.0f)
	{
		GetCharacterMovement()->MaxWalkSpeed = PresentationConfig.MoveSpeedOverride;
	}
	else if (bHasCachedMovementPresentation)
	{
		GetCharacterMovement()->MaxWalkSpeed = CachedMaxWalkSpeed;
	}
}

void APREnemyBaseCharacter::RestoreMovementPresentationDefaults()
{
	if (!bHasCachedMovementPresentation || !IsValid(GetCharacterMovement()))
	{
		return;
	}

	GetCharacterMovement()->MaxWalkSpeed = CachedMaxWalkSpeed;
	GetCharacterMovement()->RotationRate = CachedRotationRate;
	GetCharacterMovement()->bOrientRotationToMovement = bCachedOrientRotationToMovement;
	GetCharacterMovement()->bUseControllerDesiredRotation = bCachedUseControllerDesiredRotation;
	bUseControllerRotationYaw = bCachedUseControllerRotationYaw;
	bHasCachedMovementPresentation = false;
}
FPRDamageRegionInfo APREnemyBaseCharacter::GetDamageRegionInfo(FName BoneName) const
{
	if (bHasCachedStatRow)
	{
		if (const FPRDamageRegionInfo* Found = CachedStatRow.RegionMap.Find(BoneName))
		{
			return *Found;
		}
	}
	return FPRDamageRegionInfo();
}

void APREnemyBaseCharacter::OnPostDamageApplied(const FPRDamageAppliedContext& Context)
{
	Super::OnPostDamageApplied(Context);

	// 서버에서만 실행. InstigatorController에게 Unreliable RPC로 데미지 텍스트를 전송한다
	if (!HasAuthority())
	{
		return;
	}

	if (Context.FinalDamage > 0.0f && bUseTickOptimization)
	{
		if (UWorld* World = GetWorld())
		{
			if (UPRWorldTickOptimizerSubsystem* TickOptimizer = World->GetSubsystem<UPRWorldTickOptimizerSubsystem>())
			{
				// 실제 피해를 받은 적의 전투 반응 유지를 위한 강제 활성화
				TickOptimizer->ForceActivateTarget(this);
			}
		}
	}
	
	GiveModGauge(Context);

	if (Context.FinalDamage > 0.0f && IsValid(ThreatComponent))
	{
		AActor* ThreatSourceActor = ResolveEnemyThreatSourceActor(Context);
		if (UPRCombatStatics::GetActorTeam(ThreatSourceActor) == EPRTeam::Player)
		{
			ThreatComponent->AddDamageThreat(ThreatSourceActor, Context.FinalDamage);

			if (!Context.EffectTags.HasTagExact(PRCombatGameplayTags::Ability_Source_Drone))
			{
				if (UWorld* World = GetWorld())
				{
					if (UPREventManagerSubsystem* EventManager = World->GetSubsystem<UPREventManagerSubsystem>())
					{
						FPRPlayerAttackTargetPayload Payload;
						Payload.Attacker = ThreatSourceActor;
						Payload.Target = this;
						Payload.ServerWorldTimeSeconds = World->GetTimeSeconds();
						Payload.HitResult = Context.HitResult;
						EventManager->BroadcastTyped(PRGameplayTags::Event_Player_AttackTarget, Payload);
					}
				}
			}
		}
	}

	APRPlayerController* PC = Cast<APRPlayerController>(Context.InstigatorController.Get());
	if (!IsValid(PC))
	{
		return;
	}

	UPRFloatingTextManager* FloatingTextManager = PC->GetFloatingTextManager();
	if (!IsValid(FloatingTextManager))
	{
		return;
	}

	// 텍스트 타입 결정
	EPRFloatingTextType TextType = EPRFloatingTextType::NormalDamage;
	if (Context.Region.IsWeakpoint())
	{
		TextType = Context.bIsCritical ? EPRFloatingTextType::WeakCritical : EPRFloatingTextType::WeakNormal;
	}
	else if (Context.bIsCritical)
	{
		TextType = EPRFloatingTextType::CriticalDamage;
	}

	FPRFloatingTextRequest Request;
	Request.Text = FText::AsNumber(FMath::CeilToInt(Context.FinalDamage));
	Request.TextType = TextType;
	Request.WorldLocation = Context.HitResult.ImpactPoint;

	FloatingTextManager->ClientShowFloatingText_Unreliable(Request);
}

FPRWorldMarkerVisualData APREnemyBaseCharacter::GetPingMarkerVisualData_Implementation() const
{
	if (const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>())
	{
		return Settings->GetWorldMarkerPreset(EPRWorldMarkerPreset::Enemy);
	}
	
	UE_LOG(LogTemp,Warning,TEXT("PRDeveloperSettings를 찾을 수 없음"));
	return FPRWorldMarkerVisualData();
}

FVector APREnemyBaseCharacter::GetPingMarkerWorldLocation_Implementation() const
{
	FVector ActorLocation = GetActorLocation();
	float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	return ActorLocation + PingMarkerOffset + FVector(0, 0, HalfHeight);
}

bool APREnemyBaseCharacter::ShouldPingMarkerVisible_Implementation() const
{
	return !IsDead();
}

/*~ IPRTickOptimizable ~*/

FPRTickOptimizationConfig APREnemyBaseCharacter::GetTickConfig() const
{
	return TickOptimizationConfig;
}

FVector APREnemyBaseCharacter::GetTickLocation() const
{
	return GetActorLocation();
}

bool APREnemyBaseCharacter::CanOptimizeTick() const
{
	return bUseTickOptimization && !IsDead();
}

bool APREnemyBaseCharacter::IsTickActive() const
{
	return bTickActive;
}

void APREnemyBaseCharacter::SetTickActive(bool bActive)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bTickActive == bActive)
	{
		return;
	}

	bTickActive = bActive;
	ApplyTickOptimizationState(bActive);
}

void APREnemyBaseCharacter::HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool TagExists)
{
	Super::HandleGameplayTagUpdated(ChangedTag, TagExists);
	
	if (ChangedTag.MatchesTag(PRGameplayTags::State_Dead))
	{
		HandleDeadTagChanged(TagExists);
	}
	if (ChangedTag.MatchesTag(PRGameplayTags::State_Groggy))
	{
		HandleGroggyTagChanged(TagExists);
	}
}

void APREnemyBaseCharacter::InitializeHomeLocation()
{
	if (bHasInitializedHomeLocation)
	{
		return;
	}

	HomeLocation = GetActorLocation();
	bHasInitializedHomeLocation = true;
}

void APREnemyBaseCharacter::InitializeEnemyAbilitySystem()
{
	if (!HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry())
	{
		// CharacterID와 Role을 기준으로 DataTable 초기 스탯을 주입한다.
		AbilitySystemComponent->InitializeAttributesFromRegistry(Registry, GetCharacterRole(), CharacterID);
		BaseMaxHealth = AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Common::GetMaxHealthAttribute());

		// 부위 매핑 조회를 위해 동일 Row를 캐릭터에도 캐싱한다.
		if (UDataTable* StatTable = Registry->GetStatTableSynchronous(GetCharacterRole()))
		{
			static const FString ContextString(TEXT("APREnemyBaseCharacter::CacheStatRow"));
			if (const FPREnemyStatRow* FoundRow = StatTable->FindRow<FPREnemyStatRow>(CharacterID, ContextString, false))
			{
				CachedStatRow = *FoundRow;
				bHasCachedStatRow = true;
			}
		}
	}

	BindPlayerCountChanged();
	ApplyHealthScale();

	// 재 Possess나 재초기화 상황에서 이전 AbilitySet이 중복 부여되지 않도록 먼저 회수한다.
	AbilitySystemComponent->ClearAbilitySetByHandles(GrantedAbilitySetHandles);

	if (IsValid(AbilitySet))
	{
		AbilitySystemComponent->GiveAbilitySet(AbilitySet, GrantedAbilitySetHandles);
	}

	// 적 AvatarActor 준비 이후 어빌리티 훅 호출
	AbilitySystemComponent->NotifyAvatarSet();

	InitializeEnemyWorldHealthBar();
}

void APREnemyBaseCharacter::ApplyHealthScale()
{
	if (!HasAuthority() || !IsValid(AbilitySystemComponent) || BaseMaxHealth <= 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	const APRGameStateBase* PRGameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(PRGameState))
	{
		return;
	}

	const float OldMaxHealth = AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Common::GetMaxHealthAttribute());
	const float OldHealth = AbilitySystemComponent->GetNumericAttribute(UPRAttributeSet_Common::GetHealthAttribute());
	const float OldHealthRatio = OldMaxHealth > UE_SMALL_NUMBER
		? FMath::Clamp(OldHealth / OldMaxHealth, 0.0f, 1.0f)
		: 1.0f;

	const float HealthScale = PREnemyHealthScaling::CalculateHealthScale(PRGameState->GetPlayerCount());
	const float NewMaxHealth = FMath::Max(BaseMaxHealth * HealthScale, 0.0f);
	const float NewHealth = FMath::Clamp(NewMaxHealth * OldHealthRatio, 0.0f, NewMaxHealth);

	if (EnemyWorldHealthBarComponent)
	{
		EnemyWorldHealthBarComponent->SuppressVisibility(true);
	}
	
	// MaxHealth 선적용 후 Health 보정
	AbilitySystemComponent->SetNumericAttributeBase(UPRAttributeSet_Common::GetMaxHealthAttribute(), NewMaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(UPRAttributeSet_Common::GetHealthAttribute(), NewHealth);
	
	if (EnemyWorldHealthBarComponent)
	{
		EnemyWorldHealthBarComponent->SuppressVisibility(false);
	}
}

void APREnemyBaseCharacter::BindPlayerCountChanged()
{
	if (!HasAuthority() || PlayerCountChangedDelegateHandle.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	APRGameStateBase* PRGameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(PRGameState))
	{
		return;
	}

	PlayerCountChangedDelegateHandle = PRGameState->OnPlayerCountChanged.AddUObject(this, &ThisClass::HandlePlayerCountChanged);
}

void APREnemyBaseCharacter::UnbindPlayerCountChanged()
{
	if (!PlayerCountChangedDelegateHandle.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	APRGameStateBase* PRGameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (IsValid(PRGameState))
	{
		PRGameState->OnPlayerCountChanged.Remove(PlayerCountChangedDelegateHandle);
	}

	PlayerCountChangedDelegateHandle.Reset();
}

void APREnemyBaseCharacter::HandlePlayerCountChanged()
{
	ApplyHealthScale();
}

void APREnemyBaseCharacter::InitializeEnemyWorldHealthBar()
{
	if (!IsValid(EnemyWorldHealthBarComponent))
	{
		return;
	}

	if (!bUseWorldHealthBar)
	{
		EnemyWorldHealthBarComponent->HideHealthBar();
		EnemyWorldHealthBarComponent->SetComponentTickEnabled(false);
		return;
	}

	EnemyWorldHealthBarComponent->SetComponentTickEnabled(true);
	EnemyWorldHealthBarComponent->InitializeFromAbilitySystem(AbilitySystemComponent);
}

/*~ Tick 최적화 ~*/

void APREnemyBaseCharacter::ApplyTickOptimizationState(bool bActive)
{
	if (!HasAuthority())
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (IsValid(MovementComponent))
	{
		MovementComponent->SetComponentTickEnabled(bActive);
		if (!bActive)
		{
			MovementComponent->StopMovementImmediately();
		}
	}

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		if (bActive)
		{
			if (UBrainComponent* BrainComponent = AIController->GetBrainComponent())
			{
				BrainComponent->RestartLogic();
			}

			if (UAIPerceptionComponent* PerceptionComponent = AIController->GetPerceptionComponent())
			{
				PerceptionComponent->SetComponentTickEnabled(true);
				PerceptionComponent->Activate(true);
			}
		}
		else
		{
			AIController->StopMovement();
			AIController->ClearFocus(EAIFocusPriority::Gameplay);

			if (UBrainComponent* BrainComponent = AIController->GetBrainComponent())
			{
				BrainComponent->StopLogic(TEXT("TickOptimization"));
			}

			if (UAIPerceptionComponent* PerceptionComponent = AIController->GetPerceptionComponent())
			{
				PerceptionComponent->Deactivate();
				PerceptionComponent->SetComponentTickEnabled(false);
			}
		}
	}

	if (!bActive)
	{
		ClearCombatMovePresentationContext();
	}
}

void APREnemyBaseCharacter::HandleDeath(AActor* InstigatorActor)
{
	// 사망은 Ability 쪽에서도 처리하지만, 캐릭터 레벨에서도 Brain/Movement를 확실히 멈춘다.
	GetCharacterMovement()->DisableMovement();

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();

		if (UBrainComponent* BrainComponent = AIController->GetBrainComponent())
		{
			BrainComponent->StopLogic(TEXT("Dead"));
		}
	}
}

void APREnemyBaseCharacter::HandleDeadTagChanged(bool bEntered)
{
	// 사망 상태 진입
	if (bEntered)
	{
		SetActorHiddenInGame(false);
		if (USkeletalMeshComponent* MeshComponent = GetMesh())
		{
			MeshComponent->SetVisibility(true, true);
			MeshComponent->SetComponentTickEnabled(true);
		}

		if (IsValid(ThreatComponent))
		{
			ThreatComponent->ForceClearAttackCommit();
		}

		ClearCombatMovePresentationContext();
		HandleDeath(nullptr);
	}
}

void APREnemyBaseCharacter::HandleGroggyTagChanged(bool bEntered)
{
	// 그로기 상태 진입
	if (bEntered)
	{
		if (IsValid(ThreatComponent))
		{
			ThreatComponent->ForceClearAttackCommit();
		}

		ClearCombatMovePresentationContext();
		GetCharacterMovement()->StopMovementImmediately();
	}
}
