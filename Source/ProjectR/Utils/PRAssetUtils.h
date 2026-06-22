// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (렌더 프리웜 및 에셋 비동기 로딩 헬퍼 구현)
// Author: 배유찬 (리스폰 에셋 메모리 프리로드 헬퍼 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "PRAssetUtils.generated.h"

class APRPlayerCharacter;
class UPRWeaponDataAsset;
class UPRWeaponManagerComponent;

// 에셋 경로 수집 유틸리티
UCLASS()
class PROJECTR_API UPRAssetUtils : public UObject
{
	GENERATED_BODY()

public:
	// 캐릭터 저장 데이터 프리뷰에 필요한 1차 에셋 경로 수집
	static void CollectPreviewRootAssetPaths(const FPRCharacterSaveData& SaveData, TArray<FSoftObjectPath>& OutAssetPaths);

	// 현재 캐릭터 프리뷰에 필요한 1차 에셋 경로 수집
	static void CollectPreviewRootAssetPaths(const APRPlayerCharacter* SourceCharacter, const UPRWeaponManagerComponent* WeaponManagerComponent, TArray<FSoftObjectPath>& OutAssetPaths);

	// 현재 캐릭터 프리뷰에서 이미 로드된 1차 에셋 수집
	static void CollectPreviewLoadedRootAssets(const APRPlayerCharacter* SourceCharacter, const UPRWeaponManagerComponent* WeaponManagerComponent, TArray<UObject*>& OutLoadedAssets);

	// 로드된 1차 에셋에서 프리뷰 적용에 필요한 의존 에셋 경로 수집
	static void CollectPreviewDependentAssetPaths(const TArray<UObject*>& LoadedAssets, TArray<FSoftObjectPath>& OutAssetPaths);

	// 비동기 로드 결과 맵에서 UObject 배열 수집
	static void CollectLoadedAssetsFromMap(const TMap<FSoftObjectPath, TWeakObjectPtr<UObject>>& LoadedAssetMap, TArray<UObject*>& OutLoadedAssets);

	// 플레이어 세이브 기준 런타임 진입 전에 필요한 경로 수집
	static void CollectPlayerRuntimePreloadPaths(const FPRCharacterSaveData& SaveData, TArray<FSoftObjectPath>& OutAssetPaths);

	// 기본 플레이어 전투 FX 태그 수집
	static void CollectDefaultPlayerCombatFXTags(FGameplayTagContainer& OutFXTags);

private:
	// 유효한 경로 중복 제거 추가
	static void AddUniqueAssetPath(const FSoftObjectPath& AssetPath, TArray<FSoftObjectPath>& OutAssetPaths);

	// 유효한 UObject 경로 중복 제거 추가
	static void AddUniqueAssetPathFromObject(const UObject* AssetObject, TArray<FSoftObjectPath>& OutAssetPaths);

	static void AddWeaponEntryPathByIndex(const FPRCharacterSaveData& SaveData, int32 WeaponIndex, TArray<FSoftObjectPath>& OutAssetPaths);
	static void AddLoadedWeaponDependencyPaths(const UPRWeaponDataAsset* WeaponData, TArray<FSoftObjectPath>& OutAssetPaths);
};
