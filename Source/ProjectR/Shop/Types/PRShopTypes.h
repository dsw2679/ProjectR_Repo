// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (재화/재료 복합 거래 타입 정의)
// Author: 배유찬 (기본 상점 거래 분류 타입 정의)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectR/UI/Inventory/PRInventoryUITypes.h"
#include "PRShopTypes.generated.h"

class UPRMaterialDataAsset;
class UPRShopComponent;
class UTexture2D;

// 상점 거래 유형
UENUM(BlueprintType)
enum class EPRShopTransactionType : uint8
{
	Buy,
	Sell
};

// 상점 구매 실패 사유
UENUM(BlueprintType)
enum class EPRShopBuyFailReason : uint8
{
	None,
	InvalidAuthority,
	RequestThrottled,
	InvalidBuyer,
	InvalidShopData,
	InvalidEntry,
	NotForSale,
	MissingItemData,
	Locked,
	OutOfStock,
	NotEnoughScrap,
	NotEnoughMaterial,
	InventoryFull,
	ConsumeFailed,
	GrantFailed
};

// 상점 판매 실패 사유
UENUM(BlueprintType)
enum class EPRShopSellFailReason : uint8
{
	None,
	InvalidAuthority,
	RequestThrottled,
	InvalidSeller,
	InvalidShopData,
	InvalidEntry,
	NotBuyableByShop,
	MissingItemData,
	UnsupportedItemType,
	InvalidQuantity,
	NotEnoughItem,
	MissingPriceData,
	RemoveFailed,
	GrantScrapFailed
};

// 상점 UI 탭 유형
UENUM(BlueprintType)
enum class EPRShopTabType : uint8
{
	Buy,
	Sell
};

// 상점 구매 비용으로 요구하는 재료 수량
USTRUCT(BlueprintType)
struct PROJECTR_API FPRShopMaterialCost
{
	GENERATED_BODY()

	// 구매 비용으로 요구하는 재료 Primary Asset Id
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Shop", meta = (AllowedTypes = "PRMaterialDataAsset"))
	FPrimaryAssetId MaterialAssetId;

	// 요구 수량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Shop", meta = (ClampMin = "1"))
	int32 Quantity = 1;
};

// 상점 데이터 에셋에 저장되는 품목 거래 조건
USTRUCT(BlueprintType)
struct PROJECTR_API FPRShopEntry
{
	GENERATED_BODY()

	// 상점 거래 요청에 사용할 Entry 식별자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Shop")
	FName EntryId = NAME_None;

	// 거래 대상 아이템 Primary Asset Id
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Shop")
	FPrimaryAssetId ItemAssetId;

	// 한 번의 거래로 지급하거나 제거할 아이템 수량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Shop", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	// 상점 기준 고철 가격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Shop", meta = (ClampMin = "0"))
	int32 BaseScrapPrice = 0;

	// 플레이어 구매 시 추가로 요구하는 재료 비용 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Shop")
	TArray<FPRShopMaterialCost> BuyMaterialCosts;

	// 상점이 플레이어에게 판매할 수 있는지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Shop")
	bool bCanShopSellToPlayer = true;

	// 플레이어가 상점에 판매할 수 있는지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Shop")
	bool bCanPlayerSellToShop = false;

	// 유한 재고일 때 초기 재고 수량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Shop", meta = (ClampMin = "0"))
	int32 MaxStock = 0;

	// 재고를 차감하지 않는지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Shop")
	bool bUnlimitedStock = true;

	// 해금에 필요한 진행 태그 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Shop")
	FGameplayTagContainer UnlockRequiredTags;
};

// 유한 재고 상점의 런타임 재고 상태
USTRUCT(BlueprintType)
struct PROJECTR_API FPRShopRuntimeStock
{
	GENERATED_BODY()

	// 재고가 연결된 상점 Entry 식별자
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	FName EntryId = NAME_None;

	// 현재 남은 재고 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	int32 RemainingStock = 0;
};

// 상점 UI에 표시할 재료 비용 데이터
USTRUCT(BlueprintType)
struct PROJECTR_API FPRShopMaterialCostViewData
{
	GENERATED_BODY()

	// 요구 재료 데이터
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	TObjectPtr<UPRMaterialDataAsset> MaterialData = nullptr;

	// UI 표시 이름
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	FText DisplayName;

	// UI 표시 아이콘
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// 요구 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	int32 RequiredQuantity = 0;

	// 현재 보유 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	int32 OwnedQuantity = 0;

	// 요구 수량 충족 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	bool bEnough = false;
};

// 상점 UI 전용 슬롯 표시 데이터
USTRUCT(BlueprintType)
struct PROJECTR_API FPRShopItemSlotViewData
{
	GENERATED_BODY()

	// 이름, 아이콘, 타입, 보유 수량 같은 기본 아이템 표시 데이터
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	FPRInventoryItemSlotViewData ItemViewData;

	// 거래 요청에 사용할 Entry 식별자
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	FName EntryId = NAME_None;

	// 이 슬롯이 표시되는 상점 탭 유형
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	EPRShopTabType TabType = EPRShopTabType::Buy;

	// 단위 거래 고철 가격
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	int32 UnitScrapPrice = 0;

	// 현재 보유 고철 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	int32 OwnedScrap = 0;

	// 고철 가격 충족 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	bool bEnoughScrap = true;

	// 구매 탭에서 요구하는 재료 비용 목록
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	TArray<FPRShopMaterialCostViewData> MaterialCosts;

	// 유한 재고일 때 남은 재고 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	int32 RemainingStock = 0;

	// 재고를 차감하지 않는지 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	bool bUnlimitedStock = true;

	// UI 입력상 거래 요청이 가능한지 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	bool bCanRequestTransaction = false;

	// 상점 목록 선택 표시 상태
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	bool bSelected = false;
};

// 상점 구매 처리 결과
USTRUCT(BlueprintType)
struct PROJECTR_API FPRShopBuyResult
{
	GENERATED_BODY()

	// 구매 성공 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	bool bSuccess = false;

	// 실패한 경우의 사유
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	EPRShopBuyFailReason FailReason = EPRShopBuyFailReason::None;

	// 요청을 처리한 상점 컴포넌트
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	TObjectPtr<UPRShopComponent> ShopComponent = nullptr;

	// 요청한 Entry 식별자
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	FName EntryId = NAME_None;

	// 요청한 거래 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	int32 Quantity = 0;
};

// 상점 판매 처리 결과
USTRUCT(BlueprintType)
struct PROJECTR_API FPRShopSellResult
{
	GENERATED_BODY()

	// 판매 성공 여부
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	bool bSuccess = false;

	// 실패한 경우의 사유
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	EPRShopSellFailReason FailReason = EPRShopSellFailReason::None;

	// 요청을 처리한 상점 컴포넌트
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	TObjectPtr<UPRShopComponent> ShopComponent = nullptr;

	// 요청한 Entry 식별자
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	FName EntryId = NAME_None;

	// 요청한 거래 수량
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Shop")
	int32 Quantity = 0;
};
