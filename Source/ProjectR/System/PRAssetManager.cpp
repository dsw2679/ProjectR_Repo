// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (로딩 화면 프리웜 및 프리로드 에셋 캐싱 시스템 구현)
// Author: 배유찬 (어빌리티 레지스트리 및 리스폰 관련 에셋 비동기 로드 구현)
#include "PRAssetManager.h"
#include "PRDeveloperSettings.h"
#include "Containers/Ticker.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/StreamableManager.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Types/PRDropTypes.h"

UPRAssetManager& UPRAssetManager::Get()
{
	UPRAssetManager* Singleton = Cast<UPRAssetManager>(GEngine->AssetManager);
	checkf(Singleton, TEXT("PRAssetManager 미설정. DefaultEngine.ini의 AssetManagerClassName을 PRAssetManager로 지정 필요"));
	return *Singleton;
}

void UPRAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	LoadRegistries();
}

UPRAbilitySystemRegistry* UPRAssetManager::GetAbilitySystemRegistry()
{
	if (!CachedAbilitySystemRegistry)
	{
		LoadRegistries();
	}
	return CachedAbilitySystemRegistry;
}

const FPRMonsterDropTableRow* UPRAssetManager::FindMonsterDropRow(FName MonsterId)
{
	if (MonsterId.IsNone())
	{
		return nullptr;
	}

	if (!CachedMonsterDropTable)
	{
		LoadRegistries();
	}

	if (!::IsValid(CachedMonsterDropTable))
	{
		return nullptr;
	}

	static const FString ContextString(TEXT("UPRAssetManager::FindMonsterDropRow"));
	return CachedMonsterDropTable->FindRow<FPRMonsterDropTableRow>(MonsterId, ContextString, false);
}

UPRItemDataAsset* UPRAssetManager::GetItemDataByPrimaryAssetId(const FPrimaryAssetId& ItemAssetId)
{
	if (!ItemAssetId.IsValid())
	{
		return nullptr;
	}

	const FSoftObjectPath AssetPath = GetPrimaryAssetPath(ItemAssetId);
	if (!AssetPath.IsValid())
	{
		return nullptr;
	}

	return Cast<UPRItemDataAsset>(AssetPath.TryLoad());
}

uint64 UPRAssetManager::LoadAssetsAsync(const TArray<FSoftObjectPath>& AssetPaths, FPRAssetsLoadedNative Callback)
{
	const uint64 RequestId = NextAsyncLoadRequestId++;

	TArray<FSoftObjectPath> UniqueAssetPaths;
	for (const FSoftObjectPath& AssetPath : AssetPaths)
	{
		if (!AssetPath.IsValid())
		{
			continue;
		}

		// 중복 경로 제거
		UniqueAssetPaths.AddUnique(AssetPath);
	}

	if (UniqueAssetPaths.IsEmpty())
	{
		FPRAssetLoadResult Result;
		Result.RequestId = RequestId;
		Callback.ExecuteIfBound(Result);
		return RequestId;
	}

	TWeakObjectPtr<UPRAssetManager> WeakThis(this);
	TSharedPtr<FStreamableHandle> StreamableHandle = GetStreamableManager().RequestAsyncLoad(
		UniqueAssetPaths,
		FStreamableDelegate::CreateLambda([WeakThis, RequestId, UniqueAssetPaths, Callback]()
		{
			UPRAssetManager* AssetManager = WeakThis.Get();
			if (!AssetManager)
			{
				return;
			}

			FPRAssetLoadResult Result;
			Result.RequestId = RequestId;
			for (const FSoftObjectPath& AssetPath : UniqueAssetPaths)
			{
				// 로드 완료 경로의 UObject 해석
				Result.LoadedAssets.Add(AssetPath, AssetPath.ResolveObject());
			}

			Callback.ExecuteIfBound(Result);
			AssetManager->ActiveAsyncLoadHandles.Remove(RequestId);
		}));

	if (StreamableHandle.IsValid())
	{
		// 콜백 완료 전 핸들 보존
		ActiveAsyncLoadHandles.Add(RequestId, StreamableHandle);
	}
	else
	{
		FPRAssetLoadResult Result;
		Result.RequestId = RequestId;
		for (const FSoftObjectPath& AssetPath : UniqueAssetPaths)
		{
			// 즉시 해석 가능한 로드 결과
			Result.LoadedAssets.Add(AssetPath, AssetPath.ResolveObject());
		}

		Callback.ExecuteIfBound(Result);
	}

	return RequestId;
}

uint64 UPRAssetManager::StartPreloadRequest(const FPRPreloadRequest& Request, FSimpleDelegate OnComplete, FSimpleDelegate OnTimeout)
{
	const uint64 RequestId = NextPreloadRequestId++;

	TArray<FSoftObjectPath> UniqueAssetPaths;
	for (const FSoftObjectPath& AssetPath : Request.AssetPaths)
	{
		if (!AssetPath.IsValid())
		{
			continue;
		}

		UniqueAssetPaths.AddUnique(AssetPath);
	}

	FPRActivePreloadRequest& ActiveRequest = ActivePreloadRequests.Add(RequestId);
	ActiveRequest.Group = Request.Group;
	ActiveRequest.QueueType = Request.QueueType;
	ActiveRequest.AssetPaths = UniqueAssetPaths;
	ActiveRequest.OnComplete = OnComplete;
	ActiveRequest.OnTimeout = OnTimeout;
	PreloadGroupRequestIds.FindOrAdd(Request.Group).Add(RequestId);

	if (UniqueAssetPaths.IsEmpty())
	{
		ActiveRequest.bCompleted = true;
		OnComplete.ExecuteIfBound();
		return RequestId;
	}

	TWeakObjectPtr<UPRAssetManager> WeakThis(this);
	TSharedPtr<FStreamableHandle> StreamableHandle = GetStreamableManager().RequestAsyncLoad(
		UniqueAssetPaths,
		FStreamableDelegate::CreateLambda([WeakThis, RequestId]()
		{
			UPRAssetManager* AssetManager = WeakThis.Get();
			if (!AssetManager)
			{
				return;
			}

			FPRActivePreloadRequest* ActiveRequest = AssetManager->ActivePreloadRequests.Find(RequestId);
			if (!ActiveRequest || ActiveRequest->bCompleted)
			{
				return;
			}

			ActiveRequest->bCompleted = true;
			if (ActiveRequest->TimeoutTickerHandle.IsValid())
			{
				FTSTicker::GetCoreTicker().RemoveTicker(ActiveRequest->TimeoutTickerHandle);
				ActiveRequest->TimeoutTickerHandle.Reset();
			}

			ActiveRequest->OnComplete.ExecuteIfBound();
		}),
		Request.QueueType == EPRPreloadQueueType::Required ? 100 : 0);

	ActiveRequest.Handle = StreamableHandle;

	if (Request.TimeoutSeconds > 0.0f)
	{
		ActiveRequest.TimeoutTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateWeakLambda(this, [RequestId](float)
			{
				UPRAssetManager* AssetManager = Cast<UPRAssetManager>(GEngine ? GEngine->AssetManager : nullptr);
				if (!AssetManager)
				{
					return false;
				}

				FPRActivePreloadRequest* ActiveRequest = AssetManager->ActivePreloadRequests.Find(RequestId);
				if (!ActiveRequest || ActiveRequest->bCompleted || ActiveRequest->bTimedOut)
				{
					return false;
				}

				ActiveRequest->bTimedOut = true;
				UE_LOG(LogTemp, Warning, TEXT("PRAssetManager: Preload timeout RequestId=%llu RemainingAssets=%d"), RequestId, ActiveRequest->AssetPaths.Num());
				ActiveRequest->OnTimeout.ExecuteIfBound();
				return false;
			}),
			Request.TimeoutSeconds);
	}

	if (!StreamableHandle.IsValid())
	{
		ActiveRequest.bCompleted = true;
		if (ActiveRequest.TimeoutTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(ActiveRequest.TimeoutTickerHandle);
			ActiveRequest.TimeoutTickerHandle.Reset();
		}

		OnComplete.ExecuteIfBound();
	}

	return RequestId;
}

void UPRAssetManager::PromoteDeferredPreload(uint64 RequestId)
{
	FPRActivePreloadRequest* ActiveRequest = ActivePreloadRequests.Find(RequestId);
	if (!ActiveRequest || ActiveRequest->bCompleted || ActiveRequest->QueueType != EPRPreloadQueueType::Deferred)
	{
		return;
	}

	ActiveRequest->QueueType = EPRPreloadQueueType::Required;
}

void UPRAssetManager::CancelPreloadRequest(uint64 RequestId)
{
	FPRActivePreloadRequest* ActiveRequest = ActivePreloadRequests.Find(RequestId);
	if (!ActiveRequest)
	{
		return;
	}

	if (ActiveRequest->TimeoutTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ActiveRequest->TimeoutTickerHandle);
		ActiveRequest->TimeoutTickerHandle.Reset();
	}

	if (ActiveRequest->Handle.IsValid())
	{
		ActiveRequest->Handle->CancelHandle();
	}

	if (TSet<uint64>* RequestIds = PreloadGroupRequestIds.Find(ActiveRequest->Group))
	{
		RequestIds->Remove(RequestId);
	}

	ActivePreloadRequests.Remove(RequestId);
}

void UPRAssetManager::ReleasePreloadGroup(EPRPreloadGroup Group)
{
	TSet<uint64> RequestIds;
	if (TSet<uint64>* FoundRequestIds = PreloadGroupRequestIds.Find(Group))
	{
		RequestIds = *FoundRequestIds;
	}

	for (uint64 RequestId : RequestIds)
	{
		CancelPreloadRequest(RequestId);
	}

	PreloadGroupRequestIds.Remove(Group);
}

bool UPRAssetManager::IsPreloadGroupComplete(EPRPreloadGroup Group) const
{
	const TSet<uint64>* RequestIds = PreloadGroupRequestIds.Find(Group);
	if (!RequestIds || RequestIds->IsEmpty())
	{
		return true;
	}

	for (uint64 RequestId : *RequestIds)
	{
		const FPRActivePreloadRequest* ActiveRequest = ActivePreloadRequests.Find(RequestId);
		if (ActiveRequest && !ActiveRequest->bCompleted)
		{
			return false;
		}
	}

	return true;
}

void UPRAssetManager::LoadRegistries()
{
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (Settings == nullptr)
	{
		return;
	}

	if (!CachedAbilitySystemRegistry && !Settings->AbilitySystemRegistry.IsNull())
	{
		CachedAbilitySystemRegistry = Settings->AbilitySystemRegistry.LoadSynchronous();
		if (!CachedAbilitySystemRegistry)
		{
			UE_LOG(LogTemp, Error, TEXT("PRAssetManager: AbilitySystemRegistry 로드 실패"));
		}
	}

	if (!CachedMonsterDropTable && !Settings->MonsterDropTable.IsNull())
	{
		CachedMonsterDropTable = Settings->MonsterDropTable.LoadSynchronous();
		if (!CachedMonsterDropTable)
		{
			UE_LOG(LogTemp, Error, TEXT("PRAssetManager: MonsterDropTable 로드 실패"));
		}
	}
}
