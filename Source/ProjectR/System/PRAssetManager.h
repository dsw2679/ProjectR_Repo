// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (로딩 화면 프리웜 및 프리로드 에셋 캐싱 시스템 구현)
// Author: 배유찬 (어빌리티 레지스트리 및 리스폰 관련 에셋 비동기 로드 구현)
#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Engine/AssetManager.h"
#include "PRAssetManager.generated.h"

class UPRAbilitySystemRegistry;
class UDataTable;
class UPRItemDataAsset;
struct FStreamableHandle;
struct FPRMonsterDropTableRow;

// 비동기 로드 완료 결과
struct FPRAssetLoadResult
{
	// 요청 식별자
	uint64 RequestId = 0;

	// 요청 경로별 로드 결과
	TMap<FSoftObjectPath, TWeakObjectPtr<UObject>> LoadedAssets;
};

// 비동기 로드 완료 네이티브 콜백
DECLARE_DELEGATE_OneParam(FPRAssetsLoadedNative, const FPRAssetLoadResult&);

UENUM(BlueprintType)
enum class EPRPreloadGroup : uint8
{
	MapTransition,
	MapRuntime,
	CharacterRuntime,
	CombatRuntime
};

UENUM(BlueprintType)
enum class EPRPreloadQueueType : uint8
{
	Required,
	Deferred
};

USTRUCT(BlueprintType)
struct FPRPreloadRequest
{
	GENERATED_BODY()

	// 프리로드 핸들을 보존할 그룹
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Preload")
	EPRPreloadGroup Group = EPRPreloadGroup::MapTransition;

	// Ready Gate 차단 여부를 결정하는 큐 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Preload")
	EPRPreloadQueueType QueueType = EPRPreloadQueueType::Required;

	// 비동기 로드 대상 경로
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Preload")
	TArray<FSoftObjectPath> AssetPaths;

	// 0 이하이면 타임아웃 비활성
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Preload", meta = (ClampMin = "0.0"))
	float TimeoutSeconds = 0.0f;
};

// 프로젝트 전용 AssetManager. DeveloperSettings에 지정된 Registry들을 시작 시 동기 로드 및 캐싱
UCLASS(Config = Game)
class PROJECTR_API UPRAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	// 싱글톤 접근. 프로젝트 AssetManager 클래스 미지정 시 체크 실패
	static UPRAssetManager& Get();

	/*~ UAssetManager Interface ~*/
	virtual void StartInitialLoading() override;

public:
	// 캐싱된 AbilitySystem Registry 반환. 미로드 시 Lazy Load
	UPRAbilitySystemRegistry* GetAbilitySystemRegistry();

	// 몬스터 드롭 테이블 Row를 조회한다
	const FPRMonsterDropTableRow* FindMonsterDropRow(FName MonsterId);

	// Primary Asset Id로 아이템 데이터 에셋을 조회한다
	UPRItemDataAsset* GetItemDataByPrimaryAssetId(const FPrimaryAssetId& ItemAssetId);

	// UObject 경로 목록을 타입 해석 없이 비동기 로드 요청
	uint64 LoadAssetsAsync(const TArray<FSoftObjectPath>& AssetPaths, FPRAssetsLoadedNative Callback);

	// 그룹 보존과 타임아웃을 지원하는 프리로드 요청 시작
	uint64 StartPreloadRequest(const FPRPreloadRequest& Request, FSimpleDelegate OnComplete, FSimpleDelegate OnTimeout = FSimpleDelegate());

	// 지연 큐 요청을 높은 우선순위로 승격
	void PromoteDeferredPreload(uint64 RequestId);

	// 진행 중인 프리로드 요청 취소
	void CancelPreloadRequest(uint64 RequestId);

	// 지정 그룹의 보존 핸들 해제
	void ReleasePreloadGroup(EPRPreloadGroup Group);

	// 지정 그룹의 진행 중 요청 완료 여부
	bool IsPreloadGroupComplete(EPRPreloadGroup Group) const;

private:
	// DeveloperSettings 기반 Registry들을 동기 로드 및 캐시에 등록
	void LoadRegistries();

private:
	UPROPERTY(Transient)
	TObjectPtr<UPRAbilitySystemRegistry> CachedAbilitySystemRegistry;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CachedMonsterDropTable;

	uint64 NextAsyncLoadRequestId = 1;

	TMap<uint64, TSharedPtr<FStreamableHandle>> ActiveAsyncLoadHandles;

	struct FPRActivePreloadRequest
	{
		EPRPreloadGroup Group = EPRPreloadGroup::MapTransition;
		EPRPreloadQueueType QueueType = EPRPreloadQueueType::Required;
		TArray<FSoftObjectPath> AssetPaths;
		TSharedPtr<FStreamableHandle> Handle;
		FSimpleDelegate OnComplete;
		FSimpleDelegate OnTimeout;
		FTSTicker::FDelegateHandle TimeoutTickerHandle;
		bool bCompleted = false;
		bool bTimedOut = false;
	};

	uint64 NextPreloadRequestId = 1;

	TMap<uint64, FPRActivePreloadRequest> ActivePreloadRequests;

	TMap<EPRPreloadGroup, TSet<uint64>> PreloadGroupRequestIds;
};
