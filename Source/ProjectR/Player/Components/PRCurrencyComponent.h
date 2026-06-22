// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (골드 획득/소모 로직 및 실시간 UI 동기화 구현)
// Author: 배유찬 (리스폰 골드 패널티 및 골드 상태 보존 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRCurrencyComponent.generated.h"

struct FPRCurrencySaveData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRScrapChangedSignature, int32, NewScrap);

// 플레이어가 보유한 고철 재화를 서버 권위로 관리한다
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRCurrencyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRCurrencyComponent();

	/*~ UActorComponent Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 현재 고철 보유량을 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Currency")
	int32 GetScrap() const { return Scrap; }

	// 서버에서 고철을 증가시킨다
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "ProjectR|Currency")
	bool AddScrap(int32 Amount);

	// 서버에서 고철을 소비하고 성공 여부를 반환한다
	UFUNCTION(BlueprintAuthorityOnly, Category = "ProjectR|Currency")
	bool TrySpendScrap(int32 Amount);

	// 재화 저장 데이터 생성
	FPRCurrencySaveData MakeSaveData() const;

	// 재화 저장 데이터 적용
	void ApplySaveData(const FPRCurrencySaveData& InSaveData);

	// 리스폰 전 재화 시스템 런타임 상태 정리
	void ResetSystem();

protected:
	UFUNCTION()
	void OnRep_Scrap();

public:
	UPROPERTY(BlueprintAssignable, Category = "ProjectR|Currency")
	FPRScrapChangedSignature OnScrapChanged;

private:
	// 현재 보유한 고철 수량
	UPROPERTY(ReplicatedUsing = OnRep_Scrap, VisibleInstanceOnly, Category = "ProjectR|Currency")
	int32 Scrap = 0;
};
