// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (피격 임팩트 Pool Actor 구현)
#include "PRImpactPoolActor.h"

#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"

APRImpactPoolActor::APRImpactPoolActor()
{
	// 풀 호스트는 직접 Tick이나 피해 판정을 갖지 않고 Component 보관용 루트만 유지
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void APRImpactPoolActor::InitializePool(int32 InitialNiagaraCount, int32 InitialDecalCount, int32 MaxNiagaraCount, int32 MaxDecalCount)
{
	// MaxSize가 0 이하이면 제한 없이 확장하고 1 이상이면 해당 수량까지만 슬롯 생성
	MaxNiagaraPoolSize = MaxNiagaraCount <= 0 ? -1 : MaxNiagaraCount;
	MaxDecalPoolSize = MaxDecalCount <= 0 ? -1 : MaxDecalCount;

	// 초기 생성 수량이 최대 수량보다 큰 경우 최대 수량까지만 미리 생성
	const int32 SafeNiagaraCount = MaxNiagaraPoolSize < 0 ? FMath::Max(0, InitialNiagaraCount) : FMath::Clamp(InitialNiagaraCount, 0, MaxNiagaraPoolSize);
	const int32 SafeDecalCount = MaxDecalPoolSize < 0 ? FMath::Max(0, InitialDecalCount) : FMath::Clamp(InitialDecalCount, 0, MaxDecalPoolSize);

	for (int32 Index = NiagaraSlots.Num(); Index < SafeNiagaraCount; ++Index)
	{
		FPRImpactNiagaraSlot& Slot = NiagaraSlots.AddDefaulted_GetRef();
		Slot.Component = CreatePooledNiagaraComponent();
	}

	for (int32 Index = DecalSlots.Num(); Index < SafeDecalCount; ++Index)
	{
		FPRImpactDecalSlot& Slot = DecalSlots.AddDefaulted_GetRef();
		Slot.Component = CreatePooledDecalComponent();
	}
}

FPRImpactNiagaraComponentLease APRImpactPoolActor::AcquireNiagaraComponent()
{
	// 비어 있는 Niagara 슬롯을 재생 중 상태로 바꾸고 반환 검증용 Lease 구성
	FPRImpactNiagaraComponentLease Lease;
	const int32 SlotIndex = AcquireNiagaraSlot();
	if (!NiagaraSlots.IsValidIndex(SlotIndex))
	{
		return Lease;
	}

	FPRImpactNiagaraSlot& Slot = NiagaraSlots[SlotIndex];
	if (!IsValid(Slot.Component))
	{
		Slot.Component = CreatePooledNiagaraComponent();
	}

	if (!IsValid(Slot.Component))
	{
		return Lease;
	}

	// 같은 슬롯을 이전에 사용하던 재생의 안전 반환 타이머가 남아 있을 가능성 제거
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Slot.ReturnTimerHandle);
	}

	Slot.bInUse = true;
	++Slot.Generation;
	Slot.LastAcquireSequence = NextAcquireSequence++;
	if (NextAcquireSequence <= 0)
	{
		NextAcquireSequence = 1;
	}

	Lease.Component = Slot.Component;
	Lease.bPooled = true;
	Lease.SlotIndex = SlotIndex;
	Lease.Generation = Slot.Generation;
	return Lease;
}

FPRImpactDecalComponentLease APRImpactPoolActor::AcquireDecalComponent()
{
	// 비어 있는 Decal 슬롯을 표시 중 상태로 바꾸고 반환 검증용 Lease 구성
	FPRImpactDecalComponentLease Lease;
	const int32 SlotIndex = AcquireDecalSlot();
	if (!DecalSlots.IsValidIndex(SlotIndex))
	{
		return Lease;
	}

	FPRImpactDecalSlot& Slot = DecalSlots[SlotIndex];
	if (!IsValid(Slot.Component))
	{
		Slot.Component = CreatePooledDecalComponent();
	}

	if (!IsValid(Slot.Component))
	{
		return Lease;
	}

	// 같은 슬롯을 이전에 사용하던 DecalLifeTime 타이머가 남아 있을 가능성 제거
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Slot.ReturnTimerHandle);
	}

	Slot.bInUse = true;
	++Slot.Generation;
	Slot.LastAcquireSequence = NextAcquireSequence++;
	if (NextAcquireSequence <= 0)
	{
		NextAcquireSequence = 1;
	}

	Lease.Component = Slot.Component;
	Lease.bPooled = true;
	Lease.SlotIndex = SlotIndex;
	Lease.Generation = Slot.Generation;
	return Lease;
}

void APRImpactPoolActor::ReleaseNiagaraComponent(FPRImpactNiagaraComponentLease Lease)
{
	// 슬롯 번호나 풀 여부가 맞지 않는 Lease는 현재 풀의 Component를 건드리지 않도록 차단
	if (!Lease.bPooled || !NiagaraSlots.IsValidIndex(Lease.SlotIndex))
	{
		return;
	}

	FPRImpactNiagaraSlot& Slot = NiagaraSlots[Lease.SlotIndex];
	if (Slot.Generation != Lease.Generation)
	{
		return;
	}

	// 현재 세대가 맞는 Lease만 다음 재생을 위해 Component와 타이머 상태 초기화
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Slot.ReturnTimerHandle);
	}

	ResetNiagaraComponent(Slot.Component);
	Slot.bInUse = false;
}

void APRImpactPoolActor::ReleaseDecalComponent(FPRImpactDecalComponentLease Lease)
{
	// 슬롯 번호나 풀 여부가 맞지 않는 Lease는 현재 풀의 Component를 건드리지 않도록 차단
	if (!Lease.bPooled || !DecalSlots.IsValidIndex(Lease.SlotIndex))
	{
		return;
	}

	FPRImpactDecalSlot& Slot = DecalSlots[Lease.SlotIndex];
	if (Slot.Generation != Lease.Generation)
	{
		return;
	}

	// 현재 세대가 맞는 Lease만 다음 표시를 위해 Component와 타이머 상태 초기화
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Slot.ReturnTimerHandle);
	}

	ResetDecalComponent(Slot.Component);
	Slot.bInUse = false;
}

UNiagaraComponent* APRImpactPoolActor::CreateTransientNiagaraComponent()
{
	// 풀링 비활성화 시에도 같은 생성 경로를 쓰되 슬롯 배열에는 등록하지 않음
	return CreatePooledNiagaraComponent();
}

UDecalComponent* APRImpactPoolActor::CreateTransientDecalComponent()
{
	// 풀링 비활성화 시에도 같은 생성 경로를 쓰되 슬롯 배열에는 등록하지 않음
	return CreatePooledDecalComponent();
}

int32 APRImpactPoolActor::AcquireNiagaraSlot()
{
	// 이미 만들어 둔 Niagara 슬롯 중 사용 중이 아닌 항목을 우선 선택
	for (int32 Index = 0; Index < NiagaraSlots.Num(); ++Index)
	{
		if (!NiagaraSlots[Index].bInUse)
		{
			return Index;
		}
	}

	if (MaxNiagaraPoolSize >= 0 && NiagaraSlots.Num() >= MaxNiagaraPoolSize)
	{
		int32 OldestSlotIndex = INDEX_NONE;
		int32 OldestAcquireSequence = TNumericLimits<int32>::Max();
		for (int32 Index = 0; Index < NiagaraSlots.Num(); ++Index)
		{
			if (NiagaraSlots[Index].bInUse && NiagaraSlots[Index].LastAcquireSequence < OldestAcquireSequence)
			{
				OldestSlotIndex = Index;
				OldestAcquireSequence = NiagaraSlots[Index].LastAcquireSequence;
			}
		}

		if (NiagaraSlots.IsValidIndex(OldestSlotIndex))
		{
			// 최대 크기에 도달한 경우 가장 오래 사용 중인 슬롯을 강제로 초기화한 뒤 재사용
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(NiagaraSlots[OldestSlotIndex].ReturnTimerHandle);
			}
			ResetNiagaraComponent(NiagaraSlots[OldestSlotIndex].Component);
			NiagaraSlots[OldestSlotIndex].bInUse = false;
			return OldestSlotIndex;
		}

		return INDEX_NONE;
	}

	FPRImpactNiagaraSlot& NewSlot = NiagaraSlots.AddDefaulted_GetRef();
	NewSlot.Component = CreatePooledNiagaraComponent();
	return NiagaraSlots.Num() - 1;
}

int32 APRImpactPoolActor::AcquireDecalSlot()
{
	// 이미 만들어 둔 Decal 슬롯 중 사용 중이 아닌 항목을 우선 선택
	for (int32 Index = 0; Index < DecalSlots.Num(); ++Index)
	{
		if (!DecalSlots[Index].bInUse)
		{
			return Index;
		}
	}

	if (MaxDecalPoolSize >= 0 && DecalSlots.Num() >= MaxDecalPoolSize)
	{
		int32 OldestSlotIndex = INDEX_NONE;
		int32 OldestAcquireSequence = TNumericLimits<int32>::Max();
		for (int32 Index = 0; Index < DecalSlots.Num(); ++Index)
		{
			if (DecalSlots[Index].bInUse && DecalSlots[Index].LastAcquireSequence < OldestAcquireSequence)
			{
				OldestSlotIndex = Index;
				OldestAcquireSequence = DecalSlots[Index].LastAcquireSequence;
			}
		}

		if (DecalSlots.IsValidIndex(OldestSlotIndex))
		{
			// 최대 크기에 도달한 경우 가장 오래 사용 중인 슬롯을 강제로 초기화한 뒤 재사용
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(DecalSlots[OldestSlotIndex].ReturnTimerHandle);
			}
			ResetDecalComponent(DecalSlots[OldestSlotIndex].Component);
			DecalSlots[OldestSlotIndex].bInUse = false;
			return OldestSlotIndex;
		}

		return INDEX_NONE;
	}

	FPRImpactDecalSlot& NewSlot = DecalSlots.AddDefaulted_GetRef();
	NewSlot.Component = CreatePooledDecalComponent();
	return DecalSlots.Num() - 1;
}

UNiagaraComponent* APRImpactPoolActor::CreatePooledNiagaraComponent()
{
	UNiagaraComponent* Component = NewObject<UNiagaraComponent>(this);
	if (!IsValid(Component))
	{
		return nullptr;
	}

	// 월드에서 렌더링 가능한 런타임 Component로 등록하고 자동 파괴는 풀 정책에서만 처리
	AddInstanceComponent(Component);
	Component->SetupAttachment(SceneRoot);
	Component->SetAutoActivate(false);
	Component->SetAutoDestroy(false);
	Component->RegisterComponent();
	ResetNiagaraComponent(Component);
	return Component;
}

UDecalComponent* APRImpactPoolActor::CreatePooledDecalComponent()
{
	UDecalComponent* Component = NewObject<UDecalComponent>(this);
	if (!IsValid(Component))
	{
		return nullptr;
	}

	// 월드에서 렌더링 가능한 런타임 Component로 등록하고 표시 상태는 반환 초기화에서 통제
	AddInstanceComponent(Component);
	Component->SetupAttachment(SceneRoot);
	Component->RegisterComponent();
	ResetDecalComponent(Component);
	return Component;
}

void APRImpactPoolActor::ResetNiagaraComponent(UNiagaraComponent* Component)
{
	if (!IsValid(Component))
	{
		return;
	}

	// 다음 Impact가 이전 Niagara 에셋, 종료 델리게이트, Transform을 물려받지 않도록 반환 상태 구성
	Component->OnSystemFinished.Clear();
	Component->DeactivateImmediate();
	Component->SetAsset(nullptr);
	Component->SetVisibility(false, true);
	Component->SetHiddenInGame(true, true);
	Component->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
	Component->SetRelativeTransform(FTransform::Identity);
}

void APRImpactPoolActor::ResetDecalComponent(UDecalComponent* Component)
{
	if (!IsValid(Component))
	{
		return;
	}

	// 다음 Impact가 이전 Decal 머티리얼, 크기, Transform을 물려받지 않도록 반환 상태 구성
	Component->SetVisibility(false, true);
	Component->SetHiddenInGame(true, true);
	Component->SetDecalMaterial(nullptr);
	Component->DecalSize = FVector::ZeroVector;
	Component->FadeScreenSize = 0.0f;
	Component->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
	Component->SetRelativeTransform(FTransform::Identity);
}
