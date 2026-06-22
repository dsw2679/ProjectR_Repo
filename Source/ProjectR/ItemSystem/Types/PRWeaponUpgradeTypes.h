// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Weapon 강화/업그레이드 타입 및 구조체 정의)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PRWeaponUpgradeTypes.generated.h"

class UPRItemInstance_Weapon;
class UPRMaterialDataAsset;
class UPRWeaponUpgradeComponent;
class UTexture2D;

// 강화 비용으로 사용할 아이템 수량이다
USTRUCT(BlueprintType)
struct PROJECTR_API FPREconomyItemCost
{
	GENERATED_BODY()

	// 비용으로 요구할 아이템 Primary Asset Id
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	FPrimaryAssetId ItemAssetId;

	// 요구 수량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade", meta = (ClampMin = "1"))
	int32 Quantity = 1;
};

// 강화 비용 묶음이다
USTRUCT(BlueprintType)
struct PROJECTR_API FPREconomyCost
{
	GENERATED_BODY()

	// 요구 고철 수량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade", meta = (ClampMin = "0"))
	int32 Scrap = 0;

	// 요구 재료 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	TArray<FPREconomyItemCost> Items;
};

// 강화 단계별 비용 데이터 Row다
USTRUCT(BlueprintType)
struct PROJECTR_API FPRWeaponUpgradeRow : public FTableRowBase
{
	GENERATED_BODY()

	// 목표 단계까지 올리는 데 필요한 비용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	FPREconomyCost Cost;
};

// 무기 강화 실패 사유다
UENUM(BlueprintType)
enum class EPRWeaponUpgradeFailReason : uint8
{
	None,
	InvalidAuthority,
	InvalidUpgradeStation,
	InvalidInteractor,
	RequestThrottled,
	InvalidWeapon,
	MaxLevel,
	MissingUpgradeData,
	MissingItemData,
	NotEnoughScrap,
	NotEnoughMaterial,
	ConsumeFailed
};

// 무기 강화 처리 결과다
USTRUCT(BlueprintType)
struct PROJECTR_API FPRWeaponUpgradeResult
{
	GENERATED_BODY()

	// 강화 성공 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	bool bSuccess = false;

	// 실패한 경우의 사유
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	EPRWeaponUpgradeFailReason FailReason = EPRWeaponUpgradeFailReason::None;

	// 요청을 처리한 강화 컴포넌트
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UPRWeaponUpgradeComponent> UpgradeComponent = nullptr;

	// 강화 요청 대상 무기
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UPRItemInstance_Weapon> WeaponItem = nullptr;

	// 처리 후 강화 단계
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	int32 UpgradeLevel = 0;
};

// 강화 비용 UI에 표시할 재료 요구량이다
USTRUCT(BlueprintType)
struct PROJECTR_API FPRWeaponUpgradeMaterialCostViewData
{
	GENERATED_BODY()

	// 요구 재료 데이터
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UPRMaterialDataAsset> MaterialData = nullptr;

	// UI에 표시할 재료 이름
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	FText DisplayName;

	// UI에 표시할 재료 아이콘
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// 요구 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	int32 RequiredQuantity = 0;

	// 현재 보유 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	int32 OwnedQuantity = 0;

	// 요구 수량을 충족하는지 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	bool bEnough = false;
};

// 강화 UI에서 표시할 다음 강화 비용과 가능 여부다
USTRUCT(BlueprintType)
struct PROJECTR_API FPRWeaponUpgradePreview
{
	GENERATED_BODY()

	// 유효한 무기가 선택되었는지 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	bool bValidWeapon = false;

	// 선택 무기가 요청 플레이어의 인벤토리에 있는지 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	bool bOwnsWeapon = false;

	// 이미 최대 강화 단계인지 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	bool bMaxLevel = false;

	// 다음 단계 강화 데이터가 존재하는지 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	bool bHasUpgradeData = false;

	// 최종 강화 요청이 가능한지 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	bool bCanUpgrade = false;

	// 현재 강화 단계
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	int32 CurrentLevel = 0;

	// 다음 강화 단계
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	int32 TargetLevel = 0;

	// 현재 강화 단계가 반영된 BaseDamage
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	float CurrentBaseDamage = 0.0f;

	// 다음 강화 단계가 반영될 BaseDamage
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	float TargetBaseDamage = 0.0f;

	// 요구 고철 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	int32 ScrapCost = 0;

	// 현재 보유 고철 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	int32 OwnedScrap = 0;

	// 고철 요구량을 충족하는지 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	bool bEnoughScrap = false;

	// 요구 재료 목록
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	TArray<FPRWeaponUpgradeMaterialCostViewData> MaterialCosts;
};
