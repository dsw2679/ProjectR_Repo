// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (이펙트 Network 컴포넌트 구현)
#include "PRFXNetworkComponent.h"

#include "PRFXCue.h"
#include "PRFXSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"

UPRFXNetworkComponent::UPRFXNetworkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UPRFXNetworkComponent* UPRFXNetworkComponent::FindForActor(AActor* InActor)
{
	if (!IsValid(InActor))
	{
		return nullptr;
	}

	if (UPRFXNetworkComponent* FoundComponent = InActor->FindComponentByClass<UPRFXNetworkComponent>())
	{
		return FoundComponent;
	}
	
	// Pawn 요청에서 Controller의 FX 네트워크 컴포넌트까지 검색
	if (APawn* AsPawn = Cast<APawn>(InActor))
	{
		if (APlayerController* PC = AsPawn->GetController<APlayerController>())
		{
			return PC->FindComponentByClass<UPRFXNetworkComponent>();
		}
	}
	
	// Controller 요청에서 Pawn의 FX 네트워크 컴포넌트까지 검색
	if (APlayerController* AsController = Cast<APlayerController>(InActor))
	{
		if (APawn* AsPawn = Cast<APawn>(InActor))
		{
			return AsPawn->FindComponentByClass<UPRFXNetworkComponent>();
		}
	}

	return nullptr;
}

void UPRFXNetworkComponent::ServerPlayFX_Implementation(FPRFXRequest Request)
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPRFXSubsystem* FXSubsystem = World->GetSubsystem<UPRFXSubsystem>();
	if (!IsValid(FXSubsystem))
	{
		return;
	}

	FPRFXRegistryEntry Entry;
	if (!FXSubsystem->FindRegistryEntry(Request.FXTag, Entry))
	{
		return;
	}

	// 서버가 주변 클라이언트를 고를 때 사용할 FX 발생 위치 산출
	FVector Origin = FVector::ZeroVector;
	if (!ResolveFXOrigin(Request.Payload, Origin))
	{
		if (AActor* OwnerActor = GetOwner())
		{
			Origin = OwnerActor->GetActorLocation();
		}
	}

	TArray<APlayerController*> RelevantControllers;
	GatherRelevantClients(Origin, Entry.ReplicationRadius, RelevantControllers);

	// 거리 조건을 통과한 클라이언트마다 해당 소유 FX 컴포넌트에 Client RPC 호출
	for (APlayerController* PlayerController : RelevantControllers)
	{
		UPRFXNetworkComponent* ClientComponent = ResolveClientComponent(PlayerController);
		if (IsValid(ClientComponent))
		{
			SendClientFX(ClientComponent, Entry, Request);
		}
	}
}

void UPRFXNetworkComponent::ServerPlayFX_Unreliable_Implementation(FPRFXRequest Request)
{
	ServerPlayFX_Implementation(Request);
}

void UPRFXNetworkComponent::ClientPlayFX_Implementation(FPRFXRequest Request)
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	if (UPRFXSubsystem* FXSubsystem = World->GetSubsystem<UPRFXSubsystem>())
	{
		FXSubsystem->PlayConfirmedFX(Request);
	}
}

void UPRFXNetworkComponent::ClientPlayFX_Unreliable_Implementation(FPRFXRequest Request)
{
	// Unreliable RPC로 받은 요청의 로컬 실행 처리는 Reliable RPC와 동일
	ClientPlayFX_Implementation(Request);
}

void UPRFXNetworkComponent::GatherRelevantClients(FVector Origin, float Radius, TArray<APlayerController*>& OutControllers) const
{
	OutControllers.Reset();

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// Radius가 0 이하이면 모든 PlayerController를 수신 대상으로 해석
	const float RadiusSq = FMath::Square(FMath::Max(0.0f, Radius));
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!IsValid(PlayerController))
		{
			continue;
		}

		FVector ViewLocation = FVector::ZeroVector;
		if (APawn* Pawn = PlayerController->GetPawn())
		{
			// 일반 플레이어의 FX 수신 여부를 Pawn 위치로 판정
			ViewLocation = Pawn->GetActorLocation();
		}
		else
		{
			// Pawn이 없는 관전 또는 전환 상태의 FX 수신 여부를 카메라 위치로 판정
			FRotator UnusedRotation = FRotator::ZeroRotator;
			PlayerController->GetPlayerViewPoint(ViewLocation, UnusedRotation);
		}

		if (Radius <= 0.0f || FVector::DistSquared(Origin, ViewLocation) <= RadiusSq)
		{
			OutControllers.Add(PlayerController);
		}
	}
}

bool UPRFXNetworkComponent::ResolveFXOrigin(const FInstancedStruct& Payload, FVector& OutOrigin) const
{
	if (const FPRFXLocationPayload* LocationPayload = Payload.GetPtr<FPRFXLocationPayload>())
	{
		// 위치 기반 FX가 직접 지정한 월드 위치 사용
		OutOrigin = LocationPayload->Location;
		return true;
	}

	if (const FPRFXAttachPayload* AttachPayload = Payload.GetPtr<FPRFXAttachPayload>())
	{
		if (IsValid(AttachPayload->AttachComponent))
		{
			// 부착 Component가 지정된 FX의 Component 월드 위치 사용
			OutOrigin = AttachPayload->AttachComponent->GetComponentLocation();
			return true;
		}

		if (IsValid(AttachPayload->AttachActor))
		{
			// 부착 Component가 없는 FX의 Actor 월드 위치 사용
			OutOrigin = AttachPayload->AttachActor->GetActorLocation();
			return true;
		}
	}

	if (const FPRFXTrailPayload* TrailPayload = Payload.GetPtr<FPRFXTrailPayload>())
	{
		// Trail FX의 발사 시작점 사용
		OutOrigin = TrailPayload->StartLocation;
		return true;
	}

	if (const FPRFXHitPayload* HitPayload = Payload.GetPtr<FPRFXHitPayload>())
	{
		// 탄착과 피격 FX의 충돌 지점 사용
		OutOrigin = HitPayload->ImpactLocation;
		return true;
	}

	if (const FPRFXActorSpawnPayload* ActorSpawnPayload = Payload.GetPtr<FPRFXActorSpawnPayload>())
	{
		// Actor Spawn형 FX의 Spawn Transform 위치 사용
		OutOrigin = ActorSpawnPayload->SpawnTransform.GetLocation();
		return true;
	}

	if (const FPRFXPersistentPayload* PersistentPayload = Payload.GetPtr<FPRFXPersistentPayload>())
	{
		if (IsValid(PersistentPayload->TargetActor))
		{
			// 지속 FX가 따라갈 대상 Actor의 현재 위치 사용
			OutOrigin = PersistentPayload->TargetActor->GetActorLocation();
			return true;
		}
	}

	return false;
}

UPRFXNetworkComponent* UPRFXNetworkComponent::ResolveClientComponent(APlayerController* PlayerController) const
{
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	if (UPRFXNetworkComponent* ControllerComponent = UPRFXNetworkComponent::FindForActor(PlayerController))
	{
		// Client RPC 소유권이 명확한 PlayerController 부착 컴포넌트 우선
		return ControllerComponent;
	}

	return nullptr;
}

void UPRFXNetworkComponent::SendClientFX(UPRFXNetworkComponent* ClientComponent, const FPRFXRegistryEntry& Entry, const FPRFXRequest& Request) const
{
	if (!IsValid(ClientComponent))
	{
		return;
	}

	if (Entry.bReliableNetworkEvent)
	{
		// 누락되면 안 되는 FX의 Reliable Client RPC 사용
		ClientComponent->ClientPlayFX(Request);
	}
	else
	{
		// 총구 화염이나 Trail처럼 손실 가능한 FX의 Unreliable Client RPC 사용
		ClientComponent->ClientPlayFX_Unreliable(Request);
	}
}
