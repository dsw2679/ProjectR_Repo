// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (스폰 Predicted 투사체 구현)
#include "PRAT_SpawnPredictedProjectile.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Projectile/PRProjectileBase.h"
#include "ProjectR/Projectile/PRProjectileManagerComponent.h"

DEFINE_LOG_CATEGORY(LogPRSpawnPredictedProjectile);

UPRAT_SpawnPredictedProjectile* UPRAT_SpawnPredictedProjectile::SpawnPredictedProjectile(
	UGameplayAbility* OwningAbility, TSubclassOf<APRProjectileBase> ProjectileClass, FVector SpawnLocation,
	FRotator SpawnRotation)
{
	if (!ensureAlwaysMsgf(IsValid(OwningAbility),
		TEXT("UPRAT_SpawnPredictedProjectile 생성 실패. OwningAbility가 유효하지 않음")))
	{
		return nullptr;
	}

	if (!ensureAlwaysMsgf(OwningAbility->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted,
		TEXT("(%s): UPRAT_SpawnPredictedProjectile는 LocalPredicted 타입 어빌리티에서만 동작 가능"), *GetNameSafe(OwningAbility)))
	{
		return nullptr;
	}

	if (!ensureAlwaysMsgf(IsValid(ProjectileClass),
		TEXT("(%s): UPRAT_SpawnPredictedProjectile이 유효하지 않은 ProjectileClass를 전달 받음"), *GetNameSafe(OwningAbility)))
	{
		return nullptr;
	}

	UPRAT_SpawnPredictedProjectile* Task = NewAbilityTask<UPRAT_SpawnPredictedProjectile>(OwningAbility);
	Task->ProjectileClass = ProjectileClass;
	Task->SpawnLocation = SpawnLocation;
	Task->SpawnRotation = SpawnRotation;

	UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
		TEXT("Task 생성 완료. Ability=%s, ProjectileClass=%s, SpawnLocation=%s, SpawnRotation=%s"),
		*GetNameSafe(OwningAbility), *GetNameSafe(ProjectileClass.Get()),
		*SpawnLocation.ToCompactString(), *SpawnRotation.ToCompactString());

	return Task;
}

void UPRAT_SpawnPredictedProjectile::Activate()
{
	Super::Activate();
	
	if (!Ability)
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Warning, TEXT("UPRAT_SpawnPredictedProjectile Activate 실패. Ability 없음"));
		BroadcastFailureAndEnd();
		return;
	}

	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC)
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Warning,
			TEXT("UPRAT_SpawnPredictedProjectile Activate 실패. AbilitySystemComponent 없음, Ability=%s"), *GetNameSafe(Ability));
		BroadcastFailureAndEnd();
		return;
	}

	UPRProjectileManagerComponent* Manager = UPRProjectileManagerComponent::FindForActor(Ability->GetCurrentActorInfo()->PlayerController.Get());
	if (!Manager)
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Warning,
			TEXT("UPRAT_SpawnPredictedProjectile Activate 실패. ProjectileManager 없음, Avatar=%s, Ability=%s"),
			*GetNameSafe(GetAvatarActor()), *GetNameSafe(Ability));
		BroadcastFailureAndEnd();
		return;
	}
	CachedManager = Manager;

	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	const bool bAuthority = ActorInfo && ActorInfo->IsNetAuthority();
	const bool bLocallyControlled = ActorInfo && ActorInfo->IsLocallyControlled();

	// 실행 주체별 분기 로깅.
	UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
		TEXT("UPRAT_SpawnPredictedProjectile Activate. Ability=%s, bAuthority=%d, bLocallyControlled=%d"),
		*GetNameSafe(Ability), bAuthority, bLocallyControlled);

	if (bAuthority && bLocallyControlled)
	{
		// Standalone / Listen Host 본인: Auth 즉시 스폰
		HandleAuthorityLocal(Manager);
	}
	else if (!bAuthority && bLocallyControlled)
	{
		// 리슨 클라이언트: 예측 스폰 + 서버 송신
		HandleLocalPredictingClient(Manager);
	}
	else if (bAuthority && !bLocallyControlled)
	{
		// Remote 서버: TargetData 대기
		HandleAuthorityRemote();
	}
	else
	{
		// 시뮬레이션 프록시: LocalPredicted는 여기 도달하지 않음
		BroadcastFailureAndEnd();
	}
}

void UPRAT_SpawnPredictedProjectile::OnDestroy(bool bInOwnerFinished)
{
	UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
		TEXT("OnDestroy 호출. Ability=%s, OwnerFinished=%d, CachedProjectileId=%u"),
		*GetNameSafe(Ability), bInOwnerFinished, CachedProjectileId);

	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC)
	{
		// 등록된 TargetData 델리게이트 정리.
		const FGameplayAbilitySpecHandle SpecHandle = GetAbilitySpecHandle();
		const FPredictionKey PredKey = GetActivationPredictionKey();
		ASC->AbilityTargetDataSetDelegate(SpecHandle, PredKey).RemoveAll(this);
		ASC->AbilityTargetDataCancelledDelegate(SpecHandle, PredKey).RemoveAll(this);
	}

	// 자체 지연 스폰 타이머 정리. AT가 일찍 죽어도 만료된 콜백이 호출되지 않도록 보장
	if (DelayedSpawnTimer.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DelayedSpawnTimer);
		}
	}

	Super::OnDestroy(bInOwnerFinished);
}

/*~ Local Predicting Client ~*/

void UPRAT_SpawnPredictedProjectile::HandleLocalPredictingClient(UPRProjectileManagerComponent* Manager)
{
	// 클라이언트 예측 스폰용 ProjectileId 선할당.
	const uint32 NewId = Manager->GenerateNewProjectileId();
	CachedProjectileId = NewId;

	// 예측 거부 시 정리할 콜백 등록
	FPredictionKey PredKeyForReject = GetActivationPredictionKey();
	PredKeyForReject.NewRejectedDelegate().BindUObject(this, &UPRAT_SpawnPredictedProjectile::HandlePredictionRejected);

	FPRProjectileSpawnInfo Params;
	Params.ProjectileId = NewId;
	Params.ProjectileClass = ProjectileClass;
	Params.SpawnLocation = SpawnLocation;
	Params.SpawnRotation = SpawnRotation;

	const float SpawnDelay = Manager->GetProjectileSpawnDelay();
	if (SpawnDelay > 0.f)
	{
		// 지연 스폰: AT 자체 타이머로 만료 시 ExecuteDelayedSpawn 호출. AT 수명 내내 콜백 타겟이 살아있음을 보장
		bUsedDelayedSpawn = true;
		PendingDelayedSpawnInfo = Params;

		UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
			TEXT("LocalPredictingClient 지연 스폰 예약. ProjectileId=%u, SpawnDelay=%.3f"),
			NewId, SpawnDelay);

		UWorld* World = GetWorld();
		if (!World)
		{
			BroadcastFailureAndEnd();
			return;
		}

		FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UPRAT_SpawnPredictedProjectile::ExecuteDelayedSpawn);
		World->GetTimerManager().SetTimer(DelayedSpawnTimer, TimerDelegate, SpawnDelay, false);
		return;
	}

	APRProjectileBase* Predicted = Manager->SpawnPredictedProjectile(Params);
	if (!IsValid(Predicted))
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Warning,
			TEXT("LocalPredictingClient 실패. 예측 투사체 스폰 실패, ProjectileId=%u, ProjectileClass=%s"),
			NewId, *GetNameSafe(ProjectileClass.Get()));
		BroadcastFailureAndEnd();
		return;
	}
	SpawnedPredictedProjectile = Predicted;

	UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
		TEXT("LocalPredictingClient 성공. 예측 투사체 스폰 완료, ProjectileId=%u, Actor=%s"),
		NewId, *GetNameSafe(Predicted));

	SendSpawnDataToServer(NewId);

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnSuccess.ExecuteIfBound(Predicted);	
	}
	
	EndTask();
}

void UPRAT_SpawnPredictedProjectile::ExecuteDelayedSpawn()
{
	UPRProjectileManagerComponent* Manager = CachedManager.Get();
	if (!Manager)
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Warning,
			TEXT("ExecuteDelayedSpawn 실패. Manager 무효, CachedProjectileId=%u"), CachedProjectileId);
		BroadcastFailureAndEnd();
		return;
	}

	APRProjectileBase* Predicted = Manager->SpawnPredictedProjectile(PendingDelayedSpawnInfo);
	if (!IsValid(Predicted))
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Warning,
			TEXT("ExecuteDelayedSpawn 실패. 예측 투사체 스폰 실패, ProjectileId=%u"), CachedProjectileId);
		BroadcastFailureAndEnd();
		return;
	}

	SpawnedPredictedProjectile = Predicted;
	UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
		TEXT("ExecuteDelayedSpawn 성공. ProjectileId=%u, Actor=%s"),
		Predicted->GetProjectileId(), *GetNameSafe(Predicted));

	SendSpawnDataToServer(Predicted->GetProjectileId());

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnSuccess.ExecuteIfBound(Predicted);
	}

	EndTask();
}

void UPRAT_SpawnPredictedProjectile::SendSpawnDataToServer(uint32 ProjectileId)
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC)
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Warning,
			TEXT("SendSpawnDataToServer 실패. ASC 없음, ProjectileId=%u, Ability=%s"),
			ProjectileId, *GetNameSafe(Ability));
		return;
	}

	// 서버 권위 스폰에 필요한 최소 데이터 구성.
	FPRTargetData_SpawnProjectile* DataPtr = new FPRTargetData_SpawnProjectile();
	DataPtr->ProjectileId = ProjectileId;
	DataPtr->SpawnLocation = SpawnLocation;
	DataPtr->SpawnRotation = SpawnRotation;

	FGameplayAbilityTargetDataHandle Handle;
	Handle.Data.Add(TSharedPtr<FPRTargetData_SpawnProjectile>(DataPtr));

	const FGameplayAbilitySpecHandle SpecHandle = GetAbilitySpecHandle();
	const FPredictionKey PredKey = GetActivationPredictionKey();

	UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
		TEXT("SendSpawnDataToServer 전송. Ability=%s, ProjectileId=%u,PredKey=%d"),
		*GetNameSafe(Ability), ProjectileId, PredKey.Current);

	FScopedPredictionWindow ScopedPrediction(ASC, PredKey);
	ASC->ServerSetReplicatedTargetData(SpecHandle, PredKey, Handle, FGameplayTag(), ASC->ScopedPredictionKey);
}

/*~ Authority ~*/

void UPRAT_SpawnPredictedProjectile::HandleAuthorityLocal(UPRProjectileManagerComponent* Manager)
{
	// 권위 인스턴스 즉시 스폰 처리.
	FPRProjectileSpawnInfo Params;
	Params.ProjectileId = Manager->GenerateNewProjectileId();
	Params.ProjectileClass = ProjectileClass;
	Params.SpawnLocation = SpawnLocation;
	Params.SpawnRotation = SpawnRotation;

	APRProjectileBase* Auth = Manager->SpawnAuthProjectile(Params);
	if (!IsValid(Auth))
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Warning,
			TEXT("AuthorityLocal 실패. 권위 투사체 스폰 실패, ProjectileId=%u, ProjectileClass=%s"),
			Params.ProjectileId, *GetNameSafe(ProjectileClass.Get()));
		BroadcastFailureAndEnd();
		return;
	}

	UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
		TEXT("AuthorityLocal 성공. 권위 투사체 스폰 완료, ProjectileId=%u, Actor=%s"),
		Params.ProjectileId, *GetNameSafe(Auth));

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnSuccess.ExecuteIfBound(Auth);	
	}
	
	EndTask();
}

void UPRAT_SpawnPredictedProjectile::HandleAuthorityRemote()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC)
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Warning,
			TEXT("AuthorityRemote 실패. ASC 없음, Ability=%s"), *GetNameSafe(Ability));
		BroadcastFailureAndEnd();
		return;
	}

	const FGameplayAbilitySpecHandle SpecHandle = GetAbilitySpecHandle();
	const FPredictionKey PredKey = GetActivationPredictionKey();

	ASC->AbilityTargetDataSetDelegate(SpecHandle, PredKey).AddUObject(
		this, &UPRAT_SpawnPredictedProjectile::OnTargetDataReplicated);
	ASC->AbilityTargetDataCancelledDelegate(SpecHandle, PredKey).AddUObject(
		this, &UPRAT_SpawnPredictedProjectile::OnTargetDataCancelled);

	UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
		TEXT("AuthorityRemote 등록 완료. Ability=%s,PredKey=%d"),
		*GetNameSafe(Ability),PredKey.Current);

	// 이미 도착한 데이터가 있으면 즉시 처리
	const bool bCalledImmediately = ASC->CallReplicatedTargetDataDelegatesIfSet(SpecHandle, PredKey);
	if (!bCalledImmediately)
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
			TEXT("AuthorityRemote 대기 시작. 원격 TargetData 수신 대기 상태"));
		SetWaitingOnRemotePlayerData();
	}
	else
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
			TEXT("AuthorityRemote 즉시 처리. 이미 도착한 TargetData 존재"));
	}
}

void UPRAT_SpawnPredictedProjectile::OnTargetDataReplicated(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag /*ActivationTag*/)
{
	const FGameplayAbilityTargetData* TargetData = Data.Get(0);
	if (!TargetData || TargetData->GetScriptStruct() != FPRTargetData_SpawnProjectile::StaticStruct())
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Warning,
			TEXT("OnTargetDataReplicated 실패. TargetData 타입 불일치 또는 비어 있음, Ability=%s, DataNum=%d"),
			*GetNameSafe(Ability), Data.Num());
		BroadcastFailureAndEnd();
		return;
	}
	
	if (UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(AbilitySystemComponent.Get()))
	{
		if (!ASC->TryConsumeClientReplicatedTargetData(GetAbilitySpecHandle(),GetActivationPredictionKey()))
		{
			// 이미 다른 Task에서 Consume한 경우 다른 TargetData가 설정될 때까지 기다린다.  
			return;
		}
	}
	
	const FPRTargetData_SpawnProjectile* SpawnData = static_cast<const FPRTargetData_SpawnProjectile*>(TargetData);

	UPRProjectileManagerComponent* Manager = CachedManager.Get();
	if (!Manager)
	{
		Manager = UPRProjectileManagerComponent::FindForActor(Ability->GetCurrentActorInfo()->PlayerController.Get());
	}
	if (!Manager)
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Warning,
			TEXT("OnTargetDataReplicated 실패. ProjectileManager 없음, Avatar=%s"),
			*GetNameSafe(GetAvatarActor()));
		BroadcastFailureAndEnd();
		return;
	}

	FPRProjectileSpawnInfo Params;
	Params.ProjectileId = SpawnData->ProjectileId;
	Params.ProjectileClass = ProjectileClass;
	Params.SpawnLocation = SpawnData->SpawnLocation;
	Params.SpawnRotation = SpawnData->SpawnRotation;

	UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
		TEXT("OnTargetDataReplicated 스폰 시도. ProjectileId=%u, ProjectileClass=%s, SpawnLocation=%s, SpawnRotation=%s"),
		Params.ProjectileId, *GetNameSafe(ProjectileClass.Get()),
		*Params.SpawnLocation.ToCompactString(), *Params.SpawnRotation.ToCompactString());

	APRProjectileBase* Auth = Manager->SpawnAuthProjectile(Params);
	if (!IsValid(Auth))
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Warning,
			TEXT("OnTargetDataReplicated 실패. 권위 투사체 스폰 실패, ProjectileId=%u"), Params.ProjectileId);
		BroadcastFailureAndEnd();
		return;
	}

	UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
		TEXT("OnTargetDataReplicated 성공. 권위 투사체 스폰 완료, ProjectileId=%u, Actor=%s"),
		Params.ProjectileId, *GetNameSafe(Auth));

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnSuccess.ExecuteIfBound(Auth);	
	}
	
	// 클라이언트 발사 시점까지 서버 투사체를 전진. bUseFastForward=false인 투사체는 내부에서 무시
	// OnSuccess에서 GE 초기화등을 하므로, FastForward 시점을 OnSuccess 이후로 미룸.
	Auth->ApplyFastForward(Manager->GetForwardPredictionTime());
	
	EndTask();
}

void UPRAT_SpawnPredictedProjectile::OnTargetDataCancelled()
{
	UE_LOG(LogPRSpawnPredictedProjectile, Warning,
		TEXT("OnTargetDataCancelled 호출. 원격 TargetData 취소 수신, Ability=%s"), *GetNameSafe(Ability));
	BroadcastFailureAndEnd();
}

/*~ Rejection ~*/

void UPRAT_SpawnPredictedProjectile::HandlePredictionRejected()
{
	// 지연 스폰 타이머가 아직 만료 전이면 취소. 만료 후라면 SpawnedPredictedProjectile 정리로 충분
	if (bUsedDelayedSpawn && DelayedSpawnTimer.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
				TEXT("PredictionRejected 지연 스폰 타이머 취소. ProjectileId=%u"), CachedProjectileId);
			World->GetTimerManager().ClearTimer(DelayedSpawnTimer);
		}
	}

	UPRProjectileManagerComponent* Manager = CachedManager.Get();
	if (Manager && CachedProjectileId != 0)
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
			TEXT("PredictionRejected 예측 맵 정리. ProjectileId=%u"), CachedProjectileId);
		Manager->UnregisterPredictedProjectile(CachedProjectileId);
	}

	if (APRProjectileBase* Predicted = SpawnedPredictedProjectile.Get())
	{
		UE_LOG(LogPRSpawnPredictedProjectile, Verbose,
			TEXT("PredictionRejected 예측 투사체 파괴. Actor=%s, ProjectileId=%u"),
			*GetNameSafe(Predicted), Predicted->GetProjectileId());
		Predicted->Destroy();
	}

	BroadcastFailureAndEnd();
}

void UPRAT_SpawnPredictedProjectile::BroadcastFailureAndEnd()
{
	UE_LOG(LogPRSpawnPredictedProjectile, Warning,
		TEXT("Task 실패 종료. Ability=%s, CachedProjectileId=%u"), *GetNameSafe(Ability), CachedProjectileId);
	
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnFailed.ExecuteIfBound(nullptr);
	}
	
	if (!IsFinished())
	{
		EndTask();
	}
}
