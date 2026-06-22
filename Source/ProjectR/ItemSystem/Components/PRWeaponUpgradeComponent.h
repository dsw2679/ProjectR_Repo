// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (재료 소모 기반 무기 업그레이드 및 데미지 스탯 강화 구현)
// Author: 배유찬 (업그레이드 정보 장비 인스턴스 동기화 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/ItemSystem/Types/PRWeaponUpgradeTypes.h"
#include "PRWeaponUpgradeComponent.generated.h"

class APRPlayerController;
class UDataTable;
class UPRInventoryComponent;
class UPRItemInstance_Weapon;
class UPRMaterialDataAsset;
class UPRWeaponDataAsset;
class UPRWeaponManagerComponent;

// NPC 또는 터미널이 제공하는 무기 강화 거래를 서버 권위로 처리한다
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRWeaponUpgradeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 무기 강화 컴포넌트 기본값을 초기화한다
	UPRWeaponUpgradeComponent();

public:
	// 요청 플레이어의 무기 강화 시도를 처리한다
	UFUNCTION(BlueprintAuthorityOnly, Category = "ProjectR|WeaponUpgrade")
	FPRWeaponUpgradeResult RequestUpgradeWeapon(APRPlayerController* RequestingController, UPRItemInstance_Weapon* WeaponItem);

	// 강화 UI 표시용 다음 단계 비용과 가능 여부를 구성한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WeaponUpgrade")
	FPRWeaponUpgradePreview BuildUpgradePreview(APRPlayerController* RequestingController, UPRItemInstance_Weapon* WeaponItem) const;

	// 무기 데이터에 지정된 다음 강화 비용 Row를 찾는다
	const FPRWeaponUpgradeRow* FindNextUpgradeRow(const UPRItemInstance_Weapon* WeaponItem, int32 CurrentUpgradeLevel) const;

	// 목표 단계 RowName을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|WeaponUpgrade")
	FName MakeUpgradeRowName(int32 UpgradeLevel) const;

protected:
	// 요청 플레이어가 이 강화 대상과 상호작용 가능한지 확인한다
	bool CanRequestUpgrade(APRPlayerController* RequestingController) const;

	// 비용 재료 데이터를 조회하고 요구 수량을 합산한다
	bool ResolveMaterialCosts(const FPREconomyCost& Cost, TMap<UPRMaterialDataAsset*, int32>& OutMaterialCosts) const;

	// 요청자의 현재 무기 매니저를 조회한다
	UPRWeaponManagerComponent* ResolveWeaponManager(APRPlayerController* RequestingController) const;

	// 대상 무기 데이터에 지정된 강화 테이블을 조회한다
	UDataTable* ResolveUpgradeTable(const UPRItemInstance_Weapon* WeaponItem) const;

	// 무기 데이터의 강화 테이블을 조회한다
	UDataTable* ResolveUpgradeTableByWeaponData(const UPRWeaponDataAsset* WeaponData) const;

	// 실패 결과를 생성하고 요청자에게 알린다
	FPRWeaponUpgradeResult MakeFailureResult(APRPlayerController* RequestingController, UPRItemInstance_Weapon* WeaponItem, EPRWeaponUpgradeFailReason FailReason);

	// 성공 결과를 생성하고 요청자에게 알린다
	FPRWeaponUpgradeResult MakeSuccessResult(APRPlayerController* RequestingController, UPRItemInstance_Weapon* WeaponItem);

private:
	// 요청자에게 강화 처리 결과를 보낸다
	void NotifyResult(APRPlayerController* RequestingController, const FPRWeaponUpgradeResult& Result) const;

public:
	// 무기 데이터에 강화 테이블이 없을 때 사용할 기본 강화 단계 비용 데이터 테이블
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade")
	TObjectPtr<UDataTable> UpgradeTable;

	// 강화 가능한 최대 단계
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade", meta = (ClampMin = "1"))
	int32 MaxUpgradeLevel = 5;

	// 강화 요청 최소 서버 간격
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade", meta = (ClampMin = "0.0"))
	float UpgradeRequestMinInterval = 0.15f;

	// 상호작용 유효 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|WeaponUpgrade", meta = (ClampMin = "0.0"))
	float RequestInteractionDistance = 350.0f;

private:
	// 현재 강화 요청 처리 중 여부
	UPROPERTY(Transient)
	bool bUpgradeRequestInProgress = false;

	// 다음 강화 요청 허용 서버 시간
	UPROPERTY(Transient)
	double NextAllowedUpgradeRequestServerTime = 0.0;
};
