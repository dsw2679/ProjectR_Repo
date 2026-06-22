// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 상태 변화에 따른 AI 알림 연동)
// Author: 손승우 (아머드 솔저 전투 이동 및 EQS 관련 AI 이벤트 중계 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PREnemyCombatEventRelayComponent.generated.h"

// 기존 BP 서브오브젝트 호환을 위해 유지하는 컴포넌트다.
// 새 적 AI 구조에서는 이 컴포넌트에 의존하지 않고, 상태 이벤트는 ASC/BossBase가 직접 처리한다.
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPREnemyCombatEventRelayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPREnemyCombatEventRelayComponent();

	virtual void BeginPlay() override;

protected:
	// 호환용 컴포넌트가 생성되었는지 확인하기 위한 최소 상태값이다.
	bool bEventsBound = false;
};
