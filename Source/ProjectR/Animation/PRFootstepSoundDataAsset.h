// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (Footstep 사운드 데이터 에셋 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "PRFootstepSoundDataAsset.generated.h"

class USoundAttenuation;
class USoundBase;
class USoundConcurrency;

UENUM(BlueprintType)
enum class EPRFootstepFoot : uint8
{
	Left,
	Right,
};

UENUM(BlueprintType)
enum class EPRFootstepMoveType : uint8
{
	Crouch,
	Walk,
	Jog,
	Sprint,
};

// 표면 하나에 대응하는 발소리 설정
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFootstepSoundEntry
{
	GENERATED_BODY()

	// 재생할 사운드 에셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Audio")
	TObjectPtr<USoundBase> Sound = nullptr;

	// 재생 볼륨 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Audio", meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	// 재생 피치 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Audio", meta = (ClampMin = "0.1"))
	float PitchMultiplier = 1.0f;

	// AI 청각 자극 세기 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|AI", meta = (ClampMin = "0.0"))
	float NoiseLoudnessMultiplier = 1.0f;
};

// 이동 방식 하나에 대응하는 발소리 배율
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFootstepMoveModifier
{
	GENERATED_BODY()

	FPRFootstepMoveModifier() = default;

	FPRFootstepMoveModifier(float InVolumeMultiplier, float InPitchMultiplier, float InNoiseLoudnessMultiplier)
		: VolumeMultiplier(InVolumeMultiplier)
		, PitchMultiplier(InPitchMultiplier)
		, NoiseLoudnessMultiplier(InNoiseLoudnessMultiplier)
	{
	}

	// 재생 볼륨 추가 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Move", meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	// 재생 피치 추가 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Move", meta = (ClampMin = "0.1"))
	float PitchMultiplier = 1.0f;

	// 이동 방식별 3D 재생 감쇠 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Move")
	TObjectPtr<USoundAttenuation> AttenuationSettings = nullptr;

	// 이동 방식별 동시 재생 제한 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Move")
	TObjectPtr<USoundConcurrency> ConcurrencySettings = nullptr;

	// AI 청각 자극 세기 추가 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Move", meta = (ClampMin = "0.0"))
	float NoiseLoudnessMultiplier = 1.0f;
};

// 표면 재질별 발소리와 발 소켓 이름을 담는 데이터 에셋
UCLASS(BlueprintType)
class PROJECTR_API UPRFootstepSoundDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 발 구분에 대응하는 소켓 이름 반환
	FName ResolveFootSocketName(EPRFootstepFoot Foot) const;

	// 표면 타입에 대응하는 발소리 설정 반환
	const FPRFootstepSoundEntry* FindSoundEntry(EPhysicalSurface SurfaceType) const;

	// 이동 방식에 대응하는 발소리 배율 반환
	const FPRFootstepMoveModifier& ResolveMoveModifier(EPRFootstepMoveType MoveType) const;

public:
	// 매핑 실패 또는 기본 표면용 발소리 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Audio")
	FPRFootstepSoundEntry DefaultEntry;

	// 표면 타입별 발소리 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Audio")
	TMap<TEnumAsByte<EPhysicalSurface>, FPRFootstepSoundEntry> SurfaceEntries;

	// 왼발 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Socket")
	FName LeftFootSocketName = NAME_None;

	// 오른발 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Socket")
	FName RightFootSocketName = NAME_None;

	// 앉기 이동 발소리 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Move")
	FPRFootstepMoveModifier CrouchModifier = { 0.35f, 1.0f, 0.2f };

	// 걷기 이동 발소리 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Move")
	FPRFootstepMoveModifier WalkModifier = { 0.65f, 1.0f, 0.6f };

	// 조깅 이동 발소리 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Move")
	FPRFootstepMoveModifier JogModifier = { 1.0f, 1.0f, 1.0f };

	// 질주 이동 발소리 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Move")
	FPRFootstepMoveModifier SprintModifier = { 1.25f, 1.0f, 1.5f };

	// 트레이스 실패 시 기본 사운드 재생 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Audio")
	bool bPlayDefaultWhenTraceMissed = true;
};
