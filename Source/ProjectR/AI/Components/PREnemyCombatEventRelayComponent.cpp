// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 상태 변화에 따른 AI 알림 연동)
// Author: 손승우 (아머드 솔저 전투 이동 및 EQS 관련 AI 이벤트 중계 구현)
#include "PREnemyCombatEventRelayComponent.h"

UPREnemyCombatEventRelayComponent::UPREnemyCombatEventRelayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UPREnemyCombatEventRelayComponent::BeginPlay()
{
	Super::BeginPlay();

	// 새 적 AI 구조에서는 상태 이벤트를 직접 중계하지 않는다.
	// 기존 BP 서브오브젝트 참조가 끊기지 않도록 호환용 껍데기만 유지한다.
	bEventsBound = true;
}
