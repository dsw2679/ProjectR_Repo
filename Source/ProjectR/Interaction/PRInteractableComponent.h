// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Interactable 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/System/PREventTypes.h"
#include "PRInteractableComponent.generated.h"

class UPRInteractionAction;
class UPRInteractorComponent;
class UMeshComponent;

/**
 * 상호작용 프롬프트 이벤트 페이로드.
 */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRInteractableEventPayload : public FPREventPayload
{
	GENERATED_BODY()

	// 프롬프트 표시 여부: true = 표시, false = 숨김
	UPROPERTY(BlueprintReadWrite)
	bool bShowPrompt = false;
	
	// 상호작용 가능 여부: false면 반투명 처리
	UPROPERTY(BlueprintReadWrite)
	bool bCanInteract = false;
	
	// 상호작용 힌트 텍스트 (예: "줍기")
	UPROPERTY(BlueprintReadWrite)
	FText ActionHintText;
};

/**
 * 상호작용 가능한 오브젝트에 부착하는 컴포넌트.
 * UPRInteractionAction 목록을 보유하며, OnInteract 시 조건을 만족하는
 * 가장 높은 우선순위의 행동을 선택하여 실행한다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRInteractableComponent();

	/*~ UActorComponent Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// 포커스 진입 시 호출. 소유 액터의 모든 메시에 오버레이 머티리얼 적용
	virtual void OnFocus(AActor* Viewer, bool bIsInRange);
	
	// 이미 포커스된 상태에서 업데이트
	virtual void UpdateFocus(AActor* Viewer, bool bIsInRange);

	// 포커스 해제 시 호출. 소유 액터의 모든 메시에서 오버레이 머티리얼 제거
	virtual void OnUnfocus();

	// 상호작용 시 호출. 조건을 만족하는 최고 우선순위 행동을 실행
	virtual void OnInteract(AActor* Interactor, int32 ActionIndex);

	// 조건을 만족하는 최고 우선순위 Action 선택 (실행은 안 함). 없으면 nullptr
	virtual UPRInteractionAction* SelectBestAction(AActor* Interactor) const;

	// 실행 가능한 행동이 하나라도 있는지 여부 반환
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool HasAvailableAction(AActor* Interactor) const;

	// 활성 유지형 상호작용을 종료
	void EndActiveInteraction(AActor* Interactor, bool bCanceled);

	// 현재 활성 Action 반환 (nullptr이면 비활성)
	UPRInteractionAction* GetActiveAction() const { return ActiveAction; }

	// Index로부터 Action 반환 (nullptr이면 InValid Index)
	UPRInteractionAction* FindActionByIndex(int32 InActionIndex) const;
	
	int32 FindActionIndex(UPRInteractionAction* InAction) const;
	
	// 현재 사용 중인 Interactor 반환 (없으면 nullptr)
	AActor* GetCurrentInteractor() const { return CurrentInteractor; }

	// 현재 사용 중 여부 반환 (배타 점유 사용 시에만 의미 있음)
	bool IsBeingInteracted() const { return ::IsValid(CurrentInteractor); }

	// 배타 점유 사용 여부 반환
	bool IsExclusive() const { return bExclusiveInteraction; }

	// 주어진 Interactor가 점유 정책상 진입 가능한지 여부 반환
	bool CanBeInteractedBy(AActor* Interactor) const;

	// 배타 점유 시 현재 Interactor를 기록
	void AcquireExclusive(AActor* Interactor);

	// 배타 점유 해제
	void ReleaseExclusive();

	// 아웃라인 적용 여부
	virtual bool ShouldApplyDepthStencilValue(AActor* Viewer) const;

	// 아웃라인 적용
	void ApplyDepthStencilValues(bool bIsInRange);

	// 아웃라인 적용 중 여부
	bool IsDepthStencilApplied() const { return bDepthStencilApplied; }
	
	// 아웃라인 해제
	void ResetDepthStencilValues();
	
	// 홀딩 상태 설정
	void SetIsHolding(const bool bInIsHolding) { bIsHolding = bInIsHolding; }
	
	// 홀딩 여부
	bool IsHolding() const {return bIsHolding; }
	
	FVector GetActorLocation() const;
protected:
	UFUNCTION()
	void OnRep_CurrentInteractor();

private:
	// EventManager 로 상호작용 프롬프트 이벤트(bShowPrompt + bCanInteract + ActionHintText) 브로드캐스트
	void BroadcastInteractableEvent(AActor* Viewer, bool bIsInRange) const;

public:
	// 이 컴포넌트에 등록된 상호작용 행동 목록
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Interaction")
	TArray<TObjectPtr<UPRInteractionAction>> InteractionActions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText FallbackDisplayName;

	// 동시에 한 명만 상호작용 가능 여부. true면 CurrentInteractor로 점유 추적
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bExclusiveInteraction = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bOnlyApplyDepthStencilOnAvailable = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float InteractionRangeScale = 1.0f;
	
private:
	// 현재 포커스 상태 여부
	bool bIsFocused = false;
	
	// 현재 홀딩 여부
	bool bIsHolding = false;

	// Depth Stencil 적용 여부
	bool bDepthStencilApplied = false;
	
	// 포커스 진입 전 메시별 RenderCustomDepth 원래 값 (복원용)
	UPROPERTY()
	TMap<UMeshComponent*, bool> SavedCustomDepthStates;

	// 현재 사용 중인 Interactor (없으면 nullptr)
	UPROPERTY(ReplicatedUsing = OnRep_CurrentInteractor)
	TObjectPtr<AActor> CurrentInteractor;

	// 현재 활성 유지형 Action (서버 전용 추적값)
	UPROPERTY()
	TObjectPtr<UPRInteractionAction> ActiveAction;
};