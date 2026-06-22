// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (드롭 상자 메시 및 픽업 보상 알림 UI 데이터 구조 정의)
// Author: 배유찬 (필드 아이템 배치용 드롭 데이터 연동)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PRDropTypes.generated.h"

class UPRItemDataAsset;

UENUM(BlueprintType)
enum class EPRRewardType : uint8
{
	None,
	Currency,
	Item,
	Ammo
};

UENUM(BlueprintType)
enum class EPRRewardDistributionRule : uint8
{
	// 월드 픽업은 Claim한 플레이어 지급, 즉시 지급은 개인 대상 지급
	Personal,

	// 파티 대상 전원 지급
	PartyShared
};

USTRUCT(BlueprintType)
struct FPRDropRewardEntry
{
	GENERATED_BODY()

	// 지급할 보상의 종류
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPRRewardType RewardType = EPRRewardType::None;

	// 보상을 받을 대상 산정 규칙. 월드 픽업 Personal은 Claim자 기준
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPRRewardDistributionRule DistributionRule = EPRRewardDistributionRule::Personal;

	// 아이템 또는 탄약 보상일 때 조회할 Primary Asset Id
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "RewardType == EPRRewardType::Item || RewardType == EPRRewardType::Ammo"))
	FPrimaryAssetId ItemAssetId;

	// 아이템 또는 탄약 보상 최소 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "RewardType == EPRRewardType::Item || RewardType == EPRRewardType::Ammo", ClampMin = "1"))
	int32 MinQuantity = 1;

	// 아이템 또는 탄약 보상 최대 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "RewardType == EPRRewardType::Item || RewardType == EPRRewardType::Ammo", ClampMin = "1"))
	int32 MaxQuantity = 1;

	// 고철 보상 최소 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "RewardType == EPRRewardType::Currency", ClampMin = "0"))
	int32 MinScrap = 0;

	// 고철 보상 최대 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "RewardType == EPRRewardType::Currency", ClampMin = "0"))
	int32 MaxScrap = 0;

	// 보상 발생 확률
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 1.0f;

	// 월드 픽업 액터를 생성할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSpawnPickup = true;
};

USTRUCT(BlueprintType)
struct FPRMonsterDropTableRow : public FTableRowBase
{
	GENERATED_BODY()

	// 처치 시 지급할 고정 경험치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 Experience = 0;

	// 경험치 보상을 받을 대상 산정 규칙
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPRRewardDistributionRule ExperienceDistributionRule = EPRRewardDistributionRule::PartyShared;

	// 몬스터 사망 시 독립 판정할 보상 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPRDropRewardEntry> Rewards;
};

USTRUCT(BlueprintType)
struct FPRMonsterDeathDropRequest
{
	GENERATED_BODY()

	// 사망한 몬스터의 드롭 테이블 식별자
	UPROPERTY()
	FName MonsterId = NAME_None;

	// 사망 처리 중복 방어에 사용할 몬스터 액터
	UPROPERTY()
	TWeakObjectPtr<AActor> DeadMonster;

	// 즉시 개인 지급 대상 산정에 사용할 처치자 컨트롤러
	UPROPERTY()
	TWeakObjectPtr<AController> KillerController;

	// 월드 픽업을 생성할 기준 위치
	UPROPERTY()
	FVector DropLocation = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FPRResolvedDropReward
{
	GENERATED_BODY()

	// 확정된 보상의 종류
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EPRRewardType RewardType = EPRRewardType::None;

	// 확정된 보상의 분배 규칙
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EPRRewardDistributionRule DistributionRule = EPRRewardDistributionRule::Personal;

	// 확정된 아이템 또는 탄약 데이터
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UPRItemDataAsset> ItemData = nullptr;

	// 확정된 아이템 또는 탄약 Primary Asset Id
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FPrimaryAssetId ItemAssetId;

	// 확정된 아이템 또는 탄약 수량
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Quantity = 0;

	// 확정된 고철 수량
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ScrapAmount = 0;

	// 월드 픽업 생성 여부 (true 면 플레이어가 e키를 통한 획득, false면 인벤토리에 직접 추가)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bSpawnPickup = true;
};

USTRUCT(BlueprintType)
struct FPRPickupNotificationPayload
{
	GENERATED_BODY()

	// HUD에 표시할 보상 종류
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EPRRewardType RewardType = EPRRewardType::None;

	// 아이템 또는 탄약 보상일 때 표시 데이터를 조회할 Primary Asset Id
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FPrimaryAssetId ItemAssetId;

	// 아이템 수량, 탄약 수량 또는 고철 획득량
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Quantity = 0;
};
