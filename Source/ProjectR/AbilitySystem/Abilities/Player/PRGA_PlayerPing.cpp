// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (플레이어 핑 어빌리티 구현)
#include "PRGA_PlayerPing.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/UI/WorldMarker/PRPingMarkerTargetInterface.h"

UPRGA_PlayerPing::UPRGA_PlayerPing()
{
	// 어빌리티 태그
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_Ping);
	SetAssetTags(DefaultAbilityTags);

	// 입력 태그
	InputTag = PRGameplayTags::Input_Ability_Ping;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UPRGA_PlayerPing::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 로컬 예측 경로
	if (!ActorInfo || !ActorInfo->IsLocallyControlledPlayer())
	{
		return;
	}

	// 조준 지점 탐색
	FHitResult Hit;
	if (!TracePingTarget(Hit))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 서버 제출 요청
	const FPRWorldMarkerRequest Request = BuildMarkerRequest(Hit);
	SubmitMarkerRequest(Request);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UPRGA_PlayerPing::TracePingTarget(FHitResult& OutHit) const
{
	APlayerController* RequestingController = GetRequestingController();
	if (!IsValid(RequestingController))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	RequestingController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	// 카메라 기준 트레이스
	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * MaxTraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRPingTrace), false, GetAvatarActorFromActorInfo());
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		// 자기 자신 제외
		QueryParams.AddIgnoredActor(AvatarActor);
	}

	const bool bBlockingHit = World->LineTraceSingleByChannel(
		OutHit,
		ViewLocation,
		TraceEnd,
		PingTraceChannel,
		QueryParams);

	if (!bBlockingHit)
	{
		return false;
	}

	if (bDrawDebugTrace)
	{
		// 트레이스 디버그
		DrawDebugLine(
			World,
			ViewLocation,
			bBlockingHit ? OutHit.ImpactPoint : TraceEnd,
			bBlockingHit ? FColor::Green : FColor::Yellow,
			false,
			DebugDrawDuration,
			0,
			1.0f);
	}

	return true;
}

AActor* UPRGA_PlayerPing::ResolvePingTargetActor(const FHitResult& Hit) const
{
	// 인터페이스 대상 판정
	AActor* HitActor = Hit.GetActor();
	if (!IsValid(HitActor))
	{
		return nullptr;
	}

	if (!HitActor->Implements<UPRPingMarkerTargetInterface>())
	{
		return nullptr;
	}

	return HitActor;
}

FPRWorldMarkerRequest UPRGA_PlayerPing::BuildMarkerRequest(const FHitResult& Hit) const
{
	// 최소 요청 페이로드 구성
	FPRWorldMarkerRequest Request;
	Request.RequestingController = GetRequestingController();
	Request.TargetActor = ResolvePingTargetActor(Hit);
	Request.WorldLocation = Hit.bBlockingHit ? Hit.ImpactPoint : Hit.TraceEnd;
	return Request;
}

// ===== 서버 제출 =====

void UPRGA_PlayerPing::SubmitMarkerRequest(const FPRWorldMarkerRequest& Request)
{
	// 호스트 권위 경로
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (IsValid(AvatarActor) && AvatarActor->HasAuthority())
	{
		UWorld* World = GetWorld();
		APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
		if (IsValid(GameState))
		{
			GameState->ServerSubmitWorldMarker(Request);
		}
		return;
	}

	// 원격 클라이언트 경로
	ServerSubmitPing(Request);
}

APlayerController* UPRGA_PlayerPing::GetRequestingController() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo)
	{
		return nullptr;
	}

	return Cast<APlayerController>(ActorInfo->PlayerController.Get());
}

void UPRGA_PlayerPing::ServerSubmitPing_Implementation(FPRWorldMarkerRequest Request)
{
	// 서버 GameState 위임
	UWorld* World = GetWorld();
	APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		return;
	}

	GameState->ServerSubmitWorldMarker(Request);
}
