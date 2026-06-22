// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (UI World 핑/마커 타입 및 구조체 정의)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/System/PREventTypes.h"
#include "PRWorldMarkerTypes.generated.h"

class APlayerController;
class APlayerState;
class AActor;
class UTexture2D;

UENUM(BlueprintType)
enum class EPRWorldMarkerEventType : uint8
{
	Added,
	Removed,
};

UENUM(BlueprintType)
enum class EPRWorldMarkerLocationMode : uint8
{
	FixedWorldLocation,
	InterfaceTarget,
};

// 마커 표시 스타일
USTRUCT(BlueprintType)
struct PROJECTR_API FPRWorldMarkerVisualData
{
	GENERATED_BODY()

	FPRWorldMarkerVisualData();

	// 표시 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText MarkerName;

	// 표시 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> Icon;

	// 표시 색상
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color = FLinearColor::White;

	// 표시 아이콘 크기
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D IconSize = FVector2D(48.0f, 48.0f);
};

// 마커 생성 요청 데이터
USTRUCT(BlueprintType)
struct PROJECTR_API FPRWorldMarkerRequest
{
	GENERATED_BODY()

	// 요청 플레이어 컨트롤러
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<APlayerController> RequestingController = nullptr;

	// 추적 대상 액터
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> TargetActor = nullptr;

	// 고정 또는 폴백 월드 위치
	UPROPERTY(BlueprintReadWrite)
	FVector_NetQuantize WorldLocation = FVector::ZeroVector;
};

// 화면 표시용 마커 데이터
USTRUCT(BlueprintType)
struct PROJECTR_API FPRWorldMarkerData
{
	GENERATED_BODY()

	// 마커 식별자
	UPROPERTY(BlueprintReadOnly)
	FGuid MarkerId;

	// 생성자 PlayerState
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APlayerState> SourcePlayerState = nullptr;

	// 위치 갱신 방식
	UPROPERTY(BlueprintReadOnly)
	EPRWorldMarkerLocationMode LocationMode = EPRWorldMarkerLocationMode::FixedWorldLocation;

	// 추적 대상 액터
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> TargetActor = nullptr;

	// 고정 또는 폴백 월드 위치
	UPROPERTY(BlueprintReadOnly)
	FVector WorldLocation = FVector::ZeroVector;

	// 표시 스타일
	UPROPERTY(BlueprintReadOnly)
	FPRWorldMarkerVisualData VisualData;

	// 유지 시간
	UPROPERTY(BlueprintReadOnly)
	float Duration = 6.0f;

	// 서버 기준 생성 시간
	UPROPERTY(BlueprintReadOnly)
	float ServerWorldTime = 0.0f;
};

// 월드 마커 이벤트 페이로드
USTRUCT(BlueprintType)
struct PROJECTR_API FPRWorldMarkerEventPayload : public FPREventPayload
{
	GENERATED_BODY()

	// 이벤트 종류
	UPROPERTY(BlueprintReadOnly)
	EPRWorldMarkerEventType EventType = EPRWorldMarkerEventType::Added;

	// 표시할 마커 데이터
	UPROPERTY(BlueprintReadOnly)
	FPRWorldMarkerData MarkerData;

	// 제거 대상 식별자
	UPROPERTY(BlueprintReadOnly)
	FGuid MarkerId;
};

// 활성 마커 식별자 목록
USTRUCT()
struct PROJECTR_API FPRActiveWorldMarkerList
{
	GENERATED_BODY()

	// 생성 순서 기준 마커 식별자
	UPROPERTY()
	TArray<FGuid> MarkerIds;
};
