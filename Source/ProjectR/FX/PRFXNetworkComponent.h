// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (이펙트 Network 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRFXTypes.h"
#include "Components/ActorComponent.h"
#include "StructUtils/InstancedStruct.h"
#include "PRFXNetworkComponent.generated.h"

class APlayerController;

// FX 네트워크 요청과 전파 대상 선별을 담당하는 액터 컴포넌트
UCLASS(ClassGroup=(FX), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRFXNetworkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRFXNetworkComponent();

	// 지정 액터 또는 액터의 Pawn/Controller 문맥에서 FX 네트워크 컴포넌트 검색
	static UPRFXNetworkComponent* FindForActor(AActor* InActor);

	// 서버에 FX 전파 요청
	UFUNCTION(Server, Reliable)
	void ServerPlayFX(FPRFXRequest Request);
	
	// 서버에 FX 전파 요청 (Unreliable)
	UFUNCTION(Server, Unreliable)
	void ServerPlayFX_Unreliable(FPRFXRequest Request);

	// 서버가 특정 클라이언트에게 FX 재생 명령 전달
	UFUNCTION(Client, Reliable)
	void ClientPlayFX(FPRFXRequest Request);

	// 서버가 특정 클라이언트에게 전달 보장 없이 FX 재생 명령 전달
	UFUNCTION(Client, Unreliable)
	void ClientPlayFX_Unreliable(FPRFXRequest Request);

protected:
	// 서버가 거리 기반으로 전파 대상 PlayerController 선별
	void GatherRelevantClients(FVector Origin, float Radius, TArray<APlayerController*>& OutControllers) const;

	// Payload에서 전파 기준 위치 추출
	bool ResolveFXOrigin(const FInstancedStruct& Payload, FVector& OutOrigin) const;

	// PlayerController 또는 Pawn에서 클라이언트 RPC를 받을 FX 컴포넌트 검색
	UPRFXNetworkComponent* ResolveClientComponent(APlayerController* PlayerController) const;

	// Registry 신뢰도 정책에 맞는 Client RPC 호출
	void SendClientFX(UPRFXNetworkComponent* ClientComponent, const FPRFXRegistryEntry& Entry, const FPRFXRequest& Request) const;
};
