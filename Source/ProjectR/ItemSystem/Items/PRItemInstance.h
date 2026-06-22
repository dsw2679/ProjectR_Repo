// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (아이템 인스턴스 데이터 구조 및 상태 보존 정보 정의)
// Author: 이건주 (3D 프리뷰 연동용 인스턴스 렌더링 데이터 정의)
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PRItemInstance.generated.h"

class UPRAbilitySystemComponent;
class AActor;
class UPRInventoryComponent;
class UPRItemDataAsset;
enum class EPRInventoryChangeReason : uint8;

// Item 활성화 요청 컨텍스트
USTRUCT(BlueprintType)
struct FPRItemActivationContext
{
	GENERATED_BODY()

	// 사용 주체 Actor
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> UserActor = nullptr;

	// 추가 컨텍스트 오브젝트 (e.g. 모드를 장착할 Weapon)
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UObject> ContextObject = nullptr; 
	
	// 요청 인벤토리 컴포넌트
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UPRInventoryComponent> InventoryComponent = nullptr;

	// 추가 컨텍스트 인덱스
	UPROPERTY(BlueprintReadWrite)
	int32 ContextIndex = INDEX_NONE;
};

// 인벤토리가 소유하는 공용 Item 인스턴스의 최소 기반 클래스다
UCLASS(BlueprintType)
class PROJECTR_API UPRItemInstance : public UObject
{
	GENERATED_BODY()

public:
	UPRItemInstance();

	/*~ UObject Interface ~*/
	virtual bool IsSupportedForNetworking() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~UPRItemInstance Interface ~*/
	virtual void InitializeItem(UPRItemDataAsset* InItemData, int32 InitialStackCount);

	// Item 활성화
	virtual bool ActivateItem(const FPRItemActivationContext& ActivationContext);

	// Item 비활성화
	virtual bool DeactivateItem(const FPRItemActivationContext& ActivationContext);
	
	// 현재 보유 개수를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	int32 GetStackCount() const { return StackCount; }

	// 현재 연결된 Item 데이터를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPRItemDataAsset* GetItemData() const { return ItemData; }

	// 보유 개수가 1 이상인지 확인
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	bool HasAnyStack() const;

	// 보상, 픽업, 상점 구매로 보유 개수를 증가시킨다
	bool AddStack(int32 AddCount);

	// 판매, 보정, 디버그 등 사용 외 경로로 보유 개수를 감소시킨다
	bool RemoveStack(int32 RemoveCount);

	// 지정 개수로 스택 설정
	void SetStack(int32 NewStack);
	
public:
	// 소유 인벤토리에 변경 이벤트를 전달
	void NotifyInventoryChanged(EPRInventoryChangeReason ChangeReason);
	
protected:
	// 보유 개수 복제 결과로 인벤토리 UI 갱신 신호를 발행
	UFUNCTION()
	virtual void OnRep_StackCount(const int32& OldStackCount);
	
protected:
	// 현재 보유 개수
	UPROPERTY(ReplicatedUsing = OnRep_StackCount, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Inventory")
	int32 StackCount = 0;
	
	// 소비 아이템 정적 데이터
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Inventory")
	TObjectPtr<UPRItemDataAsset> ItemData = nullptr;

};
