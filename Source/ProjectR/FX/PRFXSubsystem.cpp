// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (렌더 프리웜 및 프리로드용 이펙트 캐싱 시스템 구현)
// Author: 배유찬 (사격 궤적 및 데미지 이펙트 런타임 제어 구현)
// Author: 손승우 (보스 특수 연출용 FX 캐싱 구현)
#include "PRFXSubsystem.h"

#include "PRFXCue.h"
#include "PRFXRegistryDataAsset.h"
#include "ProjectR/System/PRDeveloperSettings.h"

void UPRFXSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 한 월드 안에서 생성되는 예측 키 충돌 방지용 SourceId 초기화
	LocalPredictionSourceId = FGuid::NewGuid();
	NextPredictionSequence = 1;
}

void UPRFXSubsystem::Deinitialize()
{
	CachedRegistry = nullptr;
	PerPlayerCueInstances.Empty();
	ExecutionCueInstances.Empty();
	PredictedFXKeys.Empty();
	PersistentActors.Empty();

	Super::Deinitialize();
}

void UPRFXSubsystem::PlayLocalFX(FPRFXRequest Request)
{
	ExecuteCue(Request, EPRFXPlaybackMode::LocalOnly);
}

FPRFXPredictionKey UPRFXSubsystem::PlayPredictiveFX(FPRFXRequest Request)
{
	FPRFXRegistryEntry Entry;
	if (!FindRegistryEntry(Request.FXTag, Entry))
	{
		UE_LOG(LogPRFX, Warning, TEXT("예측 FX Registry 조회 실패. FXTag=%s"), *Request.FXTag.ToString());
		return FPRFXPredictionKey();
	}

	if (!Entry.bAllowPrediction)
	{
		UE_LOG(LogPRFX, Warning, TEXT("예측 재생이 허용되지 않은 FX 요청. FXTag=%s"), *Request.FXTag.ToString());
		return FPRFXPredictionKey();
	}

	// 로컬 선재생과 서버 확정 재생을 비교하기 위한 예측 키 저장
	FPRFXPredictionKey PredictionKey = GeneratePredictionKey();
	PredictedFXKeys.Add(PredictionKey);

	Request.PredictionKey = PredictionKey;
	ExecuteCue(Request, EPRFXPlaybackMode::PredictiveLocal);

	return PredictionKey;
}

void UPRFXSubsystem::PlayConfirmedFX(FPRFXRequest Request)
{
	FPRFXRegistryEntry Entry;
	if (!FindRegistryEntry(Request.FXTag, Entry))
	{
		UE_LOG(LogPRFX, Warning, TEXT("확정 FX Registry 조회 실패. FXTag=%s"), *Request.FXTag.ToString());
		return;
	}

	// 서버 확정 FX가 이미 로컬에서 예측 재생된 요청인지 확인
	const bool bPredictedDuplicate = Request.PredictionKey.IsValid() && PredictedFXKeys.Contains(Request.PredictionKey);
	if (bPredictedDuplicate)
	{
		PredictedFXKeys.Remove(Request.PredictionKey);
		if (Entry.OwnerReplayPolicy == EPRFXOwnerReplayPolicy::SkipIfPredicted)
		{
			return;
		}
	}

	ExecuteCue(Request, EPRFXPlaybackMode::ServerConfirmed);
}

void UPRFXSubsystem::RegisterPersistentFX(FPRFXPersistentId PersistentId, AActor* PersistentActor)
{
	if (!PersistentId.IsValid() || !IsValid(PersistentActor))
	{
		return;
	}

	// 지속 FX 종료 요청에서 Actor를 찾기 위한 PersistentId와 Actor 연결 저장
	PersistentActors.FindOrAdd(PersistentId) = PersistentActor;
}

AActor* UPRFXSubsystem::FindPersistentFX(FPRFXPersistentId PersistentId) const
{
	if (const TWeakObjectPtr<AActor>* FoundActor = PersistentActors.Find(PersistentId))
	{
		return FoundActor->Get();
	}

	return nullptr;
}

void UPRFXSubsystem::UnregisterPersistentFX(FPRFXPersistentId PersistentId)
{
	PersistentActors.Remove(PersistentId);
}

bool UPRFXSubsystem::FindRegistryEntry(FGameplayTag FXTag, FPRFXRegistryEntry& OutEntry) const
{
	// 실행 시점에 Registry를 로드한 뒤 FXTag에 연결된 Entry 조회
	const UPRFXRegistryDataAsset* Registry = GetRegistry();
	if (!IsValid(Registry))
	{
		UE_LOG(LogPRFX, Warning, TEXT("FX Registry가 설정되지 않음. FXTag=%s"), *FXTag.ToString());
		return false;
	}

	return Registry->FindEntry(FXTag, OutEntry);
}

void UPRFXSubsystem::CollectPreloadAssetPathsForTags(const FGameplayTagContainer& FXTags, TArray<FSoftObjectPath>& OutAssetPaths) const
{
	for (const FGameplayTag& FXTag : FXTags)
	{
		FPRFXRegistryEntry Entry;
		if (!FindRegistryEntry(FXTag, Entry))
		{
			continue;
		}

		for (const TSoftClassPtr<UPRFXCue>& CueClass : Entry.CueClasses)
		{
			if (!CueClass.IsNull())
			{
				OutAssetPaths.AddUnique(CueClass.ToSoftObjectPath());
			}
		}
	}
}

bool UPRFXSubsystem::ResolveCues(FGameplayTag FXTag, TArray<UPRFXCue*>& OutCues)
{
	OutCues.Reset();

	FPRFXRegistryEntry Entry;
	if (!FindRegistryEntry(FXTag, Entry))
	{
		return false;
	}

	if (Entry.CueClasses.IsEmpty())
	{
		UE_LOG(LogPRFX, Warning, TEXT("FX CueClass 목록이 비어 있음. FXTag=%s"), *FXTag.ToString());
		return false;
	}

	for (const TSoftClassPtr<UPRFXCue>& CueClassRef : Entry.CueClasses)
	{
		UClass* CueClass = CueClassRef.LoadSynchronous();
		if (!IsValid(CueClass))
		{
			UE_LOG(LogPRFX, Warning, TEXT("FX CueClass 로드 실패. FXTag=%s, CueClass=%s"), *FXTag.ToString(), *CueClassRef.ToSoftObjectPath().ToString());
			continue;
		}

		// 에디터 설정에 추상 Cue 클래스가 등록된 경우 해당 요소 실행 차단
		if (CueClass->HasAnyClassFlags(CLASS_Abstract))
		{
			UE_LOG(LogPRFX, Warning, TEXT("추상 FX CueClass 실행 요청. FXTag=%s, CueClass=%s"), *FXTag.ToString(), *GetNameSafe(CueClass));
			continue;
		}

		// Cue 클래스 기본 객체에서 Instancing 정책을 읽어 실행 객체 선택
		UPRFXCue* DefaultCue = Cast<UPRFXCue>(CueClass->GetDefaultObject());
		if (!IsValid(DefaultCue))
		{
			UE_LOG(LogPRFX, Warning, TEXT("FX CueClass 기본 객체가 유효하지 않음. FXTag=%s, CueClass=%s"), *FXTag.ToString(), *GetNameSafe(CueClass));
			continue;
		}

		switch (DefaultCue->GetInstancingPolicy())
		{
		case EPRFXCueInstancingPolicy::NonInstanced:
			// 내부 상태가 없는 Cue를 클래스 기본 객체로 실행
			OutCues.Add(DefaultCue);
			break;

		case EPRFXCueInstancingPolicy::InstancedPerExecution:
		{
			// Timer나 Latent Action을 사용할 수 있는 요청별 Cue 인스턴스 생성
			UPRFXCue* ExecutionCue = NewObject<UPRFXCue>(this, CueClass);
			if (IsValid(ExecutionCue))
			{
				ExecutionCueInstances.Add(ExecutionCue);
				OutCues.Add(ExecutionCue);
			}
			break;
		}

		case EPRFXCueInstancingPolicy::InstancedPerPlayer:
		{
			UPRFXCue* PlayerCue = nullptr;

			// 로컬 플레이어별 상태를 공유하는 Cue 클래스별 인스턴스 재사용
			for (UPRFXCue* ExistingCue : PerPlayerCueInstances)
			{
				if (IsValid(ExistingCue) && ExistingCue->GetClass() == CueClass)
				{
					PlayerCue = ExistingCue;
					break;
				}
			}

			if (!IsValid(PlayerCue))
			{
				PlayerCue = NewObject<UPRFXCue>(this, CueClass);
				if (IsValid(PlayerCue))
				{
					PerPlayerCueInstances.Add(PlayerCue);
				}
			}

			if (IsValid(PlayerCue))
			{
				OutCues.Add(PlayerCue);
			}
			break;
		}

		default:
			OutCues.Add(DefaultCue);
			break;
		}
	}

	return !OutCues.IsEmpty();
}

UPRFXRegistryDataAsset* UPRFXSubsystem::GetRegistry() const
{
	if (IsValid(CachedRegistry))
	{
		return CachedRegistry;
	}

	const UPRDeveloperSettings* DeveloperSettings = GetDefault<UPRDeveloperSettings>();
	if (!IsValid(DeveloperSettings))
	{
		return nullptr;
	}

	// 프로젝트 설정에 지정된 Registry를 첫 FX 요청 시점에 동기 로드
	CachedRegistry = DeveloperSettings->FXRegistry.LoadSynchronous();
	return CachedRegistry;
}

void UPRFXSubsystem::ExecuteCue(const FPRFXRequest& Request, EPRFXPlaybackMode PlaybackMode)
{
	if (!Request.FXTag.IsValid())
	{
		UE_LOG(LogPRFX, Warning, TEXT("유효하지 않은 FXTag 실행 요청"));
		return;
	}

	TArray<UPRFXCue*> Cues;
	if (!ResolveCues(Request.FXTag, Cues))
	{
		return;
	}

	// Request의 FXTag와 예측 키에 현재 재생 모드를 더해 CueContext 구성
	const FPRFXCueContext Context = BuildCueContext(Request, PlaybackMode);
	for (UPRFXCue* Cue : Cues)
	{
		if (IsValid(Cue))
		{
			Cue->NativeExecuteFX(Context, Request.Payload);
		}
	}
}

FPRFXCueContext UPRFXSubsystem::BuildCueContext(const FPRFXRequest& Request, EPRFXPlaybackMode PlaybackMode) const
{
	FPRFXCueContext Context;
	Context.FXTag = Request.FXTag;
	Context.PlaybackMode = PlaybackMode;
	Context.PredictionKey = Request.PredictionKey;
	Context.WorldContext = GetWorld();
	return Context;
}

FPRFXPredictionKey UPRFXSubsystem::GeneratePredictionKey()
{
	// 월드별 SourceId와 증가값을 조합한 예측 재생 식별자 생성
	FPRFXPredictionKey PredictionKey;
	PredictionKey.SourceId = LocalPredictionSourceId;
	PredictionKey.Sequence = NextPredictionSequence++;
	PredictionKey.bValid = true;

	if (NextPredictionSequence <= 0)
	{
		NextPredictionSequence = 1;
	}

	return PredictionKey;
}
