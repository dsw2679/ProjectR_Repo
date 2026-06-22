// Author: 김동석 (사격 반동 벌어짐 연동용 크로스헤어 타입 정의)
// Author: 배유찬 (사격 연동 크로스헤어 상태 타입 정의)
#pragma once
#include "ProjectR/System/PREventTypes.h"
#include "ProjectR/ItemSystem/Types/PRRecoilTypes.h"
#include "PRCrosshairTypes.generated.h"

UENUM(BlueprintType)
enum class EPRCrosshairType : uint8
{
	Type_1,
	Type_2,
	Type_3,
	Type_4,
	Type_5,
	Type_6,
};


/**
 * 사격 반동 이벤트 페이로드. Event.Player.Recoil 발송 시 사용.
 */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRRecoilEventPayload : public FPREventPayload
{
	GENERATED_BODY()

	// 반동 회복 속도(또는 적용 속도). 위젯/카메라가 보간 강도로 사용
	UPROPERTY(BlueprintReadWrite)
	float Speed = 0.f;

	// 반동 강도
	UPROPERTY(BlueprintReadWrite)
	float Strength = 0.f;

	// 발사 시점의 무기 반동 프로파일
	UPROPERTY(BlueprintReadWrite)
	FPRRecoilProfile RecoilProfile;

	// 현재 입력 유지 중 누적된 연속 발사 횟수
	UPROPERTY(BlueprintReadWrite)
	int32 ConsecutiveShots = 0;

	// 발사 시점에 ADS 상태였는지 여부
	UPROPERTY(BlueprintReadWrite)
	bool bIsAiming = false;
};

class UPRCrosshairConfig;

/**
 * 히트스캔 미리보기 적중 이벤트 페이로드. Event.Player.PreviewHit 발송용
 */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRPreviewHitEventPayload : public FPREventPayload
{
	GENERATED_BODY()

	// 현재 조준선의 Combat 채널 적중 여부
	UPROPERTY(BlueprintReadWrite)
	bool bHit = false;
};

/**
 * 크로스헤어 Config 교체 이벤트 페이로드. Event.Player.ChangeCrosshair 발송 시 사용.
 */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRChangeCrosshairEventPayload : public FPREventPayload
{
	GENERATED_BODY()

	// 적용할 크로스헤어 Config. nullptr 이면 크로스헤어 초기화/해제 신호로 처리 가능
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<const UPRCrosshairConfig> Config = nullptr;
};
