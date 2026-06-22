// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (재화/아이템 드롭 처리 및 픽업 HUD 알림 연동 구현)
// Author: 배유찬 (드롭 확률 테이블 기반 아이템 스포너 배치 시스템 구현)
#include "PRItemDropManagerSubsystem.h"

#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Data/PRAmmoDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance.h"
#include "ProjectR/Player/Components/PRCurrencyComponent.h"
#include "ProjectR/Player/Components/PRPlayerGrowthComponent.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "ProjectR/World/Pickable/PRRewardPickupActor.h"

namespace
{
	constexpr float RewardPickupGroundTraceUpDistance = 200.0f;
	constexpr float RewardPickupGroundTraceDownDistance = 3000.0f;
	constexpr float RewardPickupSpawnHeight = 120.0f;

	// 보상 구조체의 탄약 데이터 조회
	const UPRAmmoDataAsset* ResolveAmmoData(const FPRResolvedDropReward& Reward)
	{
		const UPRAmmoDataAsset* AmmoData = Cast<UPRAmmoDataAsset>(Reward.ItemData);
		if (!IsValid(AmmoData) && Reward.ItemAssetId.IsValid())
		{
			AmmoData = Cast<UPRAmmoDataAsset>(UPRAssetManager::Get().GetItemDataByPrimaryAssetId(Reward.ItemAssetId));
		}

		return AmmoData;
	}

	// 현재 PlayerArray 기준 플레이어 수 조회
	int32 ResolvePlayerCount(UWorld* World)
	{
		const APRGameStateBase* PRGameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
		return IsValid(PRGameState) ? PRGameState->GetPlayerCount() : 1;
	}
}

void UPRItemDropManagerSubsystem::HandleMonsterDied(const FPRMonsterDeathDropRequest& Request)
{
	UWorld* World = GetWorld();
	AActor* DeadMonster = Request.DeadMonster.Get();
	if (!IsValid(World) || !IsValid(DeadMonster) || !DeadMonster->HasAuthority())
	{
		return;
	}

	if (ProcessedDeadMonsters.Contains(DeadMonster))
	{
		return;
	}
	ProcessedDeadMonsters.Add(DeadMonster);

	if (Request.MonsterId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] MonsterId가 없어 드롭을 건너뜀. DeadMonster = %s"), *GetNameSafe(DeadMonster));
		return;
	}

	const FPRMonsterDropTableRow* DropRow = UPRAssetManager::Get().FindMonsterDropRow(Request.MonsterId);
	if (DropRow == nullptr)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Drop][Server] 드롭 Row 없음. MonsterId = %s"), *Request.MonsterId.ToString());
		return;
	}

	GrantExperienceReward(*DropRow, Request);

	for (const FPRDropRewardEntry& Entry : DropRow->Rewards)
	{
		FPRResolvedDropReward Reward;
		if (!ResolveReward(Entry, Reward))
		{
			continue;
		}

		CommitResolvedReward(Reward, Request);
	}
}

bool UPRItemDropManagerSubsystem::ClaimPickup(APRRewardPickupActor* PickupActor, AActor* Interactor)
{
	if (!IsValid(PickupActor) || !PickupActor->HasAuthority() || PickupActor->IsClaimed())
	{
		return false;
	}

	if (!PickupActor->CanBeClaimedBy(Interactor))
	{
		return false;
	}

	if (ClaimedPickups.Contains(PickupActor))
	{
		return false;
	}

	AController* InteractorController = ResolveInteractorController(Interactor);
	TArray<APRPlayerState*> Recipients;
	ResolveRecipients(PickupActor->GetReward().DistributionRule, InteractorController, Recipients);
	if (Recipients.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 픽업 지급 대상 없음. Pickup = %s"), *GetNameSafe(PickupActor));
		return false;
	}

	const FPRResolvedDropReward& PickupReward = PickupActor->GetReward();
	if (PickupReward.RewardType == EPRRewardType::Ammo
		&& PickupReward.DistributionRule == EPRRewardDistributionRule::Personal
		&& Recipients.Num() == 1)
	{
		const FPRAmmoGrantResult GrantResult = GrantAmmoRewardAmountToPlayer(Recipients[0], PickupReward);
		if (GrantResult.GrantedQuantity <= 0)
		{
			return false;
		}

		// 잔여 raw를 백분율로 환산하여 픽업에 저장
		const int32 RemainingRaw = FMath::Max(GrantResult.DesiredQuantity - GrantResult.GrantedQuantity, 0);
		if (RemainingRaw > 0 && GrantResult.DesiredQuantity > 0)
		{
			const int32 RemainingPercent = FMath::CeilToInt(
				static_cast<float>(RemainingRaw) * static_cast<float>(PickupReward.Quantity) / static_cast<float>(GrantResult.DesiredQuantity));
			PickupActor->SetRewardQuantity(FMath::Max(RemainingPercent, 1));
			return true;
		}

		ClaimedPickups.Add(PickupActor);
		PickupActor->MarkClaimed();
		PickupActor->Destroy();
		return true;
	}

	bool bGrantedAny = false;
	for (APRPlayerState* Recipient : Recipients)
	{
		bGrantedAny |= GrantRewardToPlayer(Recipient, PickupReward);
	}

	if (!bGrantedAny)
	{
		return false;
	}

	ClaimedPickups.Add(PickupActor);
	PickupActor->MarkClaimed();
	PickupActor->Destroy();
	return true;
}

APRRewardPickupActor* UPRItemDropManagerSubsystem::SpawnResolvedRewardPickup(const FPRResolvedDropReward& Reward, const FVector& DropLocation, const AActor* IgnoredActor) const
{
	// 외부 시스템용 확정 보상 픽업 생성 경로
	return SpawnRewardPickup(Reward, DropLocation, IgnoredActor);
}

bool UPRItemDropManagerSubsystem::ResolveReward(const FPRDropRewardEntry& Entry, FPRResolvedDropReward& OutReward) const
{
	if (Entry.RewardType == EPRRewardType::None || Entry.DropChance <= 0.0f)
	{
		return false;
	}

	if (FMath::FRand() > Entry.DropChance)
	{
		return false;
	}

	OutReward.RewardType = Entry.RewardType;
	OutReward.DistributionRule = Entry.DistributionRule;
	OutReward.bSpawnPickup = Entry.bSpawnPickup;

	if (Entry.RewardType == EPRRewardType::Currency)
	{
		if (Entry.MinScrap <= 0 || Entry.MaxScrap < Entry.MinScrap)
		{
			return false;
		}

		OutReward.ScrapAmount = FMath::RandRange(Entry.MinScrap, Entry.MaxScrap);
		return OutReward.ScrapAmount > 0;
	}

	if (Entry.RewardType == EPRRewardType::Item || Entry.RewardType == EPRRewardType::Ammo)
	{
		if (!Entry.ItemAssetId.IsValid() || Entry.MinQuantity <= 0 || Entry.MaxQuantity < Entry.MinQuantity)
		{
			return false;
		}

		UPRItemDataAsset* ItemData = UPRAssetManager::Get().GetItemDataByPrimaryAssetId(Entry.ItemAssetId);
		if (!IsValid(ItemData))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 아이템 데이터 조회 실패. ItemAssetId = %s"), *Entry.ItemAssetId.ToString());
			return false;
		}

		if (Entry.RewardType == EPRRewardType::Ammo && !IsValid(Cast<UPRAmmoDataAsset>(ItemData)))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 탄약 데이터 타입 불일치. ItemAssetId = %s"), *Entry.ItemAssetId.ToString());
			return false;
		}

		OutReward.ItemAssetId = Entry.ItemAssetId;
		OutReward.ItemData = ItemData;
		OutReward.Quantity = FMath::RandRange(Entry.MinQuantity, Entry.MaxQuantity);
		if (Entry.RewardType == EPRRewardType::Ammo)
		{
			// 드롭 생성 시점 플레이어 수 기준 탄약 픽업 수량 확정
			OutReward.Quantity = PRAmmoPickupScaling::CalculateAmmoQuantity(OutReward.Quantity, ResolvePlayerCount(GetWorld()));
		}
		return OutReward.Quantity > 0;
	}

	return false;
}

void UPRItemDropManagerSubsystem::CommitResolvedReward(const FPRResolvedDropReward& Reward, const FPRMonsterDeathDropRequest& Request)
{
	if (Reward.bSpawnPickup)
	{
		SpawnRewardPickup(Reward, Request.DropLocation, Request.DeadMonster.Get());
		return;
	}

	TArray<APRPlayerState*> Recipients;
	ResolveRecipients(Reward.DistributionRule, Request.KillerController.Get(), Recipients);
	if (Recipients.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 즉시 지급 대상 없음. MonsterId = %s"), *Request.MonsterId.ToString());
		return;
	}

	for (APRPlayerState* Recipient : Recipients)
	{
		GrantRewardToPlayer(Recipient, Reward);
	}
}

void UPRItemDropManagerSubsystem::GrantExperienceReward(const FPRMonsterDropTableRow& DropRow, const FPRMonsterDeathDropRequest& Request) const
{
	if (DropRow.Experience <= 0)
	{
		return;
	}

	TArray<APRPlayerState*> Recipients;
	ResolveRecipients(DropRow.ExperienceDistributionRule, Request.KillerController.Get(), Recipients);
	if (Recipients.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 경험치 지급 대상 없음. MonsterId = %s"), *Request.MonsterId.ToString());
		return;
	}

	for (APRPlayerState* Recipient : Recipients)
	{
		if (!IsValid(Recipient))
		{
			continue;
		}

		UPRPlayerGrowthComponent* GrowthComponent = Recipient->GetGrowthComponent();
		if (!IsValid(GrowthComponent))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 경험치 지급 실패. GrowthComponent 없음. PlayerState = %s"), *GetNameSafe(Recipient));
			continue;
		}

		GrowthComponent->AddExperience(DropRow.Experience);
	}
}

APRRewardPickupActor* UPRItemDropManagerSubsystem::SpawnRewardPickup(const FPRResolvedDropReward& Reward, const FVector& DropLocation, const AActor* IgnoredActor) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	TSubclassOf<APRRewardPickupActor> PickupClass = APRRewardPickupActor::StaticClass();
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (Settings != nullptr && !Settings->RewardPickupActorClass.IsNull())
	{
		PickupClass = Settings->RewardPickupActorClass.LoadSynchronous();
	}

	if (!IsValid(PickupClass.Get()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] RewardPickupActorClass 로드 실패"));
		return nullptr;
	}

	const FVector SpawnLocation = ResolveRewardPickupSpawnLocation(DropLocation, IgnoredActor);
	APRRewardPickupActor* PickupActor = World->SpawnActor<APRRewardPickupActor>(
		PickupClass,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams);
	if (!IsValid(PickupActor))
	{
		return nullptr;
	}

	PickupActor->InitializeReward(Reward);
	return PickupActor;
}

FVector UPRItemDropManagerSubsystem::ResolveRewardPickupSpawnLocation(const FVector& DropLocation, const AActor* IgnoredActor) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return DropLocation;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRRewardPickupGroundTrace), false);
	if (IsValid(IgnoredActor))
	{
		QueryParams.AddIgnoredActor(IgnoredActor);
	}

	const FVector TraceStart = DropLocation + FVector::UpVector * RewardPickupGroundTraceUpDistance;
	const FVector TraceEnd = DropLocation - FVector::UpVector * RewardPickupGroundTraceDownDistance;

	FHitResult HitResult;
	const bool bHitGround = World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		PRCollisionChannels::ECC_Ground,
		QueryParams);

	const FVector GroundLocation = bHitGround ? HitResult.ImpactPoint : DropLocation;
	return GroundLocation + FVector::UpVector * RewardPickupSpawnHeight;
}

void UPRItemDropManagerSubsystem::ResolveRecipients(EPRRewardDistributionRule DistributionRule, AController* PersonalController, TArray<APRPlayerState*>& OutRecipients) const
{
	OutRecipients.Reset();

	if (DistributionRule == EPRRewardDistributionRule::Personal)
	{
		APRPlayerState* PlayerState = IsValid(PersonalController) ? PersonalController->GetPlayerState<APRPlayerState>() : nullptr;
		if (IsValid(PlayerState))
		{
			OutRecipients.Add(PlayerState);
		}
		return;
	}

	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = IsValid(World) ? World->GetGameState<AGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		return;
	}

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || !PRPlayerState->IsCombatParticipant())
		{
			continue;
		}

		OutRecipients.Add(PRPlayerState);
	}
}

AController* UPRItemDropManagerSubsystem::ResolveInteractorController(AActor* Interactor) const
{
	AController* Controller = Cast<AController>(Interactor);
	if (IsValid(Controller))
	{
		return Controller;
	}

	APawn* Pawn = Cast<APawn>(Interactor);
	if (IsValid(Pawn))
	{
		return Pawn->GetController();
	}

	return nullptr;
}

bool UPRItemDropManagerSubsystem::GrantRewardToPlayer(APRPlayerState* PlayerState, const FPRResolvedDropReward& Reward) const
{
	if (!IsValid(PlayerState) || !PlayerState->HasAuthority())
	{
		return false;
	}

	if (Reward.RewardType == EPRRewardType::Currency)
	{
		UPRCurrencyComponent* CurrencyComponent = PlayerState->GetCurrencyComponent();
		if (!IsValid(CurrencyComponent))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[Drop][Server] 고철 지급 실패. PlayerState = %s | Reason = InvalidCurrencyComponent | ScrapAmount = %d"),
				*GetNameSafe(PlayerState),
				Reward.ScrapAmount);
			return false;
		}

		const bool bGranted = CurrencyComponent->AddScrap(Reward.ScrapAmount);
		if (bGranted)
		{
			NotifyPickupRewardGranted(PlayerState, Reward);
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[Drop][Server] 고철 지급 완료. PlayerState = %s | DistributionRule = %d | ScrapAmount = %d"),
				*GetNameSafe(PlayerState),
				static_cast<uint8>(Reward.DistributionRule),
				Reward.ScrapAmount);
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[Drop][Server] 고철 지급 실패. PlayerState = %s | DistributionRule = %d | ScrapAmount = %d"),
				*GetNameSafe(PlayerState),
				static_cast<uint8>(Reward.DistributionRule),
				Reward.ScrapAmount);
		}
		return bGranted;
	}

	if (Reward.RewardType == EPRRewardType::Ammo)
	{
		return GrantAmmoRewardToPlayer(PlayerState, Reward);
	}

	if (Reward.RewardType == EPRRewardType::Item)
	{
		UPRInventoryComponent* InventoryComponent = PlayerState->GetInventoryComponent();
		UPRItemInstance* AddedItem = IsValid(InventoryComponent) && IsValid(Reward.ItemData)
			? InventoryComponent->AddItem(Reward.ItemData, Reward.Quantity)
			: nullptr;
		const bool bGranted = IsValid(AddedItem);
		if (bGranted)
		{
			NotifyPickupRewardGranted(PlayerState, Reward);
		}
		return bGranted;
	}

	return false;
}

bool UPRItemDropManagerSubsystem::GrantAmmoRewardToPlayer(APRPlayerState* PlayerState, const FPRResolvedDropReward& Reward) const
{
	return GrantAmmoRewardAmountToPlayer(PlayerState, Reward).GrantedQuantity > 0;
}

FPRAmmoGrantResult UPRItemDropManagerSubsystem::GrantAmmoRewardAmountToPlayer(APRPlayerState* PlayerState, const FPRResolvedDropReward& Reward) const
{
	FPRAmmoGrantResult Result;
	if (!IsValid(PlayerState) || !PlayerState->HasAuthority() || Reward.Quantity <= 0)
	{
		return Result;
	}

	const UPRAmmoDataAsset* AmmoData = ResolveAmmoData(Reward);
	if (!IsValid(AmmoData))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 탄약 지급 실패. AmmoData 없음. PlayerState = %s"), *GetNameSafe(PlayerState));
		return Result;
	}

	UPRAbilitySystemComponent* ASC = PlayerState->GetPRAbilitySystemComponent();
	const UPRAttributeSet_Weapon* WeaponSet = IsValid(ASC) ? ASC->GetSet<UPRAttributeSet_Weapon>() : nullptr;
	if (!IsValid(WeaponSet))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 탄약 지급 실패. WeaponSet 없음. PlayerState = %s"), *GetNameSafe(PlayerState));
		return Result;
	}

	const EPRAmmoType AmmoType = AmmoData->GetAmmoType();
	const float CurrentReserveAmmo = WeaponSet->GetReserveAmmoByType(AmmoType);
	const float MaxReserveAmmo = WeaponSet->GetMaxReserveAmmoByType(AmmoType);

	// Quantity를 MaxMagazineAmmo 대비 백분율로 해석하여 실제 획득 목표량 계산
	const float MaxMagazineAmmo = WeaponSet->GetMaxMagazineAmmoByType(AmmoType);
	Result.DesiredQuantity = FMath::FloorToInt(MaxMagazineAmmo * static_cast<float>(Reward.Quantity) / 100.0f);
	if (Result.DesiredQuantity <= 0)
	{
		return Result;
	}

	const float GrantedAmmo = FMath::Min(static_cast<float>(Result.DesiredQuantity), FMath::Max(MaxReserveAmmo - CurrentReserveAmmo, 0.0f));
	Result.GrantedQuantity = FMath::Clamp(FMath::RoundToInt(GrantedAmmo), 0, Result.DesiredQuantity);
	if (Result.GrantedQuantity <= 0)
	{
		return Result;
	}

	const FGameplayAttribute ReserveAmmoAttribute = UPRAttributeSet_Weapon::GetReserveAmmoAttribute(AmmoType);
	ASC->SetNumericAttributeBase(ReserveAmmoAttribute, CurrentReserveAmmo + static_cast<float>(Result.GrantedQuantity));

	// 알림에는 실제 지급된 탄약 개수를 전달
	FPRResolvedDropReward GrantedReward = Reward;
	GrantedReward.Quantity = Result.GrantedQuantity;
	NotifyPickupRewardGranted(PlayerState, GrantedReward);
	return Result;
}

void UPRItemDropManagerSubsystem::NotifyPickupRewardGranted(APRPlayerState* PlayerState, const FPRResolvedDropReward& Reward) const
{
	APRPlayerController* PlayerController = IsValid(PlayerState)
		? Cast<APRPlayerController>(PlayerState->GetOwner())
		: nullptr;
	if (!IsValid(PlayerController))
	{
		return;
	}

	FPRPickupNotificationPayload Payload;
	Payload.RewardType = Reward.RewardType;
	Payload.ItemAssetId = Reward.ItemAssetId;
	Payload.Quantity = Reward.RewardType == EPRRewardType::Currency ? Reward.ScrapAmount : Reward.Quantity;
	if (Payload.Quantity <= 0)
	{
		return;
	}

	PlayerController->ClientNotifyPickupReward(Payload);
}
