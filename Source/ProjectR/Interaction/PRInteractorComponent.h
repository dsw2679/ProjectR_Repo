// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Interactor 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRInteractorComponent.generated.h"

class IPRInteractionInterface;
class UPRInteractableComponent;
class UPRInteractionAction;

USTRUCT()
struct FPRActiveInteractionInfo
{
	GENERATED_BODY()

	// 현재 유지 중인 InteractableComponent
	UPROPERTY()
	TObjectPtr<UPRInteractableComponent> ActiveInteractable;

	// ActiveInteractable->InteractionActions 내 활성 Action 인덱스 (없으면 INDEX_NONE)
	UPROPERTY()
	int8 ActiveActionIndex = INDEX_NONE;

	bool bSustained = false;
public:
	void Reset();
	bool IsValid() const;
	bool RequiresRange() const;

	// 인덱스로부터 Action 인스턴스 조회 (없으면 nullptr) 
	UPRInteractionAction* GetActiveAction() const;
};

/**
 * Hold 게이트 진행 정보 (로컬 전용, 비복제).
 * 서버: Tick 게이트 검사용 / 클라: Client RPC로 채워져 UI 게이지 보간용.
 */
USTRUCT()
struct FPRHoldInfo
{
	GENERATED_BODY()

	// Hold 트리거 대상
	UPROPERTY()
	TObjectPtr<UPRInteractableComponent> HoldTarget;

	// 대상 InteractionActions 내 후보 Action 인덱스
	UPROPERTY()
	TObjectPtr<UPRInteractionAction> HoldAction;

	// 시작 시각
	UPROPERTY()
	float StartTime = 0.f;

	// 완료까지 필요한 시간 (초)
	UPROPERTY()
	float HoldDuration = 0.f;

	FTimerHandle HoldTimerHandle;
	bool bIsHolding = false;
public:
	void Reset();
	bool IsValid() const;
};

DECLARE_MULTICAST_DELEGATE(FOnInteractionEndSignature);
DECLARE_MULTICAST_DELEGATE(FOnInteractionStartSignature);

/**
 * 상호작용 주체(플레이어)에 부착하는 컴포넌트.
 * 한 번에 하나의 UPRInteractableComponent만 포커스로 관리하며,
 * 상호작용 요청을 현재 포커스 대상에 위임한다.
 */
UCLASS(meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRInteractorComponent();

	/*~ UActorComponent Interface ~*/
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	/**
	 * 액터로부터 포커스 대상을 설정한다.
	 * IPRInteractionInterface를 구현하지 않은 경우 기존 포커스도 해제된다.
	 * (주로 로컬 플레이어 화면 중앙 물체를 Focus)
	 */
	void TryFocus(AActor* Actor);

	// 포커스 대상을 변경한다. 기존 대상에 OnUnfocus, 신규 대상에 OnFocus 호출 
	void SetFocus(UPRInteractableComponent* NewFocus);

	// 포커스를 해제한다 
	void ClearFocus();

	// 현재 포커스 대상에 상호작용을 위임한다 
	void InteractFocused();
	
	// 인터렉션 키의 눌림 해제 상태를 수신, Hold 중인 액션이 있으면 취소하기 위함 
	void OnInteractionReleased();
	
	// 활성 유지형 상호작용을 능동적으로 종료 요청 
	void RequestEndInteract(bool bCanceled);

	// 현재 포커스 대상 반환
	UPRInteractableComponent* GetFocusedComponent() const { return FocusedComponent; }

	// 현재 유효한 포커스 대상이 있는지 여부 반환
	bool HasFocus() const;

	// 대상 액터가 상호작용 최대 거리 이내인지 여부 반환
	bool IsWithinRange(const AActor* TargetActor) const;
	bool IsWithinRange(const UPRInteractableComponent* TargetComponent) const;
	
	// 현재 유지 타입 Action이 설정되었는지
	bool IsSustained() const;

	// 현재 활성 액션 반환
	UPRInteractionAction* GetActiveAction() const { return ActiveInteractionInfo.GetActiveAction(); }

	// 현재 활성 유지형 상호작용 대상 반환
	UPRInteractableComponent* GetActiveInteractableComponent() const { return ActiveInteractionInfo.ActiveInteractable; }

	// 현재 Hold타입 Action이 설정되었는지
	bool IsHolding() const;
	
	// 현재 Hold타입 Action이 설정되어 Hold 취소를 수신해야하는지
	bool ShouldListenInputReleased() const;
	
	// 포커스 대상에서 제외할 액터 목록 설정
	void SetIgnoreActors(const TArray<AActor*>& Actors);
	
protected:
	UFUNCTION(Server, Reliable)
	void ServerEndInteract(bool bCanceled);
	
	UFUNCTION(Client, Reliable)
	void ClientEndInteract();
	
	UFUNCTION(Server, Reliable)
	void ServerInteract(UPRInteractableComponent* Target, int8 ActionIndex);
	
	UFUNCTION(Server, Reliable)
	void ServerCancelHold();
	
	UFUNCTION(Server, Reliable)
	void ServerFinishHold();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartHold(UPRInteractableComponent* Target, int8 ActionIndex);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastCancelHold();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFinishHold();

	// 서버 거부 알림. 클라 선반영 상태 정리 + 종료 델리게이트 broadcast 
	UFUNCTION(Client, Reliable)
	void ClientNotifyDenied();
	
	UFUNCTION()
	void OnRep_ActiveInteractionInfo();
	
private:
	void Internal_HandleInteraction(UPRInteractableComponent* Target, int32 ActionIndex);
	
	// 홀드 시작, 서버 권위에서 실행
	void Internal_StartHold(UPRInteractableComponent* Target, int32 ActionIndex);
	
	// 이전 상호작용 정보 초기화
	void ClearPreviousInteraction();
	
	// 활성 유지형 상호작용을 종료한다
	void Internal_EndActiveInteraction(bool bCanceled);

	// 활성 유지형 상호작용 추적을 시작한다
	void Internal_SetActiveInteraction(UPRInteractionAction* Action, UPRInteractableComponent* Interactable);

	// 홀드 설정
	void SetHoldInfo(UPRInteractableComponent* Target, UPRInteractionAction* HoldAction);
	
	// 홀드 정보 초기화
	void ClearHoldInfo();
	
	UFUNCTION()
	void OnHoldFinished();

public:
	// 상호작용 최대 거리 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float MaxInteractionDistance = 300.f;
	
	// 유지형 상호작용 종료 알림
	FOnInteractionEndSignature OnInteractionEnd;

	// 유지형 상호작용 시작 알림
	FOnInteractionStartSignature OnInteractionStart;

	// Hold 상호작용 종료 알림
	FOnInteractionEndSignature OnHoldEnd;
	
private:
	// 현재 포커스 중인 상호작용 컴포넌트
	UPROPERTY()
	TObjectPtr<UPRInteractableComponent> FocusedComponent;

	// 현재 홀드 중인 상호작용 정보
	UPROPERTY()
	FPRHoldInfo HoldInfo;
	
	// 포커스에서 제외할 액터 목록
	TArray<TWeakObjectPtr<AActor>> IgnoreActors;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveInteractionInfo)
	FPRActiveInteractionInfo ActiveInteractionInfo;
};
