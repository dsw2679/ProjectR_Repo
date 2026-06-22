// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Loading Screen 서브시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PRLoadingScreenSubsystem.generated.h"

class UPRMapPreloadDataAsset;
class UPRRuntimePreloadDataAsset;
class AActor;
class UNiagaraComponent;
class UNiagaraSystem;
class SWidget;
class UStaticMeshComponent;
class UUserWidget;
class UWorld;

UENUM(BlueprintType)
enum class EPRLoadingState : uint8
{
	Idle,
	PreTravel,
	MapLoading,
	PostMapPreload,
	ReadyToReveal
};

UCLASS()
class PROJECTR_API UPRLoadingScreenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/*~ UGameInstanceSubsystem Interface ~*/
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	// 맵 이동 로딩 시작
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Loading")
	void BeginTravel(FName TravelReason);

	// 목적지 맵을 알고 있는 맵 이동 로딩 시작
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Loading")
	void BeginTravelToMap(FName TravelReason, const FString& MapName);

	// 현재 로딩 상태 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Loading")
	EPRLoadingState GetLoadingState() const { return LoadingState; }

	// Niagara 렌더 프리웜 진행 여부 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Loading")
	bool IsNiagaraRenderPrewarming() const { return bNiagaraRenderPrewarmInProgress; }

private:
	void HandlePreLoadMap(const FString& MapName);
	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void ShowPersistentLoadingOverlay(const FString& MapName);
	void HidePersistentLoadingOverlay();
	bool IsPersistentLoadingOverlayVisible() const;
	void StartCharacterRuntimePreload();
	void HandleCharacterRuntimeBasePreloadComplete();
	void HandleCharacterRuntimeBasePreloadTimeout();
	void TryStartCharacterRuntimeCuePreload();
	void HandleCharacterRuntimeCuePreloadComplete();
	void HandleCharacterRuntimeCuePreloadTimeout();
	void StartPostMapPreload(UWorld* LoadedWorld);
	void StartRequiredPreload(UPRMapPreloadDataAsset* MapPreloadData);
	void HandleRequiredPreloadComplete();
	void HandleRequiredPreloadTimeout();
	void StartNiagaraRenderPrewarm(UWorld* LoadedWorld, UPRMapPreloadDataAsset* MapPreloadData);
	void AdvanceNiagaraRenderPrewarmFrame();
	void FinishNiagaraRenderPrewarm();
	void CollectNiagaraRenderPrewarmSystems(UPRMapPreloadDataAsset* MapPreloadData, UPRRuntimePreloadDataAsset* RuntimePreloadData, TArray<UNiagaraSystem*>& OutNiagaraSystems) const;
	void StartInteractionOutlineRenderPrewarm(UWorld* LoadedWorld);
	void AdvanceInteractionOutlineRenderPrewarmFrame();
	void FinishInteractionOutlineRenderPrewarm();
	void StartShopUIPrewarm(UWorld* LoadedWorld, UPRMapPreloadDataAsset* MapPreloadData);
	void HandleShopUIPrewarmComplete();
	void TryReveal();
	void FinishRevealAfterFadeOut();
	void SetLoadingState(EPRLoadingState NewState);
	bool IsLoadedWorldTravelDestination(UWorld* LoadedWorld) const;
	UPRMapPreloadDataAsset* ResolveMapPreloadData(UWorld* LoadedWorld) const;
	UPRRuntimePreloadDataAsset* ResolveRuntimePreloadData() const;
	void CollectRequiredAssetPaths(UPRMapPreloadDataAsset* MapPreloadData, TArray<FSoftObjectPath>& OutAssetPaths) const;

private:
	EPRLoadingState LoadingState = EPRLoadingState::Idle;
	FName CurrentTravelReason = NAME_None;
	FString CurrentTravelMapName;
	double TravelStartSeconds = 0.0;
	bool bRequiredPreloadComplete = false;
	bool bCharacterRuntimeBasePreloadComplete = true;
	bool bCharacterRuntimePreloadComplete = true;
	bool bCharacterRuntimeCuePreloadStarted = false;
	bool bNiagaraRenderPrewarmComplete = true;
	bool bNiagaraRenderPrewarmInProgress = false;
	bool bInteractionOutlineRenderPrewarmComplete = true;
	bool bInteractionOutlineRenderPrewarmInProgress = false;
	bool bShopUIPrewarmComplete = true;
	bool bMinimumDisplayTimeComplete = false;
	bool bRevealFadeOutInProgress = false;
	int32 RemainingNiagaraRenderPrewarmFrames = 0;
	int32 InteractionOutlineRenderPrewarmStep = 0;
	TWeakObjectPtr<UWorld> CurrentLoadedWorld;
	FTimerHandle MinimumDisplayTimerHandle;
	FTimerHandle RevealFadeOutTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> PersistentLoadingWidget;

	TSharedPtr<SWidget> PersistentLoadingSlateWidget;

	bool bPersistentLoadingOverlayAdded = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> PrewarmNiagaraComponents;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PrewarmInteractionOutlineActor;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> PrewarmInteractionOutlineMeshComponent;
};
