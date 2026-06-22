// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Fire 프리뷰 컴포넌트 구현)
#include "PRFirePreviewComponent.h"
#include "ProjectR/Combat/PRCombatInterface.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Character/Enemy/PREnemyBaseCharacter.h"
#include "ProjectR/ItemSystem/Actors/PRWeaponActor.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/Projectile/PRProjectileBase.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/UI/Crosshair/PRCrosshairTypes.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRPathPreviewComponent, Log, All);

namespace PRFirePreviewPrivate
{
	// 무기 데이터 조회 실패 시 기존 사격 거리 보존값
	constexpr float DefaultMaxFireDistance = 20000.f;
}

UPRFirePreviewComponent::UPRFirePreviewComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// CDO 단계에서 기본 메시 바인딩. BP에서 슬롯이 비워져도 폴백 동작 보장
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultPreviewMeshFinder(
		TEXT("/Game/3_Effects/PathPreview/SM_PreviewPoint.SM_PreviewPoint"));
	if (DefaultPreviewMeshFinder.Succeeded())
	{
		PreviewMesh = DefaultPreviewMeshFinder.Object;
	}
}

void UPRFirePreviewComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}
	
	GetWeaponManager();

	// Lazy 생성. 첫 호출 시 owner 액터에 ISMC을 동적 생성/등록.
	// 런타임 SceneComponent 생성 표준 패턴: NewObject 후 Mobility 설정과 SetupAttachment를 RegisterComponent 이전에 수행
	if (!IsValid(TrajectoryISMC))
	{
		TrajectoryISMC = NewObject<UInstancedStaticMeshComponent>(Owner);
		TrajectoryISMC->SetMobility(EComponentMobility::Movable);
		if (USceneComponent* OwnerRoot = Owner->GetRootComponent())
		{
			TrajectoryISMC->SetupAttachment(OwnerRoot);
		}
		TrajectoryISMC->SetStaticMesh(PreviewMesh);
		TrajectoryISMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TrajectoryISMC->SetGenerateOverlapEvents(false);
		TrajectoryISMC->SetCastShadow(false);
		// 머티리얼이 PerInstanceCustomData를 샘플링하여 일반/착탄 색상 분기 가능하도록 1채널 확보
		TrajectoryISMC->SetNumCustomDataFloats(1);
		TrajectoryISMC->RegisterComponent();
	}
}

void UPRFirePreviewComponent::SetTrajectoryPreviewEnabled(bool bInEnabled)
{
	const bool bWasTrajectoryEnabled = bIsTrajectoryEnabled;
	bIsTrajectoryEnabled = bInEnabled;

	if (bWasTrajectoryEnabled == bIsTrajectoryEnabled)
	{
		return;
	}

	if (bIsTrajectoryEnabled)
	{
		ClearHitScanPreview();
	}
	else
	{
		ClearTrajectory();
	}
}

/*~ Public API ~*/

void UPRFirePreviewComponent::RegisterProjectilePreviewParams(
	const FPRFirePreviewKey& Key,
	const FPRFirePreviewEntry& Entry)
{
	if (!Key.IsValid() || !Entry.AbilitySpecHandle.IsValid() || Entry.Params.InitialSpeed <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogPRPathPreviewComponent, Warning,
			TEXT("Preview 등록 중단. Owner=%s, Slot=%s, Mode=%s, AbilityHandle=%s, InitialSpeed=%.2f"),
			*GetNameSafe(GetOwner()),
			*UEnum::GetValueAsString(Key.WeaponSlot),
			*UEnum::GetValueAsString(Key.FireMode),
			*Entry.AbilitySpecHandle.ToString(),
			Entry.Params.InitialSpeed);
		return;
	}

	if (const FPRFirePreviewEntry* ExistingEntry = PreviewEntries.Find(Key))
	{
		if (ExistingEntry->AbilitySpecHandle != Entry.AbilitySpecHandle)
		{
			UE_LOG(LogPRPathPreviewComponent, Warning,
				TEXT("Preview 등록 덮어쓰기. Owner=%s, Slot=%s, Mode=%s, OldHandle=%s, NewHandle=%s, OldSource=%s, NewSource=%s"),
				*GetNameSafe(GetOwner()),
				*UEnum::GetValueAsString(Key.WeaponSlot),
				*UEnum::GetValueAsString(Key.FireMode),
				*ExistingEntry->AbilitySpecHandle.ToString(),
				*Entry.AbilitySpecHandle.ToString(),
				*GetNameSafe(ExistingEntry->SourceObject.Get()),
				*GetNameSafe(Entry.SourceObject.Get()));
		}
	}

	PreviewEntries.Add(Key, Entry);

	UE_LOG(LogPRPathPreviewComponent, Log,
		TEXT("Preview 등록 완료. Owner=%s, Slot=%s, Mode=%s, AbilityHandle=%s, Source=%s, ProjectileClass=%s"),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(Key.WeaponSlot),
		*UEnum::GetValueAsString(Key.FireMode),
		*Entry.AbilitySpecHandle.ToString(),
		*GetNameSafe(Entry.SourceObject.Get()),
		*GetNameSafe(Entry.ProjectileClass.Get()));

	if (bIsShowing)
	{
		RefreshActivePreviewParams();
	}
}

void UPRFirePreviewComponent::UnregisterProjectilePreviewParams(
	const FPRFirePreviewKey& Key,
	FGameplayAbilitySpecHandle AbilitySpecHandle)
{
	FPRFirePreviewEntry* ExistingEntry = PreviewEntries.Find(Key);
	if (ExistingEntry == nullptr)
	{
		return;
	}

	if (ExistingEntry->AbilitySpecHandle != AbilitySpecHandle)
	{
		UE_LOG(LogPRPathPreviewComponent, Verbose,
			TEXT("Preview 해제 생략. Owner=%s, Slot=%s, Mode=%s, RegisteredHandle=%s, RemoveHandle=%s"),
			*GetNameSafe(GetOwner()),
			*UEnum::GetValueAsString(Key.WeaponSlot),
			*UEnum::GetValueAsString(Key.FireMode),
			*ExistingEntry->AbilitySpecHandle.ToString(),
			*AbilitySpecHandle.ToString());
		return;
	}

	PreviewEntries.Remove(Key);

	UE_LOG(LogPRPathPreviewComponent, Log,
		TEXT("Preview 해제 완료. Owner=%s, Slot=%s, Mode=%s, AbilityHandle=%s"),
		*GetNameSafe(GetOwner()),
		*UEnum::GetValueAsString(Key.WeaponSlot),
		*UEnum::GetValueAsString(Key.FireMode),
		*AbilitySpecHandle.ToString());

	if (ActivePreviewKey.IsSet() && ActivePreviewKey.GetValue() == Key)
	{
		ActivePreviewKey.Reset();
		RefreshActivePreviewParams();
	}
}

void UPRFirePreviewComponent::RefreshActivePreviewParams()
{
	FPRFirePreviewKey ResolvedKey;
	if (!TryResolveActivePreviewKey(ResolvedKey))
	{
		ActivePreviewKey.Reset();
		SetTrajectoryPreviewEnabled(false);
		return;
	}

	const FPRFirePreviewEntry* Entry = PreviewEntries.Find(ResolvedKey);
	if (Entry == nullptr || Entry->Params.InitialSpeed <= KINDA_SMALL_NUMBER)
	{
		ActivePreviewKey.Reset();
		SetTrajectoryPreviewEnabled(false);
		return;
	}

	const bool bKeyChanged = !ActivePreviewKey.IsSet() || !(ActivePreviewKey.GetValue() == ResolvedKey);
	ActivePreviewKey = ResolvedKey;
	ApplyFireParams(Entry->Params);
	SetTrajectoryPreviewEnabled(true);

	if (bKeyChanged)
	{
		ClearTrajectory();
		UE_LOG(LogPRPathPreviewComponent, Log,
			TEXT("Active Preview 갱신. Owner=%s, Slot=%s, Mode=%s, Source=%s, ProjectileClass=%s"),
			*GetNameSafe(GetOwner()),
			*UEnum::GetValueAsString(ResolvedKey.WeaponSlot),
			*UEnum::GetValueAsString(ResolvedKey.FireMode),
			*GetNameSafe(Entry->SourceObject.Get()),
			*GetNameSafe(Entry->ProjectileClass.Get()));
	}
}

void UPRFirePreviewComponent::ApplyFireParams(const FPRProjectilePreviewParams& InParams)
{
	FireParams = InParams;

	UE_LOG(LogPRPathPreviewComponent, Verbose,
		TEXT("Preview 파라미터 적용. Owner=%s, InitialSpeed=%.2f, GravityScale=%.2f, CollisionRadius=%.2f, MaxSimTime=%.2f, SimFrequency=%.2f, SampleSpacing=%.2f, MaxSamplePoints=%d"),
		*GetNameSafe(GetOwner()),
		FireParams.InitialSpeed,
		FireParams.GravityScale,
		FireParams.CollisionRadius,
		FireParams.MaxSimTime,
		FireParams.SimFrequency,
		FireParams.SampleSpacing,
		FireParams.MaxSamplePoints);
}

void UPRFirePreviewComponent::SetWeaponActor(APRWeaponActor* InWeaponActor)
{
	UE_LOG(LogPRPathPreviewComponent, Log,
		TEXT("SetWeaponActor 호출. Owner=%s, OldWeapon=%s, NewWeapon=%s, bIsShowing=%d, bIsEnabled=%d"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(WeaponActor.Get()),
		*GetNameSafe(InWeaponActor),
		bIsShowing,
		bIsTrajectoryEnabled);

	WeaponActor = InWeaponActor;

	// 댕글링 방지: 무효 WeaponActor 주입 시 표시 강제 OFF
	if (!IsValid(InWeaponActor) && bIsShowing)
	{
		Hide();
	}
}

void UPRFirePreviewComponent::Show()
{
	GetWeaponManager();
	RefreshActivePreviewParams();

	if (bIsShowing)
	{
		return;
	}

	bIsShowing = true;
	SetComponentTickEnabled(true);
}

void UPRFirePreviewComponent::Hide()
{
	if (!bIsShowing)
	{
		return;
	}

	bIsShowing = false;
	SetComponentTickEnabled(false);
	ClearTrajectory();
	ClearHitScanPreview();
}

/*~ Draw ~*/

void UPRFirePreviewComponent::DrawTrajectory(const TArray<FVector>& SamplePoints)
{
#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
	{
		UWorld* World = GetWorld();
		if (IsValid(World) && SamplePoints.Num() > 0)
		{
			const bool bImpactDetected = LastResult.EndReason == EPRPreviewEndReason::HitBlocking;
			const int32 LastIndex = SamplePoints.Num() - 1;

			for (int32 i = 0; i < SamplePoints.Num(); ++i)
			{
				// 착탄 사유로 종료된 경우에 한해 마지막 포인트만 적색으로 강조
				const bool bIsImpactPoint = bImpactDetected && (i == LastIndex);
				const FColor PointColor = bIsImpactPoint ? DebugHitColor : DebugColor;

				// 매 틱 갱신되므로 LifeTime 0(단발 프레임) 사용
				DrawDebugSphere(World, SamplePoints[i], DebugSphereRadius, 8, PointColor, false, 0.f, 0, 0.f);
			}
		}
	}
#endif

	// PreviewMesh가 지정된 경우에만 동작
	DrawTrajectoryISMC(SamplePoints);
}

void UPRFirePreviewComponent::ClearTrajectory()
{
	ClearTrajectoryISMC();
}

/*~ 히트스캔 미리보기 ~*/

void UPRFirePreviewComponent::UpdateHitScanPreview()
{
	APRWeaponActor* Weapon = WeaponActor.Get();
	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();
	const APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	if (!IsValid(Weapon) || !IsValid(World) || !IsValid(OwnerPawn))
	{
		ClearHitScanPreview();
		return;
	}

	const float TraceDistance = FMath::Max(0.f, GetHitScanPreviewDistance());
	if (TraceDistance <= KINDA_SMALL_NUMBER)
	{
		ClearHitScanPreview();
		return;
	}

	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(OwnerActor);
	IgnoredActors.Add(Weapon);

	// 카메라 조준점 산출 및 Combat 채널 기준 목표 지점 확보
	const FVector AimPoint = UPRGameplayStatics::ResolveCameraAimPoint(
		OwnerPawn, TraceDistance, ECC_Visibility, IgnoredActors);

	const FVector MuzzleLocation = Weapon->GetMuzzleTransform().GetLocation();
	FVector TraceDirection = AimPoint - MuzzleLocation;
	if (!TraceDirection.Normalize(KINDA_SMALL_NUMBER))
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		if (!UPRGameplayStatics::GetPawnViewpoint(OwnerPawn, ViewLocation, ViewRotation))
		{
			ClearHitScanPreview();
			return;
		}

		TraceDirection = ViewRotation.Vector();
	}

	// 경계 오차 보정 및 조준점 바로 앞 종료로 인한 근접 표면 누락 방지
	constexpr float TraceEndPadding = 30.f;
	const FVector TraceEndLocation = AimPoint + TraceDirection * TraceEndPadding;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PRFirePreviewHitScanTrace), false);
	Params.AddIgnoredActors(IgnoredActors);

	FHitResult Hit;
	World->LineTraceSingleByChannel(
		Hit,
		MuzzleLocation,
		TraceEndLocation,
		PRCollisionChannels::ECC_Combat,
		Params);

	
	const bool bHit = Hit.bBlockingHit && IsValid(Hit.GetActor());
	BroadcastPreviewHit(bHit);
}

float UPRFirePreviewComponent::GetHitScanPreviewDistance()
{
	const UPRWeaponManagerComponent* WeaponManager = GetWeaponManager();
	const UPRWeaponDataAsset* WeaponData = IsValid(WeaponManager)
		? WeaponManager->GetCurrentWeaponVisualInfo().WeaponData
		: nullptr;
	if (IsValid(WeaponData))
	{
		return FMath::Max(0.f, WeaponData->MaxFireDistance);
	}

	// PlayerState 무기 데이터 복제 지연 시 기존 기본 거리 유지
	return PRFirePreviewPrivate::DefaultMaxFireDistance;
}

void UPRFirePreviewComponent::ClearHitScanPreview()
{
	if (bHasPreviewHitState && bLastPreviewHit)
	{
		BroadcastPreviewHit(false, true);
	}

	bHasPreviewHitState = false;
	bLastPreviewHit = false;
}

void UPRFirePreviewComponent::BroadcastPreviewHit(bool bHit, bool bForceBroadcast)
{
	if (!bForceBroadcast && bHasPreviewHitState && bLastPreviewHit == bHit)
	{
		return;
	}

	bHasPreviewHitState = true;
	bLastPreviewHit = bHit;

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

	FPRPreviewHitEventPayload Payload;
	Payload.bHit = bHit;

	// UI 직접 참조 회피. UPRCrosshairWidget 구독 후 IPRCrosshairInterface::OnPreviewHit 호출
	EventMgr->BroadcastTyped(PRGameplayTags::Event_Player_PreviewHit, Payload);
}

void UPRFirePreviewComponent::DrawTrajectoryISMC(const TArray<FVector>& SamplePoints)
{
	if (!IsValid(PreviewMesh))
	{
		return;
	}

	if (!IsValid(TrajectoryISMC))
	{
		return;
	}

	// 기존 인스턴스의 Transform만 갱신하고, 수량 차이만 Add/Remove로 보정
	const int32 NewCount = SamplePoints.Num();
	const int32 OldCount = TrajectoryISMC->GetInstanceCount();

	const bool bImpactDetected = LastResult.EndReason == EPRPreviewEndReason::HitBlocking;
	const int32 LastIndex = NewCount - 1;
	const FVector InstanceScale(PreviewMeshScale);

	// 기존 인스턴스 Transform 갱신 (공통 범위)
	const int32 UpdateCount = FMath::Min(OldCount, NewCount);
	for (int32 i = 0; i < UpdateCount; ++i)
	{
		const FTransform InstanceTransform(FQuat::Identity, SamplePoints[i], InstanceScale);
		TrajectoryISMC->UpdateInstanceTransform(i, InstanceTransform, /*bWorldSpace=*/true,
		                                        /*bMarkRenderStateDirty=*/false, /*bTeleport=*/true);

		const float CustomValue = (bImpactDetected && i == LastIndex) ? 1.f : 0.f;
		TrajectoryISMC->SetCustomDataValue(i, 0, CustomValue, /*bMarkRenderStateDirty=*/false);
	}

	// 부족한 만큼 추가
	for (int32 i = OldCount; i < NewCount; ++i)
	{
		const FTransform InstanceTransform(FQuat::Identity, SamplePoints[i], InstanceScale);
		const int32 InstanceIndex = TrajectoryISMC->AddInstance(InstanceTransform, /*bWorldSpace=*/true);

		const float CustomValue = (bImpactDetected && i == LastIndex) ? 1.f : 0.f;
		TrajectoryISMC->SetCustomDataValue(InstanceIndex, 0, CustomValue, /*bMarkRenderStateDirty=*/false);
	}

	// 초과분 제거 (뒤에서부터)
	for (int32 i = OldCount - 1; i >= NewCount; --i)
	{
		TrajectoryISMC->RemoveInstance(i);
	}

	// 모든 인스턴스 갱신 완료 후 렌더 스테이트 1회 갱신
	TrajectoryISMC->MarkRenderStateDirty();
}

void UPRFirePreviewComponent::ClearTrajectoryISMC()
{
	if (IsValid(TrajectoryISMC))
	{
		TrajectoryISMC->ClearInstances();
	}
}

void UPRFirePreviewComponent::OnUnregister()
{
	ClearHitScanPreview();

	if (IsValid(TrajectoryISMC))
	{
		TrajectoryISMC->DestroyComponent();
		TrajectoryISMC = nullptr;
	}
	Super::OnUnregister();
}

/*~ Tick ~*/

void UPRFirePreviewComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (UPRWeaponManagerComponent* WeaponManager = GetWeaponManager())
	{
		WeaponActor = WeaponManager->GetActiveWeaponActor();	
	}

	if (bIsShowing)
	{
		RefreshActivePreviewParams();
	}
	
	// WeaponActor 유효성 재확인. 무기 교체 등으로 사라졌을 수 있음
	if (!WeaponActor.IsValid())
	{
		UE_LOG(LogPRPathPreviewComponent, Verbose,
			TEXT("Tick 경로 생성 생략. Owner=%s, bIsEnabled=%d, bIsShowing=%d, WeaponActor=%s, CachedWeaponManager=%s"),
			*GetNameSafe(GetOwner()),
			bIsTrajectoryEnabled,
			bIsShowing,
			*GetNameSafe(WeaponActor.Get()),
			*GetNameSafe(CachedWeaponManager.Get()));

		if (bIsTrajectoryEnabled)
		{
			ClearTrajectory();
		}
		else
		{
			ClearHitScanPreview();
		}
		return;
	}

	if (bIsTrajectoryEnabled)
	{
		RebuildPath();
		DrawTrajectory(LastResult.SamplePoints);
	}
	else
	{
		UpdateHitScanPreview();
	}
}

/*~ Internal ~*/

bool UPRFirePreviewComponent::TryResolveActivePreviewKey(FPRFirePreviewKey& OutKey)
{
	UPRWeaponManagerComponent* WeaponManager = GetWeaponManager();
	const EPRWeaponSlotType CurrentSlot = IsValid(WeaponManager)
		? WeaponManager->GetCurrentWeaponSlot()
		: EPRWeaponSlotType::None;
	if (CurrentSlot != EPRWeaponSlotType::Primary && CurrentSlot != EPRWeaponSlotType::Secondary)
	{
		return false;
	}

	OutKey.WeaponSlot = CurrentSlot;
	OutKey.FireMode = EPRFirePreviewMode::BaseFire;

	const UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();
	if (IsValid(ASC))
	{
		if (CurrentSlot == EPRWeaponSlotType::Primary
			&& ASC->HasMatchingGameplayTag(PRGameplayTags::State_CurrentWeaponSlot_Primary_Mod))
		{
			OutKey.FireMode = EPRFirePreviewMode::ModFire;
		}
		else if (CurrentSlot == EPRWeaponSlotType::Secondary
			&& ASC->HasMatchingGameplayTag(PRGameplayTags::State_CurrentWeaponSlot_Secondary_Mod))
		{
			OutKey.FireMode = EPRFirePreviewMode::ModFire;
		}
	}

	return OutKey.IsValid();
}

bool UPRFirePreviewComponent::HasProjectilePreviewForKey(const FPRFirePreviewKey& Key) const
{
	return PreviewEntries.Contains(Key);
}

UPRWeaponManagerComponent* UPRFirePreviewComponent::GetWeaponManager()
{
	if (CachedWeaponManager.IsValid())
	{
		return CachedWeaponManager.Get();
	}

	APRPlayerCharacter* Player = Cast<APRPlayerCharacter>(GetOwner());
	if (!IsValid(Player))
	{
		UE_LOG(LogPRPathPreviewComponent, Verbose, TEXT("WeaponManager 캐시 실패. Owner가 PlayerCharacter 아님, Owner=%s"),
			*GetNameSafe(GetOwner()));
		return nullptr;
	}

	CachedWeaponManager = Player->GetWeaponManager();
	if (CachedWeaponManager.IsValid())
	{
		UE_LOG(LogPRPathPreviewComponent, Log, TEXT("WeaponManager 캐시 완료. Owner=%s, WeaponManager=%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(CachedWeaponManager.Get()));
	}
	else
	{
		UE_LOG(LogPRPathPreviewComponent, Verbose, TEXT("WeaponManager 캐시 대기. Owner=%s"),
			*GetNameSafe(GetOwner()));
	}

	return CachedWeaponManager.Get();
}

UAbilitySystemComponent* UPRFirePreviewComponent::GetOwnerAbilitySystemComponent() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}

/*~ Path Build ~*/

void UPRFirePreviewComponent::RebuildPath()
{
	LastResult.SamplePoints.Reset();
	LastResult.EndReason = EPRPreviewEndReason::None;
	LastResult.EndHit = FHitResult();
	LastResult.ElapsedSimTime = 0.f;

	APRWeaponActor* Weapon = WeaponActor.Get();
	UWorld* World = GetWorld();
	if (!IsValid(Weapon) || !IsValid(World))
	{
		return;
	}

	// 무기 머즐 트랜스폼을 시작 기준으로 사용. BP에서 override 가능
	const FTransform MuzzleTransform = Weapon->GetMuzzleTransform();
	const FVector StartLocation = MuzzleTransform.GetLocation();

	// 발사 방향은 카메라 조준점 기준으로 산출하여 실제 발사 흐름(UPRGameplayStatics::ResolveProjectileLaunchTransform)과 정합 유지.
	// 카메라 트레이스 거리는 항상 충분히 길게 잡아 원거리 조준점도 포착(MaxDistance와 별개)
	constexpr float AimTraceDistance = 20000.f;
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	TArray<AActor*> IgnoredActors;
	if (AActor* OwnerActor = GetOwner())
	{
		IgnoredActors.Add(OwnerActor);
	}
	IgnoredActors.Add(Weapon);

	const FTransform LaunchTransform = UPRGameplayStatics::ResolveProjectileLaunchTransform(
		OwnerPawn, StartLocation, AimTraceDistance, FireParams.TraceChannel, IgnoredActors);
	const FVector LaunchVelocity = LaunchTransform.GetRotation().GetForwardVector() * FireParams.InitialSpeed;

	FPredictProjectilePathParams PathParams(FireParams.CollisionRadius, StartLocation, LaunchVelocity,
	                                        FireParams.MaxSimTime);
	PathParams.bTraceWithCollision = true;
	PathParams.bTraceWithChannel = true;
	PathParams.TraceChannel = FireParams.TraceChannel;
	PathParams.SimFrequency = FireParams.SimFrequency;

	// 중력 스케일 적용. GravityScale이 0에 가까운 경우 OverrideGravityZ==0이 "월드 기본 사용"으로 해석되므로 매우 작은 값으로 보정
	const float WorldGravityZ = World->GetGravityZ();
	float OverrideGravityZ = WorldGravityZ * FireParams.GravityScale;
	if (FMath::IsNearlyZero(OverrideGravityZ))
	{
		OverrideGravityZ = KINDA_SMALL_NUMBER;
	}
	PathParams.OverrideGravityZ = OverrideGravityZ;

	// 자기 액터와 무기 액터는 충돌에서 제외하여 발사 직후 머즐 콜리전에 막히는 케이스 방지
	PathParams.ActorsToIgnore = IgnoredActors;

	FPredictProjectilePathResult PathResult;
	const bool bHit = UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	// 다운샘플링: 누적 거리 기준 SampleSpacing 간격으로 포인트 추출. MaxDistance / MaxSamplePoints 도달 시 조기 종료
	const TArray<FPredictProjectilePathPointData>& Path = PathResult.PathData;
	if (Path.Num() == 0)
	{
		LastResult.EndReason = bHit ? EPRPreviewEndReason::HitBlocking : EPRPreviewEndReason::TimeExceeded;
		if (bHit)
		{
			LastResult.EndHit = PathResult.HitResult;
		}
		return;
	}

	const bool bHasDistanceCap = FireParams.MaxDistance > 0.f;
	const bool bHasCountCap = FireParams.MaxSamplePoints > 0;
	const float Spacing = FMath::Max(1.f, FireParams.SampleSpacing);

	// 시작점은 항상 포함
	LastResult.SamplePoints.Add(Path[0].Location);

	float AccumulatedDistance = 0.f;
	float DistanceSinceLastSample = 0.f;
	bool bDistanceCapped = false;
	FVector PrevLocation = Path[0].Location;
	FVector LastSampledLocation = Path[0].Location;

	for (int32 i = 1; i < Path.Num(); ++i)
	{
		const FVector Current = Path[i].Location;
		const float SegmentLength = FVector::Dist(PrevLocation, Current);

		// 거리 제한 도달 시 보간 지점에서 종료
		if (bHasDistanceCap && AccumulatedDistance + SegmentLength > FireParams.MaxDistance)
		{
			const float Remaining = FireParams.MaxDistance - AccumulatedDistance;
			const float Alpha = SegmentLength > KINDA_SMALL_NUMBER ? Remaining / SegmentLength : 0.f;
			const FVector EndPoint = FMath::Lerp(PrevLocation, Current, Alpha);
			LastResult.SamplePoints.Add(EndPoint);
			LastSampledLocation = EndPoint;
			AccumulatedDistance = FireParams.MaxDistance;
			bDistanceCapped = true;
			break;
		}

		AccumulatedDistance += SegmentLength;
		DistanceSinceLastSample += SegmentLength;

		if (DistanceSinceLastSample >= Spacing)
		{
			LastResult.SamplePoints.Add(Current);
			LastSampledLocation = Current;
			DistanceSinceLastSample = 0.f;

			if (bHasCountCap && LastResult.SamplePoints.Num() >= FireParams.MaxSamplePoints)
			{
				break;
			}
		}

		PrevLocation = Current;
	}

	// 끝점이 마지막 샘플과 다르고 거리/개수 제한에 걸리지 않았다면 종료점 추가하여 표시 끊김 방지
	if (!bDistanceCapped)
	{
		const FVector PathEnd = Path.Last().Location;
		const bool bCountCapReached = bHasCountCap && LastResult.SamplePoints.Num() >= FireParams.MaxSamplePoints;
		if (!bCountCapReached && !LastSampledLocation.Equals(PathEnd, KINDA_SMALL_NUMBER))
		{
			LastResult.SamplePoints.Add(PathEnd);
		}
	}

	// 종료 사유 결정
	LastResult.ElapsedSimTime = Path.Last().Time;
	if (bHit && !bDistanceCapped)
	{
		LastResult.EndReason = EPRPreviewEndReason::HitBlocking;
		LastResult.EndHit = PathResult.HitResult;
	}
	else if (bDistanceCapped)
	{
		LastResult.EndReason = EPRPreviewEndReason::DistanceExceeded;
	}
	else
	{
		LastResult.EndReason = EPRPreviewEndReason::TimeExceeded;
	}
}
