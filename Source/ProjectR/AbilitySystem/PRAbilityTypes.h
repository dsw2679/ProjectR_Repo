// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (조준 상태 관련 데이터 구조 정의)
// Author: 배유찬 (태그 이벤트 훅 및 어빌리티 기초 데이터 타입 정의)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AttributeSet.h"
#include "PRAbilityTypes.generated.h"

// GAS 표준 Attribute 접근자. Get/Set/Init/GetAttribute 쌍을 자동 생성
#define PR_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

// 캐릭터 역할 구분. Registry가 Role별 StatTable/Row를 분리 관리
UENUM(BlueprintType)
enum class EPRCharacterRole : uint8
{
	// 플레이어 캐릭터
	Player	UMETA(DisplayName = "Player"),
	// 일반 적
	Enemy	UMETA(DisplayName = "Enemy"),
	// 보스. 일반 적과 AttributeSet은 공유, Stat DT만 분리
	Boss	UMETA(DisplayName = "Boss")
};

// 어빌리티 활성화 정책.
UENUM(BlueprintType)
enum class EPRAbilityActivationPolicy : uint8
{
	None,
	// 부여 즉시 활성 (Passive)
	OnGiven				UMETA(DisplayName = "OnGiven"),
	// 플레이어 입력 Pressed 트리거
	OnInputTriggered	UMETA(DisplayName = "OnInputTriggered"),
	// 플레이어 입력 유지 중 지속 활성 (자동사격)
	WhileInputHeld		UMETA(DisplayName = "WhileInputHeld"),
	// 서버 코드/BT Task가 직접 호출
	ServerInvoked		UMETA(DisplayName = "ServerInvoked")
};

// AttributeSet Attribute 변경 직후 발행. 구독: UI/HUD, 연쇄 반응
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPRAttributeChangedSignature, FGameplayAttribute, Attribute, float, OldValue, float, NewValue);

// ASC 어빌리티 활성화 직후 발행. 구독: BT Task, UI, 사운드
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRAbilityActivatedSignature, FGameplayTag, AbilityTag);

// ASC 어빌리티 종료 시 발행. bCancelled == true 면 Cancel 경로
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPRAbilityEndedSignature, FGameplayTag, AbilityTag, bool, bCancelled);

// Enemy AttributeSet GroggyGauge Max 도달 시 발행
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRGroggyStateChangedSignature, bool, bEntered);

// Common AttributeSet Health 고갈 시 발행. Instigator는 최종 타격 소스
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPRDeathSignature,AActor*, Instigator);

// 태그 스택 변경 이벤트 (신규 추가 or 완전 제거)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGameplayTagUpdatedSignature, const FGameplayTag& /**Tag*/, bool /**TagExists*/)
