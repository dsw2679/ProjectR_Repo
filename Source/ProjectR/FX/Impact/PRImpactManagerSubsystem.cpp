// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (피격 임팩트 매니저 서브시스템 구현)
#include "PRImpactManagerSubsystem.h"

#include "PRImpactPoolActor.h"
#include "PRImpactRegistry.h"
#include "PRImpactSurface.h"
#include "Components/DecalComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "TimerManager.h"

static TAutoConsoleVariable<int32> CVarPRImpactEnablePooling(
	TEXT("pr.Impact.EnablePooling"),
	1,
	TEXT("총기 Impact Component 풀링 사용 여부"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarPRImpactDrawNormal(
	TEXT("pr.Impact.DrawNormal"),
	0,
	TEXT("총기 Impact 위치에서 표면 Normal 방향 디버그 화살표 표시 여부"),
	ECVF_Default);

void UPRImpactManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Registry는 첫 Impact 요청 시점에 DeveloperSettings에서 로드되도록 비워 둠
	CachedRegistry = nullptr;
	PoolActor = nullptr;
}

void UPRImpactManagerSubsystem::Deinitialize()
{
	// 월드 종료 이후 반환 콜백이 죽은 Subsystem이나 Component를 건드리지 않도록 모든 활성 타이머 해제
	if (UWorld* World = GetWorld())
	{
		for (TPair<TWeakObjectPtr<UNiagaraComponent>, FTimerHandle>& TimerPair : ActiveNiagaraTimers)
		{
			World->GetTimerManager().ClearTimer(TimerPair.Value);
		}

		for (TPair<TWeakObjectPtr<UDecalComponent>, FTimerHandle>& TimerPair : ActiveDecalTimers)
		{
			World->GetTimerManager().ClearTimer(TimerPair.Value);
		}
	}

	// 월드가 사라질 때 이전 재생의 Lease와 Registry 캐시가 다음 월드로 넘어가지 않도록 정리
	ActiveNiagaraLeases.Empty();
	ActiveNiagaraTimers.Empty();
	ActiveDecalLeases.Empty();
	ActiveDecalTimers.Empty();
	CachedRegistry = nullptr;

	if (IsValid(PoolActor))
	{
		PoolActor->Destroy();
		PoolActor = nullptr;
	}

	Super::Deinitialize();
}

void UPRImpactManagerSubsystem::PlayImpactFromHit(const FHitResult& HitResult, bool bUseDecal)
{
	// 실제 표면 충돌이 아닌 HitResult로 Impact 이펙트가 생성되지 않도록 차단
	if (!HitResult.bBlockingHit)
	{
		return;
	}

	const FPRImpactContext Context = BuildImpactContext(HitResult);
	const FGameplayTag ImpactTag = ResolveImpactTag(HitResult, Context);
	PlayImpactWithContext(ImpactTag, Context, bUseDecal);
}

void UPRImpactManagerSubsystem::PlayImpactAtLocation(FGameplayTag ImpactTag, FVector ImpactLocation, FVector ImpactNormal, bool bUseDecal)
{
	// 명시 위치 호출은 피격 Actor 문맥이 없으므로 월드 고정 Impact Context만 구성
	FPRImpactContext Context;
	Context.ImpactLocation = ImpactLocation;
	Context.ImpactNormal = ImpactNormal;
	PlayImpactWithContext(ImpactTag, Context, bUseDecal);
}

void UPRImpactManagerSubsystem::PlayImpactWithContext(FGameplayTag ImpactTag, const FPRImpactContext& Context, bool bUseDecal)
{
	// 호출자가 태그를 직접 넘긴 경우에도 Registry와 DefaultImpactTag 검증을 거쳐 재생
	UPRImpactRegistry* Registry = GetRegistry();
	if (!IsValid(Registry))
	{
		UE_LOG(LogPRImpact, Warning, TEXT("Impact Registry가 설정되지 않음"));
		return;
	}

	FGameplayTag ResolvedImpactTag = ImpactTag.IsValid() ? ImpactTag : Registry->DefaultImpactTag;
	FPRImpactDefinition Definition;
	if (!Registry->FindImpactDefinition(ResolvedImpactTag, Definition))
	{
		if (ResolvedImpactTag != Registry->DefaultImpactTag && Registry->DefaultImpactTag.IsValid())
		{
			ResolvedImpactTag = Registry->DefaultImpactTag;
		}
	}

	if (!Registry->FindImpactDefinition(ResolvedImpactTag, Definition))
	{
		UE_LOG(LogPRImpact, Warning, TEXT("Impact Definition 조회 실패. ImpactTag=%s"), *ResolvedImpactTag.ToString());
		return;
	}

	const FTransform ImpactTransform = BuildImpactTransform(Context.ImpactLocation, Context.ImpactNormal);
	DrawImpactNormalDebug(Context.ImpactLocation, Context.ImpactNormal);
	PlayNiagara(Definition, ImpactTransform, Context);
	if (bUseDecal)
	{
		// 호출자가 Decal 생성을 허용한 Impact만 표면 잔여 흔적 재생
		PlayDecal(Definition, ImpactTransform, Context);
	}
}

FGameplayTag UPRImpactManagerSubsystem::ResolveImpactTag(const FHitResult& HitResult, const FPRImpactContext& Context) const
{
	// 약점 콜리전이나 장갑 콜리전처럼 Component가 더 구체적인 표면 의미를 가질 수 있으므로 우선 질의
	if (UPrimitiveComponent* HitComponent = HitResult.GetComponent())
	{
		if (HitComponent->GetClass()->ImplementsInterface(UPRImpactSurface::StaticClass()))
		{
			const FPRImpactResult Result = IPRImpactSurface::Execute_ResolveImpactSurface(HitComponent, Context);
			if (Result.bHasImpactTag && Result.ImpactTag.IsValid())
			{
				return Result.ImpactTag;
			}
		}
	}

	if (AActor* HitActor = HitResult.GetActor())
	{
		if (HitActor->GetClass()->ImplementsInterface(UPRImpactSurface::StaticClass()))
		{
			const FPRImpactResult Result = IPRImpactSurface::Execute_ResolveImpactSurface(HitActor, Context);
			if (Result.bHasImpactTag && Result.ImpactTag.IsValid())
			{
				return Result.ImpactTag;
			}
		}
	}

	return ResolveFallbackImpactTag(HitResult);
}

FTransform UPRImpactManagerSubsystem::BuildImpactTransform(FVector ImpactLocation, FVector ImpactNormal) const
{
	// 비정상 법선 값이 들어와도 Decal과 Niagara가 안정적인 방향을 갖도록 기본 UpVector 보정
	FVector SafeNormal = ImpactNormal.GetSafeNormal();
	if (SafeNormal.IsNearlyZero())
	{
		SafeNormal = FVector::UpVector;
	}

	// Decal 투영 축이 표면 안쪽을 향하도록 표면 법선의 반대 방향을 X축으로 사용
	const FRotator ImpactRotation = FRotationMatrix::MakeFromX(-SafeNormal).Rotator();
	return FTransform(ImpactRotation, ImpactLocation, FVector::OneVector);
}

void UPRImpactManagerSubsystem::DrawImpactNormalDebug(FVector ImpactLocation, FVector ImpactNormal) const
{
	// 런타임 중 Impact 위치와 표면 법선 방향을 눈으로 확인하기 위한 선택적 디버그 표시
	if (CVarPRImpactDrawNormal.GetValueOnGameThread() == 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FVector SafeNormal = ImpactNormal.GetSafeNormal();
	if (SafeNormal.IsNearlyZero())
	{
		SafeNormal = FVector::UpVector;
	}

	static constexpr float DebugArrowLength = 75.0f;
	static constexpr float DebugArrowHeadSize = 12.0f;
	static constexpr float DebugLifeTime = 1.5f;
	static constexpr float DebugThickness = 1.5f;

	const FVector ArrowStart = ImpactLocation;
	const FVector ArrowEnd = ArrowStart + SafeNormal * DebugArrowLength;
	DrawDebugDirectionalArrow(World, ArrowStart, ArrowEnd, DebugArrowHeadSize, FColor::Cyan, false, DebugLifeTime, 0, DebugThickness);
}

FPRImpactNiagaraComponentLease UPRImpactManagerSubsystem::GetNiagaraComponent()
{
	// 호출자는 풀링 여부를 알 필요 없도록 Subsystem 내부에서 풀 슬롯과 임시 Component 경로를 통합
	APRImpactPoolActor* OwnerPoolActor = GetOrCreatePoolActor();
	if (!IsValid(OwnerPoolActor))
	{
		return FPRImpactNiagaraComponentLease();
	}

	if (IsPoolingEnabled())
	{
		return OwnerPoolActor->AcquireNiagaraComponent();
	}

	FPRImpactNiagaraComponentLease Lease;
	Lease.Component = OwnerPoolActor->CreateTransientNiagaraComponent();
	Lease.bPooled = false;
	return Lease;
}

FPRImpactDecalComponentLease UPRImpactManagerSubsystem::GetDecalComponent()
{
	// 호출자는 풀링 여부를 알 필요 없도록 Subsystem 내부에서 풀 슬롯과 임시 Component 경로를 통합
	APRImpactPoolActor* OwnerPoolActor = GetOrCreatePoolActor();
	if (!IsValid(OwnerPoolActor))
	{
		return FPRImpactDecalComponentLease();
	}

	if (IsPoolingEnabled())
	{
		return OwnerPoolActor->AcquireDecalComponent();
	}

	FPRImpactDecalComponentLease Lease;
	Lease.Component = OwnerPoolActor->CreateTransientDecalComponent();
	Lease.bPooled = false;
	return Lease;
}

void UPRImpactManagerSubsystem::ReleaseNiagaraComponent(FPRImpactNiagaraComponentLease Lease)
{
	// 풀링과 임시 생성 경로가 같은 반환 진입점을 사용하도록 Lease의 출처 정보로 분기
	if (!Lease.IsValid())
	{
		return;
	}

	TWeakObjectPtr<UNiagaraComponent> ComponentKey = Lease.Component.Get();
	if (const FPRImpactNiagaraComponentLease* ActiveLease = ActiveNiagaraLeases.Find(ComponentKey))
	{
		// 같은 Component가 이미 다른 Impact 재생에 재사용된 경우 이전 콜백의 반환 요청 무시
		if (ActiveLease->Generation != Lease.Generation || ActiveLease->SlotIndex != Lease.SlotIndex || ActiveLease->bPooled != Lease.bPooled)
		{
			return;
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (FTimerHandle* TimerHandle = ActiveNiagaraTimers.Find(ComponentKey))
		{
			World->GetTimerManager().ClearTimer(*TimerHandle);
		}
	}

	ActiveNiagaraLeases.Remove(ComponentKey);
	ActiveNiagaraTimers.Remove(ComponentKey);

	if (Lease.bPooled)
	{
		if (IsValid(PoolActor))
		{
			PoolActor->ReleaseNiagaraComponent(Lease);
		}
		return;
	}

	// 풀링 비활성화 상태에서 만든 Component는 재사용하지 않으므로 즉시 제거
	Lease.Component->OnSystemFinished.Clear();
	Lease.Component->DeactivateImmediate();
	Lease.Component->DestroyComponent();
}

void UPRImpactManagerSubsystem::ReleaseDecalComponent(FPRImpactDecalComponentLease Lease)
{
	// 풀링과 임시 생성 경로가 같은 반환 진입점을 사용하도록 Lease의 출처 정보로 분기
	if (!Lease.IsValid())
	{
		return;
	}

	TWeakObjectPtr<UDecalComponent> ComponentKey = Lease.Component.Get();
	if (const FPRImpactDecalComponentLease* ActiveLease = ActiveDecalLeases.Find(ComponentKey))
	{
		// 같은 Component가 이미 다른 Impact 표시에 재사용된 경우 이전 타이머의 반환 요청 무시
		if (ActiveLease->Generation != Lease.Generation || ActiveLease->SlotIndex != Lease.SlotIndex || ActiveLease->bPooled != Lease.bPooled)
		{
			return;
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (FTimerHandle* TimerHandle = ActiveDecalTimers.Find(ComponentKey))
		{
			World->GetTimerManager().ClearTimer(*TimerHandle);
		}
	}

	ActiveDecalLeases.Remove(ComponentKey);
	ActiveDecalTimers.Remove(ComponentKey);

	if (Lease.bPooled)
	{
		if (IsValid(PoolActor))
		{
			PoolActor->ReleaseDecalComponent(Lease);
		}
		return;
	}

	// 풀링 비활성화 상태에서 만든 Component는 재사용하지 않으므로 즉시 제거
	Lease.Component->SetVisibility(false, true);
	Lease.Component->DestroyComponent();
}

APRImpactPoolActor* UPRImpactManagerSubsystem::GetOrCreatePoolActor()
{
	// Impact가 실제로 재생되는 월드에서만 Pool Actor가 생기도록 첫 요청 시점까지 생성 지연
	if (IsValid(PoolActor))
	{
		return PoolActor;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PoolActor = World->SpawnActor<APRImpactPoolActor>(APRImpactPoolActor::StaticClass(), FTransform::Identity, SpawnParameters);
	if (!IsValid(PoolActor))
	{
		UE_LOG(LogPRImpact, Warning, TEXT("Impact Pool Actor 생성 실패"));
		return nullptr;
	}

	const UPRImpactRegistry* Registry = GetRegistry();
	const int32 InitialNiagaraCount = IsValid(Registry) ? Registry->InitialImpactNiagaraPoolSize : 0;
	const int32 InitialDecalCount = IsValid(Registry) ? Registry->InitialImpactDecalPoolSize : 0;
	const int32 MaxNiagaraCount = IsValid(Registry) ? Registry->MaxImpactNiagaraPoolSize : -1;
	const int32 MaxDecalCount = IsValid(Registry) ? Registry->MaxImpactDecalPoolSize : -1;
	PoolActor->InitializePool(InitialNiagaraCount, InitialDecalCount, MaxNiagaraCount, MaxDecalCount);

	return PoolActor;
}

bool UPRImpactManagerSubsystem::IsPoolingEnabled() const
{
	// 런타임 성능 비교와 문제 진단을 위해 CVar로 풀링 경로와 임시 생성 경로 전환
	return CVarPRImpactEnablePooling.GetValueOnGameThread() != 0;
}

UPRImpactRegistry* UPRImpactManagerSubsystem::GetRegistry() const
{
	// 같은 월드에서 반복되는 Impact 요청마다 Registry 에셋을 다시 로드하지 않도록 캐시 우선 사용
	if (IsValid(CachedRegistry))
	{
		return CachedRegistry;
	}

	const UPRDeveloperSettings* DeveloperSettings = GetDefault<UPRDeveloperSettings>();
	if (!IsValid(DeveloperSettings))
	{
		return nullptr;
	}

	// 프로젝트 설정의 Impact Registry를 첫 사용 시점에 동기 로드
	CachedRegistry = DeveloperSettings->ImpactRegistry.LoadSynchronous();
	return CachedRegistry;
}

void UPRImpactManagerSubsystem::PlayNiagara(const FPRImpactDefinition& Definition, const FTransform& ImpactTransform, const FPRImpactContext& Context)
{
	UNiagaraSystem* SelectedNiagaraSystem = ChooseNiagaraSystem(Definition);

	// VFX가 필요 없는 Impact Definition은 Niagara Component를 빌리지 않고 종료
	if (!IsValid(SelectedNiagaraSystem))
	{
		return;
	}

	FPRImpactNiagaraComponentLease Lease = GetNiagaraComponent();
	if (!Lease.IsValid())
	{
		UE_LOG(LogPRImpact, Warning, TEXT("Impact Niagara Component 획득 실패"));
		return;
	}

	UNiagaraComponent* NiagaraComponent = Lease.Component;
	TWeakObjectPtr<UNiagaraComponent> ComponentKey = NiagaraComponent;
	if (UWorld* World = GetWorld())
	{
		if (FTimerHandle* ExistingTimerHandle = ActiveNiagaraTimers.Find(ComponentKey))
		{
			// 최대 풀 크기 도달로 오래된 슬롯이 강제 재사용된 경우 이전 안전 반환 타이머 제거
			World->GetTimerManager().ClearTimer(*ExistingTimerHandle);
		}
	}
	ActiveNiagaraLeases.Remove(ComponentKey);
	ActiveNiagaraTimers.Remove(ComponentKey);

	// 이전 재생의 내부 시스템 상태가 잔류하지 않도록 명시적으로 비활성화하고 종료 콜백 해제
	NiagaraComponent->DeactivateImmediate();
	NiagaraComponent->OnSystemFinished.Clear();
	NiagaraComponent->OnSystemFinished.AddUniqueDynamic(this, &UPRImpactManagerSubsystem::HandleNiagaraSystemFinished);

	// SetAsset이 내부적으로 시스템을 재초기화할 때 올바른 Transform을 사용하도록 위치와 부착을 먼저 설정
	NiagaraComponent->SetWorldTransform(ImpactTransform);
	if (IsValid(Context.AttachComponent))
	{
		// Component Outer는 Pool Actor로 유지하고 부착 부모만 피격 Component로 바꿔 피격 대상 이동을 따라가도록 구성
		NiagaraComponent->AttachToComponent(Context.AttachComponent, FAttachmentTransformRules::KeepWorldTransform, Context.AttachSocketName);
	}
	NiagaraComponent->SetHiddenInGame(false, true);
	NiagaraComponent->SetVisibility(true, true);

	// Transform이 확정된 뒤 에셋을 설정하여 내부 초기화 시 파티클 위치 불일치 방지
	NiagaraComponent->SetAsset(SelectedNiagaraSystem);

	ActiveNiagaraLeases.Add(ComponentKey, Lease);

	if (Definition.MaxVFXLifeTime > 0.0f)
	{
		// Niagara 종료 이벤트가 누락되어도 Component가 영구 점유되지 않도록 안전 반환 타이머 등록
		FTimerDelegate ReturnDelegate;
		ReturnDelegate.BindUObject(this, &UPRImpactManagerSubsystem::ReleaseNiagaraComponent, Lease);

		FTimerHandle ReturnTimerHandle;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(ReturnTimerHandle, ReturnDelegate, Definition.MaxVFXLifeTime, false);
			ActiveNiagaraTimers.Add(ComponentKey, ReturnTimerHandle);
		}
	}

	// 에셋과 Transform이 모두 확정된 뒤 단발 재생을 시작하여 초기 파티클이 정확한 위치에서 생성되도록 보장
	NiagaraComponent->Activate(true);
}

void UPRImpactManagerSubsystem::PlayDecal(const FPRImpactDefinition& Definition, const FTransform& ImpactTransform, const FPRImpactContext& Context)
{
	UMaterialInterface* SelectedDecalMaterial = ChooseDecalMaterial(Definition);

	// Decal이 필요 없는 Impact Definition은 Decal Component를 빌리지 않고 종료
	if (!IsValid(SelectedDecalMaterial))
	{
		return;
	}

	FPRImpactDecalComponentLease Lease = GetDecalComponent();
	if (!Lease.IsValid())
	{
		UE_LOG(LogPRImpact, Warning, TEXT("Impact Decal Component 획득 실패"));
		return;
	}

	UDecalComponent* DecalComponent = Lease.Component;
	TWeakObjectPtr<UDecalComponent> ComponentKey = DecalComponent;
	if (UWorld* World = GetWorld())
	{
		if (FTimerHandle* ExistingTimerHandle = ActiveDecalTimers.Find(ComponentKey))
		{
			// 최대 풀 크기 도달로 오래된 슬롯이 강제 재사용된 경우 이전 Decal 반환 타이머 제거
			World->GetTimerManager().ClearTimer(*ExistingTimerHandle);
		}
	}
	ActiveDecalLeases.Remove(ComponentKey);
	ActiveDecalTimers.Remove(ComponentKey);

	ActiveDecalLeases.Add(ComponentKey, Lease);

	// Decal의 X축은 표면에 투영되는 깊이로 사용되므로 너무 작으면 시야 각도에 따라 표면이 투영 볼륨 밖으로 빠질 수 있음
	FVector ResolvedDecalSize = Definition.DecalSize;
	ResolvedDecalSize.X = FMath::Max(ResolvedDecalSize.X, Definition.MinDecalProjectionDepth);

	DecalComponent->SetDecalMaterial(SelectedDecalMaterial);
	DecalComponent->DecalSize = ResolvedDecalSize;
	DecalComponent->FadeScreenSize = Definition.DecalFadeScreenSize;
	DecalComponent->SetWorldTransform(ImpactTransform);
	if (IsValid(Context.AttachComponent))
	{
		// Component Outer는 Pool Actor로 유지하고 부착 부모만 피격 Component로 바꿔 Decal이 피격 대상 이동을 따라가도록 구성
		DecalComponent->AttachToComponent(Context.AttachComponent, FAttachmentTransformRules::KeepWorldTransform, Context.AttachSocketName);
	}
	DecalComponent->SetHiddenInGame(false, true);
	DecalComponent->SetVisibility(true, true);

	if (Definition.DecalLifeTime <= 0.0f)
	{
		// 표시 시간이 없는 Decal은 한 프레임 이상 남기지 않고 즉시 반환
		ReleaseDecalComponent(Lease);
		return;
	}

	FTimerDelegate ReturnDelegate;
	ReturnDelegate.BindUObject(this, &UPRImpactManagerSubsystem::ReleaseDecalComponent, Lease);

	FTimerHandle ReturnTimerHandle;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ReturnTimerHandle, ReturnDelegate, Definition.DecalLifeTime, false);
		ActiveDecalTimers.Add(ComponentKey, ReturnTimerHandle);
	}
}

UNiagaraSystem* UPRImpactManagerSubsystem::ChooseNiagaraSystem(const FPRImpactDefinition& Definition) const
{
	TArray<UNiagaraSystem*> ValidCandidates;
	for (UNiagaraSystem* Candidate : Definition.NiagaraSystems)
	{
		if (IsValid(Candidate))
		{
			ValidCandidates.Add(Candidate);
		}
	}

	if (ValidCandidates.IsEmpty())
	{
		return nullptr;
	}

	// Registry에 여러 후보가 등록된 경우 반복 탄착이 같은 에셋만 사용하지 않도록 무작위 선택
	const int32 CandidateIndex = FMath::RandRange(0, ValidCandidates.Num() - 1);
	return ValidCandidates[CandidateIndex];
}

UMaterialInterface* UPRImpactManagerSubsystem::ChooseDecalMaterial(const FPRImpactDefinition& Definition) const
{
	TArray<UMaterialInterface*> ValidCandidates;
	for (UMaterialInterface* Candidate : Definition.DecalMaterials)
	{
		if (IsValid(Candidate))
		{
			ValidCandidates.Add(Candidate);
		}
	}

	if (ValidCandidates.IsEmpty())
	{
		return nullptr;
	}

	// Registry에 여러 후보가 등록된 경우 반복 탄착이 같은 Decal만 사용하지 않도록 무작위 선택
	const int32 CandidateIndex = FMath::RandRange(0, ValidCandidates.Num() - 1);
	return ValidCandidates[CandidateIndex];
}

FPRImpactContext UPRImpactManagerSubsystem::BuildImpactContext(const FHitResult& HitResult) const
{
	// 인터페이스 질의와 표면 정렬에 필요한 위치와 법선만 HitResult에서 추출
	FPRImpactContext Context;
	Context.ImpactLocation = HitResult.ImpactPoint;
	if (Context.ImpactLocation.IsNearlyZero() && !HitResult.Location.IsNearlyZero())
	{
		Context.ImpactLocation = HitResult.Location;
	}

	Context.ImpactNormal = HitResult.ImpactNormal;
	if (Context.ImpactNormal.IsNearlyZero())
	{
		Context.ImpactNormal = HitResult.Normal;
	}

	if (Context.ImpactNormal.IsNearlyZero())
	{
		Context.ImpactNormal = FVector::UpVector;
	}

	Context.ImpactNormal = Context.ImpactNormal.GetSafeNormal();

	// 실제 피격 Component가 있으면 그 Component에 붙이고, 없으면 Hit Actor RootComponent에 붙여 이동 Actor의 Impact VFX가 함께 움직이도록 구성
	if (UPrimitiveComponent* HitComponent = HitResult.GetComponent())
	{
		Context.AttachComponent = HitComponent;
		Context.AttachSocketName = HitResult.BoneName;
	}
	else if (AActor* HitActor = HitResult.GetActor())
	{
		Context.AttachComponent = HitActor->GetRootComponent();
		Context.AttachSocketName = NAME_None;
	}

	return Context;
}

FGameplayTag UPRImpactManagerSubsystem::ResolveFallbackImpactTag(const FHitResult& HitResult) const
{
	// 인터페이스에서 Impact 태그를 얻지 못한 경우 물리 머티리얼의 SurfaceType으로 Registry 조회
	UPRImpactRegistry* Registry = GetRegistry();
	if (!IsValid(Registry))
	{
		return FGameplayTag();
	}

	EPhysicalSurface SurfaceType = SurfaceType_Default;
	if (UPhysicalMaterial* PhysicalMaterial = HitResult.PhysMaterial.Get())
	{
		SurfaceType = UPhysicalMaterial::DetermineSurfaceType(PhysicalMaterial);
	}

	FGameplayTag ImpactTag;
	if (Registry->FindImpactTagBySurface(SurfaceType, ImpactTag))
	{
		return ImpactTag;
	}

	return Registry->DefaultImpactTag;
}

void UPRImpactManagerSubsystem::HandleNiagaraSystemFinished(UNiagaraComponent* FinishedComponent)
{
	// Niagara 재생 완료 이벤트가 도착하면 활성 Lease 목록에서 반환 정보를 찾아 풀로 복귀
	if (!IsValid(FinishedComponent))
	{
		return;
	}

	const TWeakObjectPtr<UNiagaraComponent> ComponentKey = FinishedComponent;
	if (const FPRImpactNiagaraComponentLease* FoundLease = ActiveNiagaraLeases.Find(ComponentKey))
	{
		ReleaseNiagaraComponent(*FoundLease);
	}
}
