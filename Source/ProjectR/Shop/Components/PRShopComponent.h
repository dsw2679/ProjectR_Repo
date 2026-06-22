// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (상점 거래 비용/재료 유효성 검사 구현)
// Author: 배유찬 (장비 및 소모품 거래 관련 상점 기초 로직 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/Shop/Types/PRShopTypes.h"
#include "PRShopComponent.generated.h"

class APRPlayerController;
class APRPlayerState;
class UPRCurrencyComponent;
class UPRInventoryComponent;
class UPRItemDataAsset;
class UPRMaterialDataAsset;
class UPRShopDataAsset;

// NPC 또는 터미널이 제공하는 상점 거래를 서버 권위로 처리
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRShopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 상점 컴포넌트 기본값을 초기화
	UPRShopComponent();

	/*~ UActorComponent Interface ~*/
	// 서버에서 런타임 재고를 초기화
	virtual void BeginPlay() override;

	// 런타임 재고 복제 대상을 등록
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 요청 플레이어의 구매 시도를 처리
	UFUNCTION(BlueprintAuthorityOnly, Category = "ProjectR|Shop")
	FPRShopBuyResult RequestBuyItem(APRPlayerController* RequestingController, FName EntryId, int32 Quantity);

	// 요청 플레이어의 판매 시도를 처리
	UFUNCTION(BlueprintAuthorityOnly, Category = "ProjectR|Shop")
	FPRShopSellResult RequestSellItem(APRPlayerController* RequestingController, FName EntryId, int32 Quantity);

	// 상점 데이터 에셋을 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Shop")
	UPRShopDataAsset* GetShopData() const { return ShopData; }

	// 상점 표시 이름 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Shop")
	FText GetShopDisplayName() const;

	// 상점 Entry 목록 반환
	const TArray<FPRShopEntry>& GetShopEntries() const;

	// 지정 Entry의 현재 남은 재고를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Shop")
	int32 GetRemainingStock(FName EntryId) const;

	// 지정 Entry가 무한 재고인지 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Shop")
	bool IsUnlimitedStock(FName EntryId) const;

	// 플레이어가 상점에 판매할 때 받을 단위 가격을 계산
	UFUNCTION(BlueprintPure, Category = "ProjectR|Shop")
	int32 CalculateSellScrapPrice(const FPRShopEntry& Entry) const;

protected:
	// 상점 데이터에서 Entry를 찾음
	const FPRShopEntry* FindEntry(FName EntryId) const;

	// Entry가 가리키는 아이템 데이터를 조회
	UPRItemDataAsset* ResolveItemData(const FPRShopEntry& Entry) const;

	// 요청 플레이어가 이 상점과 거래 가능한지 확인
	bool CanRequestShop(APRPlayerController* RequestingController) const;

	// 요청자의 PlayerState를 조회
	APRPlayerState* ResolvePlayerState(APRPlayerController* RequestingController) const;

	// 요청자의 인벤토리 컴포넌트를 조회
	UPRInventoryComponent* ResolveInventoryComponent(APRPlayerController* RequestingController) const;

	// 요청자의 재화 컴포넌트를 조회
	UPRCurrencyComponent* ResolveCurrencyComponent(APRPlayerController* RequestingController) const;

	// 요청 플레이어의 거래 처리 락을 잡음
	bool BeginShopTransaction(APRPlayerState* Trader);

	// 요청 플레이어의 거래 처리 락을 해제
	void EndShopTransaction(APRPlayerState* Trader);

	// 실패한 구매 결과를 생성하고 요청자에게 알림
	FPRShopBuyResult MakeBuyFailureResult(APRPlayerController* RequestingController, FName EntryId, int32 Quantity, EPRShopBuyFailReason FailReason);

	// 성공한 구매 결과를 생성하고 요청자에게 알림
	FPRShopBuyResult MakeBuySuccessResult(APRPlayerController* RequestingController, FName EntryId, int32 Quantity);

	// 실패한 판매 결과를 생성하고 요청자에게 알림
	FPRShopSellResult MakeSellFailureResult(APRPlayerController* RequestingController, FName EntryId, int32 Quantity, EPRShopSellFailReason FailReason);

	// 성공한 판매 결과를 생성하고 요청자에게 알림
	FPRShopSellResult MakeSellSuccessResult(APRPlayerController* RequestingController, FName EntryId, int32 Quantity);

private:
	// 상점 데이터 기준으로 유한 재고를 초기화
	void InitializeRuntimeStocks();

	// 유한 재고 Entry의 런타임 재고를 찾음
	FPRShopRuntimeStock* FindRuntimeStock(FName EntryId);

	// 유한 재고 Entry의 런타임 재고를 찾음
	const FPRShopRuntimeStock* FindRuntimeStock(FName EntryId) const;

	// 구매 수량만큼 재고가 남아있는지 확인
	bool HasEnoughStock(const FPRShopEntry& Entry, int32 Quantity) const;

	// 구매 수량만큼 인벤토리에 지급 가능한지 확인
	bool CanGrantItem(UPRInventoryComponent* InventoryComponent, UPRItemDataAsset* ItemData, int32 TotalItemQuantity) const;

	// 구매 비용 재료 데이터를 조회하고 총 요구량을 계산
	bool ResolveBuyMaterialCosts(const FPRShopEntry& Entry, int32 TransactionQuantity, TMap<UPRMaterialDataAsset*, int32>& OutMaterialCosts) const;

	// 구매 비용 재료 보유량 충족 여부 확인
	bool HasEnoughBuyMaterialCosts(UPRInventoryComponent* InventoryComponent, const TMap<UPRMaterialDataAsset*, int32>& MaterialCosts) const;

	// 구매 비용 재료 소비
	bool ConsumeBuyMaterialCosts(UPRInventoryComponent* InventoryComponent, const TMap<UPRMaterialDataAsset*, int32>& MaterialCosts, TArray<TPair<UPRMaterialDataAsset*, int32>>& OutConsumedMaterials) const;

	// 실패한 구매에서 소비한 재료 복구
	void RefundBuyMaterialCosts(UPRInventoryComponent* InventoryComponent, const TArray<TPair<UPRMaterialDataAsset*, int32>>& ConsumedMaterials) const;

	// 구매 성공 후 유한 재고를 차감
	bool ConsumeStock(const FPRShopEntry& Entry, int32 Quantity);

	// 요청자에게 구매 처리 결과를 보냄
	void NotifyBuyResult(APRPlayerController* RequestingController, const FPRShopBuyResult& Result) const;

	// 요청자에게 판매 처리 결과를 보냄
	void NotifySellResult(APRPlayerController* RequestingController, const FPRShopSellResult& Result) const;

public:
	// 상점 판매 목록과 매입 조건 데이터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Shop")
	TObjectPtr<UPRShopDataAsset> ShopData;

	// 상점 요청 최소 서버 간격
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Shop", meta = (ClampMin = "0.0"))
	float ShopRequestMinInterval = 0.1f;

	// 상호작용 유효 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Shop", meta = (ClampMin = "0.0"))
	float RequestInteractionDistance = 350.0f;

private:
	// 유한 재고 상점 Entry의 현재 남은 재고
	UPROPERTY(Replicated, Transient)
	TArray<FPRShopRuntimeStock> RuntimeStocks;

	// 현재 거래 처리 중인 플레이어 목록
	TSet<TWeakObjectPtr<APRPlayerState>> TradersInTransaction;

	// 플레이어별 다음 거래 요청 허용 서버 시간
	TMap<TWeakObjectPtr<APRPlayerState>, double> NextAllowedShopRequestServerTimes;
};
