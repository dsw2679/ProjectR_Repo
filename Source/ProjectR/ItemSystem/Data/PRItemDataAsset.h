// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (월드 픽업 스폰용 드롭 메시 데이터 정의)
// Author: 배유찬 (아이템 기본 속성(이름/설명/등급) 데이터 필드 정의)
// Author: 이건주 (인벤토리 3D 프리뷰용 모델 데이터 필드 정의)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectR/ItemSystem/Types/PRItemTypes.h"
#include "PRItemDataAsset.generated.h"

class UTexture2D;
class UStaticMesh;
class UPRItemInstance;

// 아이템 공통 표시 데이터를 담는 기반 데이터 에셋
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPRItemDataAsset();

public:
	// 아이템 표시 이름을 반환
	UFUNCTION(BlueprintPure, Category = "Item")
	const FText& GetDisplayName() const { return DisplayName; }

	// 아이템 아이콘을 반환
	UFUNCTION(BlueprintPure, Category = "Item")
	UTexture2D* GetIcon() const { return Icon; }

	// 아이템 설명을 반환
	UFUNCTION(BlueprintPure, Category = "Item")
	const FText& GetDescription() const { return Description; }

	// 아이템 타입을 반환
	UFUNCTION(BlueprintPure, Category = "Item")
	EPRItemType GetItemType() const { return ItemType; }

	// 아이템 인스턴스 클래스 반환
	UFUNCTION(BlueprintPure, Category = "Item")
	TSubclassOf<UPRItemInstance> GetItemInstanceClass() const { return ItemInstanceClass; }

	// 아이템 희귀도를 반환
	UFUNCTION(BlueprintPure, Category = "Item")
	EPRItemRarity GetRarity() const { return Rarity; }

	// 월드 픽업에 표시할 메시를 반환
	UFUNCTION(BlueprintPure, Category = "Item")
	UStaticMesh* GetPickupMesh() const { return PickupMesh; }

	// 월드 픽업에 표시할 메시 스케일을 반환
	UFUNCTION(BlueprintPure, Category = "Item")
	const FVector& GetPickupMeshScale() const { return PickupMeshScale; }

	// 월드 픽업 희귀도 나이아가라 상대 위치를 반환
	UFUNCTION(BlueprintPure, Category = "Item")
	const FVector& GetRarityNiagaraOffset() const { return RarityNiagaraOffset; }

public:
	// 아이템 타입
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EPRItemType ItemType = EPRItemType::None;
	
	// UI에 표시할 아이템 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	// UI에 표시할 아이템 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// UI 상세 패널에 표시할 아이템 설명
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (MultiLine = "true"))
	FText Description;
	
	// 아이템 최대 보유 개수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStackCount = 1;

	// 아이템 희귀도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EPRItemRarity Rarity = EPRItemRarity::Common;

	// 월드 픽업에 표시할 메시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMesh> PickupMesh = nullptr;

	// 월드 픽업에 표시할 메시 스케일
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0.0"))
	FVector PickupMeshScale = FVector::OneVector;

	// 월드 픽업 희귀도 나이아가라의 메시 기준 상대 위치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FVector RarityNiagaraOffset = FVector::ZeroVector;

	// 인벤토리 생성 Item 인스턴스 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSubclassOf<UPRItemInstance> ItemInstanceClass;
};
