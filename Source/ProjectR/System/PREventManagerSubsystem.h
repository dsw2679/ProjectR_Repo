// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Event 매니저 서브시스템 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "PREventTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Subsystems/WorldSubsystem.h"
#include "PREventManagerSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPREvent, Log, All);

// 이벤트 콜백 시그니처. 페이로드는 FInstancedStruct로 전달되며, 콜백 측에서 GetPtr<T>()로 캐스팅한다
DECLARE_MULTICAST_DELEGATE_TwoParams(FPREventMulticast, FGameplayTag /*EventTag*/, const FInstancedStruct& /*Payload*/);

/**
 * 월드 스코프의 광역 이벤트 버스.
 * EventTag 기반으로 리스너를 등록하고, FInstancedStruct 기반의 페이로드를 브로드캐스트한다.
 * 월드와 함께 생성/소멸되므로 레벨 전환/체크포인트 리셋 시 리스너가 자동 정리된다.
 *
 * 사용 흐름:
 *   1) USTRUCT로 FPREventPayload 파생 페이로드 정의
 *   2) Listen으로 EventTag 구독
 *   3) Broadcast(또는 BroadcastTyped<T>)로 발송
 *   4) 콜백 내부에서 Payload.GetPtr<TConcrete>()로 캐스팅
 */
UCLASS()
class PROJECTR_API UPREventManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/*~ UWorldSubsystem Interface ~*/
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	// EventTag에 리스너를 등록한다. 반환된 핸들은 Unlisten에서 사용
	FDelegateHandle Listen(FGameplayTag EventTag, FPREventMulticast::FDelegate Callback);

	// 핸들을 사용해 특정 EventTag의 리스너를 해제한다
	void Unlisten(FGameplayTag EventTag,FDelegateHandle& InHandle);

	// 모든 등록된 EventTag에서 해당 핸들을 제거한다 (콜백 객체 소멸 시 정리용)
	void UnlistenAll(FDelegateHandle& InHandle);

	// 페이로드와 함께 EventTag를 브로드캐스트한다. 부모 태그 매칭 동작은 옵션으로 처리
	void Broadcast(FGameplayTag EventTag, const FInstancedStruct& Payload);

	// 페이로드 없는 가벼운 신호용. 내부적으로 빈 FInstancedStruct를 전달
	void BroadcastEmpty(FGameplayTag EventTag);

	// 템플릿 편의 함수. USTRUCT 페이로드를 자동으로 FInstancedStruct로 래핑해 발송
	template <typename TPayload>
	void BroadcastTyped(FGameplayTag EventTag, const TPayload& PayloadStruct)
	{
		static_assert(TIsDerivedFrom<TPayload, FPREventPayload>::IsDerived,
			"TPayload must be derived from FPREventPayload.");
		Broadcast(EventTag, FInstancedStruct::Make<TPayload>(PayloadStruct));
	}

private:
	// EventTag별 멀티캐스트 델리게이트 맵
	TMap<FGameplayTag, FPREventMulticast> Listeners;
};
