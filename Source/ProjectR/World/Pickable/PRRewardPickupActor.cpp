// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (보상 획득 시 골드/아이템 드롭 및 HUD 팝업 연동 구현)
// Author: 배유찬 (필드 보상 스포너 및 리스폰 연동 구현)
#include "PRRewardPickupActor.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/Interaction/PRInteractableComponent.h"
#include "ProjectR/Interaction/Actions/PRInteraction_ClaimRewardPickup.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/System/PRAssetManager.h"

APRRewardPickupActor::APRRewardPickupActor()
{
	SetReplicates(true);
	SetReplicateMovement(true);
	SetNetCullDistanceSquared(FMath::Square(5000.f));
	bOnlyRelevantToOwner = false;
	bDisposable = true;

	DropCollision = CreateDefaultSubobject<USphereComponent>(TEXT("DropCollision"));
	DropCollision->InitSphereRadius(18.0f);
	DropCollision->SetCollisionProfileName(TEXT("Projectile"));
	DropCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DropCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	DropCollision->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	DropCollision->SetCollisionResponseToChannel(PRCollisionChannels::ECC_Interactable, ECR_Ignore);
	SetRootComponent(DropCollision);

	InteractionCollision->SetupAttachment(DropCollision);
	InteractionCollision->SetCollisionProfileName(TEXT("Interactable"));
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(PRCollisionChannels::ECC_Ground, ECR_Ignore);

	DropMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("DropMovement"));
	DropMovement->UpdatedComponent = DropCollision;
	DropMovement->InitialSpeed = 0.0f;
	DropMovement->MaxSpeed = 1200.0f;
	DropMovement->ProjectileGravityScale = 1.6f;
	DropMovement->bShouldBounce = true;
	DropMovement->Bounciness = 0.25f;
	DropMovement->Friction = 0.8f;
	DropMovement->BounceVelocityStopSimulatingThreshold = 45.0f;
	DropMovement->bAutoActivate = false;

	RewardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RewardMesh"));
	RewardMesh->SetupAttachment(RootComponent);
	RewardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RewardNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RewardNiagara"));
	RewardNiagara->SetupAttachment(RewardMesh);
	RewardNiagara->SetAutoActivate(false);

	RewardLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RewardLight"));
	RewardLight->SetupAttachment(RewardMesh);
	RewardLight->SetVisibility(false);
	RewardLight->SetIntensity(0.0f);

	UPRInteraction_ClaimRewardPickup* ClaimAction = CreateDefaultSubobject<UPRInteraction_ClaimRewardPickup>(TEXT("ClaimRewardAction"));
	InteractableComponent->InteractionActions.Add(ClaimAction);
}

void APRRewardPickupActor::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(DropMovement))
	{
		DropMovement->OnProjectileStop.AddDynamic(this, &APRRewardPickupActor::HandleDropMovementStopped);
	}

	ApplyDropSettledState();
	RefreshVisual();
	
	if (IsValid(Reward.ItemData))
	{
		bDropSettled = true;
	}
}

void APRRewardPickupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRRewardPickupActor, Reward);
	DOREPLIFETIME(APRRewardPickupActor, bClaimed);
	DOREPLIFETIME(APRRewardPickupActor, bDropSettled);
}

void APRRewardPickupActor::InitializeReward(const FPRResolvedDropReward& InReward)
{
	if (!HasAuthority())
	{
		return;
	}

	Reward = InReward;
	bDropSettled = false;
	bOnlyRelevantToOwner = false;

	RefreshVisual();
	StartDropMotion();
	ForceNetUpdate();
}

void APRRewardPickupActor::SetRewardQuantity(int32 NewQuantity)
{
	if (!HasAuthority())
	{
		return;
	}

	// 부분 획득 후 남은 보상 수량 복제
	Reward.Quantity = FMath::Max(NewQuantity, 0);
	ForceNetUpdate();
}

bool APRRewardPickupActor::CanBeClaimedBy(AActor* Interactor) const
{
	if (!bDropSettled)
	{
		return false;
	}

	if (bClaimed)
	{
		return false;
	}

	if (Reward.DistributionRule != EPRRewardDistributionRule::Personal)
	{
		return true;
	}

	const APRPlayerState* InteractorPlayerState = ResolveInteractorPlayerState(Interactor);
	return IsValid(InteractorPlayerState);
}

void APRRewardPickupActor::MarkClaimed()
{
	if (!HasAuthority())
	{
		return;
	}

	bClaimed = true;
}

void APRRewardPickupActor::OnRep_Reward()
{
	RefreshVisual();
}

void APRRewardPickupActor::OnRep_DropSettled()
{
	ApplyDropSettledState();
}

void APRRewardPickupActor::HandleDropMovementStopped(const FHitResult& ImpactResult)
{
	if (!HasAuthority())
	{
		return;
	}

	if (ImpactResult.bBlockingHit)
	{
		SetActorLocation(ImpactResult.Location);
	}

	bDropSettled = true;
	ApplyDropSettledState();
	ForceNetUpdate();
}

void APRRewardPickupActor::RefreshVisual()
{
	if (Reward.RewardType == EPRRewardType::Currency)
	{
		if (IsValid(RewardMesh))
		{
			RewardMesh->SetStaticMesh(CurrencyMesh.Get());
			RewardMesh->SetRelativeScale3D(CurrencyMeshScale);
		}
		if (IsValid(RewardNiagara))
		{
			RewardNiagara->SetRelativeLocation(FVector::ZeroVector);
		}

		ApplyEffectVisualInfo(CurrencyVisual);
		return;
	}

	if (Reward.RewardType == EPRRewardType::Item || Reward.RewardType == EPRRewardType::Ammo)
	{
		UPRItemDataAsset* ItemData = Reward.ItemData;
		if (!IsValid(ItemData) && Reward.ItemAssetId.IsValid())
		{
			ItemData = UPRAssetManager::Get().GetItemDataByPrimaryAssetId(Reward.ItemAssetId);
		}

		if (!IsValid(ItemData))
		{
			UE_LOG(LogTemp, Warning, TEXT("[RewardPickup] 아이템 보상 데이터가 없어 비주얼을 적용할 수 없음"));
			return;
		}

		if (IsValid(RewardMesh))
		{
			RewardMesh->SetStaticMesh(ItemData->GetPickupMesh());
			RewardMesh->SetRelativeScale3D(ItemData->GetPickupMeshScale());
		}
		if (IsValid(RewardNiagara))
		{
			RewardNiagara->SetRelativeLocation(ItemData->GetRarityNiagaraOffset());
		}

		const FPRRewardPickupEffectVisualInfo* VisualInfo = ItemRarityVisuals.Find(ItemData->GetRarity());
		if (VisualInfo == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RewardPickup] 희귀도에 맞는 아이템 비주얼 없음. Rarity = %d"), static_cast<uint8>(ItemData->GetRarity()));
			DeactivateEffectVisual();
			return;
		}

		ApplyEffectVisualInfo(*VisualInfo);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[RewardPickup] 보상 타입에 맞는 비주얼 없음. RewardType = %d"), static_cast<uint8>(Reward.RewardType));
	DeactivateEffectVisual();
}

void APRRewardPickupActor::ApplyEffectVisualInfo(const FPRRewardPickupEffectVisualInfo& VisualInfo)
{
	if (IsValid(RewardNiagara))
	{
		if (IsValid(VisualInfo.NiagaraEffect.Get()))
		{
			RewardNiagara->SetAsset(VisualInfo.NiagaraEffect.Get());
			if (!VisualInfo.NiagaraColorParameterName.IsNone())
			{
				RewardNiagara->SetVariableLinearColor(VisualInfo.NiagaraColorParameterName, VisualInfo.EffectColor);
			}
			RewardNiagara->Activate(true);
		}
		else
		{
			RewardNiagara->Deactivate();
			RewardNiagara->SetAsset(nullptr);
		}
	}

	if (IsValid(RewardLight))
	{
		RewardLight->SetLightColor(VisualInfo.LightColor);
		RewardLight->SetIntensity(VisualInfo.LightIntensity);
		RewardLight->SetVisibility(VisualInfo.LightIntensity > 0.0f);
	}
}

APRPlayerState* APRRewardPickupActor::ResolveInteractorPlayerState(AActor* Interactor) const
{
	AController* Controller = Cast<AController>(Interactor);
	if (IsValid(Controller))
	{
		return Controller->GetPlayerState<APRPlayerState>();
	}

	APawn* Pawn = Cast<APawn>(Interactor);
	if (IsValid(Pawn))
	{
		Controller = Pawn->GetController();
	}

	return IsValid(Controller) ? Controller->GetPlayerState<APRPlayerState>() : nullptr;
}

void APRRewardPickupActor::StartDropMotion()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValid(DropMovement) || !IsValid(DropCollision))
	{
		bDropSettled = true;
		ApplyDropSettledState();
		ForceNetUpdate();
		return;
	}

	bDropSettled = false;
	ApplyDropSettledState();
	DropMovement->Velocity = BuildDropInitialVelocity();
	DropMovement->Activate(true);
	ForceNetUpdate();
}

FVector APRRewardPickupActor::BuildDropInitialVelocity() const
{
	const float MinHorizontalSpeed = FMath::Min(DropHorizontalSpeedMin, DropHorizontalSpeedMax);
	const float MaxHorizontalSpeed = FMath::Max(DropHorizontalSpeedMin, DropHorizontalSpeedMax);
	const float HorizontalSpeed = FMath::RandRange(MinHorizontalSpeed, MaxHorizontalSpeed);
	const float DirectionYaw = FMath::FRandRange(0.0f, 360.0f);
	const FVector HorizontalDirection = FRotator(0.0f, DirectionYaw, 0.0f).Vector();
	return HorizontalDirection * HorizontalSpeed + FVector::UpVector * DropVerticalSpeed;
}

void APRRewardPickupActor::ApplyDropSettledState()
{
	if (IsValid(DropCollision))
	{
		DropCollision->SetCollisionEnabled(bDropSettled ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
	}

	if (bDropSettled && IsValid(DropMovement))
	{
		DropMovement->StopMovementImmediately();
		DropMovement->Deactivate();
	}
}

void APRRewardPickupActor::DeactivateEffectVisual()
{
	if (IsValid(RewardNiagara))
	{
		RewardNiagara->Deactivate();
		RewardNiagara->SetAsset(nullptr);
	}

	if (IsValid(RewardLight))
	{
		RewardLight->SetIntensity(0.0f);
		RewardLight->SetVisibility(false);
	}
}
