// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Interaction Action 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/System/PREventTypes.h"
#include "UObject/Object.h"
#include "PRInteractionAction.generated.h"

class UPRInteractableComponent;
class AController;
class APawn;

/**
 * Hold 상호작용 단계.
 * Start: 입력 유지 시작 / Canceled: 도중 해제·중단 / Finished: 게이트 완료
 */
UENUM(BlueprintType)
enum class EPRInteractionHoldPhase : uint8
{
	Start,
	Canceled,
	Finished,
};


/**
 * Hold 상호작용 이벤트 페이로드.
 * Event.Player.Interaction.Hold 발송 시 동반되며, Phase 로 단계 구분.
 */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRInteractionHoldEventPayload : public FPREventPayload
{
	GENERATED_BODY()

	// 현재 단계 (Start/Canceled/Finished)
	UPROPERTY(BlueprintReadWrite)
	EPRInteractionHoldPhase Phase = EPRInteractionHoldPhase::Start;

	// Hold 완료까지 필요한 시간 (초). Start 단계에서만 의미가 있다
	UPROPERTY(BlueprintReadWrite)
	float HoldDuration = 0.f;
	
	// 상호작용 이름
	UPROPERTY(BlueprintReadWrite)
	FText ActionName;
};

/**
 * 상호작용 유형.
 * Instant: 즉발형 — Execute 한 번 호출로 완료. 추적 불필요 (기본값)
 * Sustained: 유지형 — Execute로 시작, EndInteraction으로 종료. 활성 상태 추적 대상
 */
UENUM(BlueprintType)
enum class EPRInteractionType : uint8
{
	// 즉발형: Execute 한 번 호출로 완료
	Instant,

	// 유지형: Execute로 시작, EndInteraction으로 종료 (대화 등)
	Sustained,
};

/**
 * 트리거 방식.
 * Tap: 단일 입력으로 즉시 트리거
 * Hold: HoldDuration 만큼 입력 유지 후 트리거 (도중 해제 시 미발동)
 */
UENUM(BlueprintType)
enum class EPRTriggerType : uint8
{
	// 즉시 트리거
	Tap,

	// HoldDuration 만큼 입력 유지 시 트리거
	Hold,
};

/**
 * 상호작용 행동 베이스 클래스.
 * UPRInteractableComponent에 추가되며, OnInteract 시 조건에 맞는 최우선 행동이 실행된다.
 * Interactor는 주로 PlayerController가 되므로 주의
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced, Abstract)
class PROJECTR_API UPRInteractionAction : public UObject
{
	GENERATED_BODY()

public:
	/** 상호작용 대상 (InteractableComponent 소유자) 반환 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	AActor* GetOwner() const;
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	UPRInteractableComponent* GetOwnerInteractable() const;

	/** Interactor로부터 Interactor Pawn 반환 */
	APawn* GetPawn(AActor* Interactor) const;
	
	/** Interactor로부터 Interactor Controller 반환 */
	AController* GetController(AActor* Interactor) const;
	
	/** 이 행동이 현재 실행 가능한지 여부 반환 */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;
	virtual bool CanInteract_Implementation(AActor* Interactor) const;

	/** 상호작용 행동 실행. Sustained인 경우 bIsActive를 true로 설정 */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void Execute(AActor* Interactor);
	virtual void Execute_Implementation(AActor* Interactor);

	/** 유지형 상호작용 종료. 하위 클래스에서 정리 로직 override */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void EndInteraction(AActor* Interactor, bool bCanceled);
	virtual void EndInteraction_Implementation(AActor* Interactor, bool bCanceled);

	/** Hold 게이트 시작 시 클라측 피드백 (UI/VFX/SFX 시작) */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnHoldStart(AActor* Interactor);
	virtual void OnHoldStart_Implementation(AActor* Interactor);

	/** Hold 게이트 취소 시 클라측 피드백 (UI/VFX/SFX 정리) */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnHoldCanceled(AActor* Interactor);
	virtual void OnHoldCanceled_Implementation(AActor* Interactor);

	/** Hold 게이트 완료 시 클라측 피드백 (성공 펄스, Execute 직전) */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnHoldFinished(AActor* Interactor);
	virtual void OnHoldFinished_Implementation(AActor* Interactor);

	/** TriggerType == Hold 여부 반환 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool IsHoldTrigger() const { return TriggerType == EPRTriggerType::Hold; }

	/** 우선순위 반환. 값이 높을수록 먼저 선택된다 */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	int32 GetPriority() const;
	virtual int32 GetPriority_Implementation() const;

	/** 현재 상호작용 활성 상태 여부 반환 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool IsActive(AActor* Interactor) const;

	/** InteractionType == Sustained 여부 반환 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool ShouldSustained() const { return InteractionType == EPRInteractionType::Sustained; }

	/** 힌트 프롬프트 표시 가능 여부 반환 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool ShouldShowHint(AActor* Viewer) const;

protected:
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsLocalPlayer(AActor* Interactor) const;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText ActionName;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText ActionHintText;

	// 포커스 시 힌트 프롬프트 표시 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bShowHint = true;

	// 힌트 프롬프트를 화면 중앙 반경 안에서만 표시할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction",
	          meta = (EditCondition = "bShowHint"))
	bool bOnlyShowHintNearScreenCenter = false;

	// 화면 짧은 변 기준 중앙 허용 반경 비율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction",
	          meta = (EditCondition = "bShowHint && bOnlyShowHintNearScreenCenter", ClampMin = "0.0", ClampMax = "1.0"))
	float HintScreenCenterRadiusRatio = 0.3f;
	
	// 이 행동의 기본 우선순위. 하위 클래스에서 재정의 가능
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	int32 Priority = 0;

	// 상호작용 유형 (즉발형/유지형)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	EPRInteractionType InteractionType = EPRInteractionType::Instant;

	// 거리 유지 필요 여부. true면 InteractorComponent가 Tick에서 거리 체크
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bRequiresRange = false;

	// 트리거 방식 (Tap: 즉시, Hold: 입력 유지)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	EPRTriggerType TriggerType = EPRTriggerType::Tap;

	// Hold 트리거 시 입력 유지 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction",
	          meta = (EditCondition = "TriggerType == EPRTriggerType::Hold", ClampMin = "0.0"))
	float HoldDuration = 1.f;

private:
	// EventManager 로 Hold 이벤트(Phase + HoldDuration) 브로드캐스트
	void BroadcastHoldEvent(EPRInteractionHoldPhase Phase) const;

protected:
	TArray<TWeakObjectPtr<AActor>> ActiveInteractors;
};
