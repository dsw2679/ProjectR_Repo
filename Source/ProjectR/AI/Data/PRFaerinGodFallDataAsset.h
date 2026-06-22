// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 God Fall 데이터 에셋 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectR/AI/PREnemyAITypes.h"
#include "PRFaerinGodFallDataAsset.generated.h"

class APRFaerinGodFallStaticSwordActor;
class UAnimMontage;
class UAnimSequenceBase;
class UGameplayEffect;
class UNiagaraSystem;
class USoundBase;

// God Fall 본체 시전 중 지정 시각에 몸에 붙여 재생할 Niagara cue다.
USTRUCT(BlueprintType)
struct FPRFaerinGodFallBodyNiagaraCue
{
	GENERATED_BODY()

	// 재생할 Niagara 시스템이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|BodyVFX")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	// God Fall 시전 시작 후 이 Niagara를 재생할 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|BodyVFX", meta = (ClampMin = "0.0"))
	float StartDelaySeconds = 0.0f;

	// 보스 Mesh에 붙일 소켓 이름이다. None이면 Mesh 기준으로 붙는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|BodyVFX")
	FName AttachSocketName = NAME_None;

	// AttachSocket 기준 상대 위치 보정이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|BodyVFX")
	FVector LocationOffset = FVector::ZeroVector;

	// AttachSocket 기준 상대 회전 보정이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|BodyVFX")
	FRotator RotationOffset = FRotator::ZeroRotator;

	// Niagara 상대 스케일이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|BodyVFX")
	FVector Scale = FVector::OneVector;

	// 0보다 크면 해당 시간이 지난 뒤 강제로 정리한다. 0이면 God Fall 본체 연출 종료/취소 시 정리한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|BodyVFX", meta = (ClampMin = "0.0"))
	float LifeSeconds = 0.0f;
};


// God Fall Entry Orbit 회전 제어 방식이다.
UENUM(BlueprintType)
enum class EPRFaerinGodFallEntryOrbitSpinControlMode : uint8
{
	SpeedRamp	UMETA(DisplayName = "Speed Ramp"),
	TotalTurns	UMETA(DisplayName = "Total Turns")
};

// God Fall 검 메시의 기준 local 축이다.
UENUM(BlueprintType)
enum class EPRFaerinGodFallLocalAxis : uint8
{
	PlusX	UMETA(DisplayName = "+X"),
	MinusX	UMETA(DisplayName = "-X"),
	PlusY	UMETA(DisplayName = "+Y"),
	MinusY	UMETA(DisplayName = "-Y"),
	PlusZ	UMETA(DisplayName = "+Z"),
	MinusZ	UMETA(DisplayName = "-Z")
};

// God Fall Entry Orbit 연출에서 최소/최대 랜덤값을 지정하기 위한 float range다.
USTRUCT(BlueprintType)
struct FPRFaerinGodFallFloatRange
{
	GENERATED_BODY()

	FPRFaerinGodFallFloatRange() = default;
	FPRFaerinGodFallFloatRange(const float InMin, const float InMax)
		: Min(InMin)
		, Max(InMax)
	{
	}

	// 랜덤으로 뽑을 최소값이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Range", meta = (ClampMin = "0.0"))
	float Min = 0.0f;

	// 랜덤으로 뽑을 최대값이다. Min보다 작으면 코드에서 자동으로 정렬해 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Range", meta = (ClampMin = "0.0"))
	float Max = 0.0f;
};

// Faerin God Fall의 맵 배치 검 Rig 전환, 본체 연출, StaticSword 충전/배정 루프를 관리하는 데이터다.
UCLASS(BlueprintType)
class PROJECTR_API UPRFaerinGodFallDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/*~ UPrimaryDataAsset Interface ~*/
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
	// 맵 배치 Faerin_Swords_Rig에서 StaticSword 원위치로 사용할 bone 이름 10개다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Rig")
	TArray<FName> SwordBoneNames;

	// Bone 위치마다 생성할 StaticSword Actor 클래스다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|StaticSword")
	TSubclassOf<APRFaerinGodFallStaticSwordActor> StaticSwordActorClass;

	// 맵 배치 rig가 검을 기울이는 동안 재생할 애니메이션이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Rig|Animation")
	TObjectPtr<UAnimSequenceBase> RigChargeAnimation;

	// 맵 배치 rig가 검을 위로 끌어올릴 때 재생할 애니메이션이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Rig|Animation")
	TObjectPtr<UAnimSequenceBase> RigTiltPullAnimation;

	// Rig Charge 애니메이션 재생 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Rig|Animation", meta = (ClampMin = "0.01"))
	float RigChargePlayRate = 1.0f;

	// Rig TiltPull 애니메이션 재생 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Rig|Animation", meta = (ClampMin = "0.01"))
	float RigTiltPullPlayRate = 1.0f;

	// Faerin 본체 Charge 몽타주다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Animation")
	TObjectPtr<UAnimMontage> BossBodyChargeMontage;

	// Faerin 본체 TiltPull 몽타주다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Animation")
	TObjectPtr<UAnimMontage> BossBodyTiltPullMontage;

	// Faerin 본체 Drop 몽타주다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Animation")
	TObjectPtr<UAnimMontage> BossBodyDropMontage;

	// Faerin 본체 몽타주 공통 재생 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Animation", meta = (ClampMin = "0.01"))
	float BossBodyMontagePlayRate = 1.0f;

	// Phase2 진입 시 God Fall 시전 위치까지 이동하는 속도다. Sprint가 아닌 일반 이동 연출이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.0"))
	float BossCastMoveSpeed = 560.0f;

	// God Fall 시전 위치 도착 판정 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.0"))
	float BossCastAcceptanceRadius = 80.0f;

	// Charge 동안 Faerin 본체가 상승할 높이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.0"))
	float BossChargeRiseHeight = 600.0f;

	// Charge 동안 상승과 360도 회전을 끝낼 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.01"))
	float BossChargeRiseSeconds = 10.58f;

	// Charge 동안 누적 회전할 yaw 각도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation")
	float BossChargeYawDegrees = 360.0f;

	// Drop 시작 후 Faerin 본체가 바닥 위치로 하강하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.01"))
	float BossDropSeconds = 1.3f;

	// 대기 후 Faerin이 실제로 지면까지 떨어지는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.0"))
	float BossFastDropSeconds = 0.12f;

	// Faerin 하강과 함께 모든 StaticSword가 아래로 한 번 돌진하는 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.0"))
	float EntrySwordDiveDistance = 700.0f;

	// Entry dive 후 StaticSword가 자기 위치로 복귀하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Body|Presentation", meta = (ClampMin = "0.0"))
	float EntrySwordDiveReturnSeconds = 0.35f;

	// StaticSword 전환 후 첫 하강 전에 페어린 주변 원형 대형 회전을 사용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit", meta = (InlineEditConditionToggle))
	bool bUseEntryOrbitBeforeImpact = true;

	// StaticSword 전환 직후 Entry Orbit 타임라인을 시작하기 전 대기 시간이다. 검이 전환된 순간을 짧게 보여주고 싶을 때 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Timeline", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitStartDelayAfterStaticSwitch = 0.15f;

	// 현재 위치에서 페어린 주변 원형 대형 슬롯까지 부드럽게 이동하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Timeline", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitGatherDuration = 0.45f;

	// 원형 대형 슬롯으로 모일 때 사용하는 ease 곡선이다. 1보다 크면 초반은 느리고 후반에 더 빠르게 모인다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Timeline", meta = (ClampMin = "0.1", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitGatherAccelerationExponent = 1.5f;

	// 원형 대형에 도착한 뒤 회전을 시작하기 전에 잠깐 대형을 보여주는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Timeline", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitPreSpinHoldSeconds = 0.15f;

	// 원형 대형 상태에서 회전/가속/바깥 기울기/반경 축소를 진행하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Timeline", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitSpinDuration = 2.2f;

	// 회전이 끝난 뒤 바닥 낙하 직전에 최종 대형을 잠깐 유지하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Timeline", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitPostSpinHoldSeconds = 0.05f;

	// 원형 대형 시작 반지름이다. 페어린 위치를 중심으로 각 검이 이 반경 위에서 회전한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Shape", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitRadius = 650.0f;

	// 충돌 직전까지 줄어드는 최소 반경이다. EntryOrbitRadius보다 클 수 없으며, 값이 작을수록 검들이 중심 쪽으로 더 모인다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Shape", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitImpactMinRadius = 220.0f;

	// 원형 대형 중심의 높이 보정이다. 0이면 현재 페어린 위치 높이를 중심으로 회전한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Shape", meta = (EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitHeightOffset = 0.0f;

	// 지면 trace 결과에 더할 Z 보정이다. 검 pivot 때문에 바닥 위에 뜨거나 너무 깊게 박히면 이 값으로 맞춘다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Shape", meta = (EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitImpactGroundOffsetZ = -20.0f;

	// 회전 제어 방식이다. SpeedRamp는 시작/최대 속도, TotalTurns는 총 회전 바퀴 수를 기준으로 한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Spin", meta = (EditCondition = "bUseEntryOrbitBeforeImpact"))
	EPRFaerinGodFallEntryOrbitSpinControlMode EntryOrbitSpinControlMode = EPRFaerinGodFallEntryOrbitSpinControlMode::SpeedRamp;

	// SpeedRamp 모드에서 회전 시작 속도(deg/sec)다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Spin", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitStartAngularSpeedDeg = 35.0f;

	// SpeedRamp 모드에서 회전이 끝날 때 도달할 최대 속도(deg/sec)다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Spin", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitMaxAngularSpeedDeg = 540.0f;

	// 회전 가속 곡선이다. SpeedRamp에서는 속도 보간 곡선, TotalTurns에서는 누적 회전량 곡선으로 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Spin", meta = (ClampMin = "0.1", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitAccelerationExponent = 2.0f;

	// TotalTurns 모드에서 회전 구간 동안 몇 바퀴 돌지 직접 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Spin", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitTotalTurns = 1.25f;

	// 회전 구간 중 반경 축소를 시작할 normalized time이다. 0이면 회전 시작부터 줄어든다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|RadiusShrink", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitRadiusShrinkStartAlpha = 0.25f;

	// 회전 구간 중 반경 축소를 완료할 normalized time이다. 1이면 회전 끝에서 최소 반경에 도달한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|RadiusShrink", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitRadiusShrinkEndAlpha = 1.0f;

	// 회전 중 반경이 줄어드는 곡선이다. 1보다 크면 후반에 더 급격히 좁아진다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|RadiusShrink", meta = (ClampMin = "0.1", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitRadiusShrinkExponent = 1.35f;

	// 원형 회전 중 검이 바깥 방향으로 벌어지는 최대 각도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Tilt", meta = (ClampMin = "0.0", ClampMax = "89.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitMaxOutwardTiltDegrees = 38.0f;

	// 회전 구간 중 바깥쪽 기울어짐을 시작할 normalized time이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Tilt", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitTiltStartAlpha = 0.35f;

	// 회전 구간 중 바깥쪽 기울어짐을 완료할 normalized time이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Tilt", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitTiltEndAlpha = 1.0f;

	// 바깥쪽 벌어짐 각도 증가 곡선이다. 1보다 크면 후반에 더 크게 벌어진다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Tilt", meta = (ClampMin = "0.1", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitTiltExponent = 1.5f;

	// Entry Orbit 중 지정한 검 면이 항상 Faerin 중심을 바라보도록 회전을 계산할지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|CenterFacingCone", meta = (InlineEditConditionToggle))
	bool bUseEntryOrbitCenterFacingConeRotation = true;

	// 검 메시 local 공간에서 검 길이 방향으로 사용할 축이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|CenterFacingCone", meta = (EditCondition = "bUseEntryOrbitCenterFacingConeRotation"))
	EPRFaerinGodFallLocalAxis EntryOrbitConeLocalBladeAxis = EPRFaerinGodFallLocalAxis::PlusZ;

	// Faerin 중심을 바라봐야 하는 검 면의 local normal 축이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|CenterFacingCone", meta = (EditCondition = "bUseEntryOrbitCenterFacingConeRotation"))
	EPRFaerinGodFallLocalAxis EntryOrbitCenterFacingLocalFaceAxis = EPRFaerinGodFallLocalAxis::MinusY;

	// 메시 축 보정이 더 필요할 때 마지막에 곱할 회전 보정값이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|CenterFacingCone", meta = (EditCondition = "bUseEntryOrbitCenterFacingConeRotation"))
	FRotator EntryOrbitCenterFacingRotationOffset = FRotator::ZeroRotator;

	// 선택한 blade axis 반대쪽이 실제 검 끝 방향일 때 중앙 교차를 피하도록 cone 기울기 방향을 뒤집는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|CenterFacingCone", meta = (EditCondition = "bUseEntryOrbitCenterFacingConeRotation"))
	bool bInvertEntryOrbitCenterFacingConeTiltDirection = false;

	// Impact Spread / EntryDiving 중에도 같은 면이 중심을 바라보는 회전을 유지할지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|CenterFacingCone", meta = (EditCondition = "bUseEntryOrbitCenterFacingConeRotation"))
	bool bKeepEntryOrbitCenterFacingDuringImpactDrop = true;

	// Entry Orbit 후 지면으로 꽂히는 시간이다. 0 이하이면 BossFastDropSeconds를 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Impact", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitImpactDropSeconds = 0.18f;

	// Entry Orbit 낙하 때 현재 회전 각도 방향으로 바깥쪽 landing 지점을 잡아 퍼지며 박히게 할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Impact Spread", meta = (EditCondition = "bUseEntryOrbitBeforeImpact"))
	bool bUseEntryOrbitImpactSpread = true;

	// Entry Orbit 낙하가 끝날 때 검이 꽂힐 최종 원형 반경이다. EntryOrbitImpactMinRadius보다 크면 회전 후 바깥으로 퍼지며 박힌다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Impact Spread", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact && bUseEntryOrbitImpactSpread"))
	float EntryOrbitImpactLandingRadius = 750.0f;

	// 최종 landing 각도를 회전 방향으로 얼마나 앞당길지 정한다. 0이면 순수 방사형, 양수면 회전 관성 방향으로 더 휘어져 꽂힌다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Impact Spread", meta = (EditCondition = "bUseEntryOrbitBeforeImpact && bUseEntryOrbitImpactSpread"))
	float EntryOrbitImpactLandingAngleLeadDegrees = 25.0f;

	// 낙하 구간 중 바깥 확산을 시작할 normalized time이다. 0이면 낙하 시작부터 퍼지고, 0.25면 먼저 내려간 뒤 퍼진다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Impact Spread", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bUseEntryOrbitBeforeImpact && bUseEntryOrbitImpactSpread"))
	float EntryOrbitImpactSpreadStartAlpha = 0.0f;

	// 낙하 중 XY 확산 보간 곡선이다. 1은 선형, 1보다 크면 초반은 덜 퍼지고 후반에 빠르게 벌어진다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Impact Spread", meta = (ClampMin = "0.1", EditCondition = "bUseEntryOrbitBeforeImpact && bUseEntryOrbitImpactSpread"))
	float EntryOrbitImpactSpreadExponent = 1.4f;

	// Entry Impact 후 검이 박힌 상태로 유지되는 시간이다. 일반 주기 낙하 검의 ImpactHoldSeconds와 분리한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Impact", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitImpactHoldSeconds = 0.25f;

	// Entry Impact 후 다시 상승하기 전에 검을 먼저 직선으로 세우는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Return", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitReturnStraightenSeconds = 0.25f;

	// Entry Impact 후 검을 직선으로 세울 때 사용하는 ease 곡선이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Return", meta = (ClampMin = "0.1", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitReturnStraightenExponent = 1.5f;

	// 검이 직선으로 선 뒤 HomeLocation까지 상승하는 시간이다. 기존 EntrySwordDiveReturnSeconds와 분리한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Return", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitRiseAfterStraightenSeconds = 0.45f;

	// Entry Impact 후 검별 복귀 연출 시간을 랜덤으로 부여할지 결정한다. 켜면 아래 Range 값에서 각 StaticSword Actor마다 별도 값을 뽑는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Return|Random", meta = (EditCondition = "bUseEntryOrbitBeforeImpact"))
	bool bRandomizeEntryOrbitReturnTiming = true;

	// 검별 직선 세우기 시간 범위다. bRandomizeEntryOrbitReturnTiming이 false면 EntryOrbitReturnStraightenSeconds를 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Return|Random", meta = (EditCondition = "bUseEntryOrbitBeforeImpact && bRandomizeEntryOrbitReturnTiming"))
	FPRFaerinGodFallFloatRange EntryOrbitReturnStraightenSecondsRange = FPRFaerinGodFallFloatRange(0.18f, 0.45f);

	// 검별 직선 세우기 ease exponent 범위다. 값이 클수록 후반에 빠르게 선다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Return|Random", meta = (EditCondition = "bUseEntryOrbitBeforeImpact && bRandomizeEntryOrbitReturnTiming"))
	FPRFaerinGodFallFloatRange EntryOrbitReturnStraightenExponentRange = FPRFaerinGodFallFloatRange(1.0f, 2.2f);

	// 검별 상승 시간 범위다. bRandomizeEntryOrbitReturnTiming이 false면 EntryOrbitRiseAfterStraightenSeconds를 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Return|Random", meta = (EditCondition = "bUseEntryOrbitBeforeImpact && bRandomizeEntryOrbitReturnTiming"))
	FPRFaerinGodFallFloatRange EntryOrbitRiseAfterStraightenSecondsRange = FPRFaerinGodFallFloatRange(0.35f, 0.9f);

	// 상승 완료 후 Charging 상태로 들어가기 전 추가 대기 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Return", meta = (ClampMin = "0.0", EditCondition = "bUseEntryOrbitBeforeImpact"))
	float EntryOrbitChargeStartDelayAfterRise = 0.0f;

	// Entry Orbit 후 모든 검이 바닥에 꽂히는 순간 전역 데미지를 1회 적용할지 결정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Damage", meta = (InlineEditConditionToggle))
	bool bApplyEntryOrbitImpactGlobalDamage = true;

	// Entry Impact 전역 데미지에 사용할 GameplayEffect다. 비워 두면 ImpactDamageEffectClass, Registry Enemy Damage GE 순으로 fallback한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Damage", meta = (EditCondition = "bApplyEntryOrbitImpactGlobalDamage"))
	TSubclassOf<UGameplayEffect> EntryOrbitImpactGlobalDamageEffectClass;

	// Entry Impact 전역 데미지의 Enemy AttackPower 배율이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Damage", meta = (ClampMin = "0.0", EditCondition = "bApplyEntryOrbitImpactGlobalDamage"))
	float EntryOrbitImpactGlobalDamageMultiplier = 1.0f;

	// Entry Impact 전역 데미지가 플레이어에게 적용할 강인도 피해량이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Damage", meta = (ClampMin = "0.0", EditCondition = "bApplyEntryOrbitImpactGlobalDamage"))
	float EntryOrbitImpactGlobalPoiseDamage = 0.0f;

	// 0 이하이면 현재 GodFall target 전체에게 적용한다. 0 초과이면 페어린 시전 위치 기준 반경 안의 대상만 맞춘다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|EntryOrbit|Damage", meta = (ClampMin = "0.0", EditCondition = "bApplyEntryOrbitImpactGlobalDamage"))
	float EntryOrbitImpactGlobalDamageRadius = 0.0f;

	// Phase2 기준 검 충전 최소 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.0"))
	float Phase2SwordChargeMinSeconds = 8.0f;

	// Phase2 기준 검 충전 최대 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.0"))
	float Phase2SwordChargeMaxSeconds = 10.0f;

	// 검이 돌진하기 전 원위치에서 경고를 보여주는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.0"))
	float WarningSeconds = 1.0f;

	// 배정된 검이 자기 위치에서 플레이어 머리 위까지 이동하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.0"))
	float TargetOverheadMoveSeconds = 8.0f;

	// 배정된 검이 플레이어 머리 위를 추적할 때 매초 증가하는 속도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.0"))
	float TargetOverheadMoveAcceleration = 900.0f;

	// 검이 원위치에서 목표 지점까지 돌진하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.01"))
	float DropSeconds = 0.28f;

	// impact 이후 원위치로 복귀하는 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.01"))
	float RiseSeconds = 0.45f;

	// impact 상태를 유지해 피격/시각 연출을 보여줄 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.0"))
	float ImpactHoldSeconds = 0.12f;

	// StaticSword가 바닥에 충돌할 때 충돌 위치에 재생할 Niagara다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|SwordImpact")
	TObjectPtr<UNiagaraSystem> SwordImpactNiagaraSystem;

	// StaticSword 충돌 Niagara 회전 보정값이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|SwordImpact")
	FRotator SwordImpactNiagaraRotationOffset = FRotator::ZeroRotator;

	// StaticSword 충돌 Niagara 스케일이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|SwordImpact")
	FVector SwordImpactNiagaraScale = FVector::OneVector;

	// 0보다 크면 충돌 Niagara를 해당 시간 뒤 강제로 정리한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|SwordImpact", meta = (ClampMin = "0.0"))
	float SwordImpactNiagaraLifeSeconds = 2.0f;

	// God Fall 검이 플레이어를 내려쳐 충돌할 때 충돌 위치에 재생할 사운드 큐다. 비어 있으면 재생하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|SFX|SwordStrike")
	TObjectPtr<USoundBase> SwordStrikeSoundCue;

	// 강타 사운드를 충돌 몇 초 전에 재생할지 정한다. 0이면 충돌 순간에 재생한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|SFX|SwordStrike", meta = (ClampMin = "0.0"))
	float SwordStrikeSoundLeadSeconds = 0.2f;

	// God Fall 시전 시작(보스가 검을 들고 위로 상승)할 때 보스 몸에 attach해 재생할 사운드 큐다. 비어 있으면 재생하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|SFX|Rise")
	TObjectPtr<USoundBase> GodFallRiseSoundCue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|SwordWarning")
	TObjectPtr<UNiagaraSystem> ImpactWarningNiagaraSystem;

	// warning VFX가 바닥에 묻히지 않도록 impact 위치에 더할 월드 오프셋이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|SwordWarning")
	FVector ImpactWarningLocationOffset = FVector(0.0f, 0.0f, 8.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|SwordWarning", meta = (ClampMin = "0.0"))
	float ImpactWarningLeadSeconds = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|SwordWarning")
	FRotator ImpactWarningNiagaraRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|SwordWarning")
	FVector ImpactWarningNiagaraScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|SwordWarning", meta = (ClampMin = "0.0"))
	float ImpactWarningNiagaraLifeSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|Hover")
	bool bEnableStaticSwordHoverVisual = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|Hover", meta = (ClampMin = "0.0"))
	float HoverAmplitudeZ = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|Hover", meta = (ClampMin = "0.0"))
	float HoverLateralAmplitude = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|Hover", meta = (ClampMin = "0.0"))
	float HoverFrequencyHz = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|Hover")
	float HoverPhaseOffsetPerSword = 0.73f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|ChargeShake")
	bool bEnableChargeShakeVisual = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|ChargeShake")
	FVector ChargeShakeAmplitudeLocation = FVector(4.0f, 4.0f, 8.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|ChargeShake")
	FRotator ChargeShakeAmplitudeRotation = FRotator(1.8f, 1.2f, 1.8f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|ChargeShake", meta = (ClampMin = "0.0"))
	float ChargeShakeFrequencyHz = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|ChargeShake")
	float ChargeShakePhaseOffsetPerSword = 1.37f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|ChargeShake", meta = (ClampMin = "0.0"))
	float ChargeShakeRampInSeconds = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|ImpactSlant")
	bool bEnableImpactVisualSlant = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|ImpactSlant")
	float ImpactSlantPitchDegrees = -18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|ImpactSlant")
	float ImpactSlantRollDegrees = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|ImpactSlant")
	float ImpactSlantRandomYawMinDegrees = -12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|ImpactSlant")
	float ImpactSlantRandomYawMaxDegrees = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Visual|ImpactSlant", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ImpactSlantBlendInStartAlpha = 0.0f;

	// God Fall 검 뽑힘 시점에 플레이어에게 강인도/그로기 피해를 적용할지 여부다.
	// 기존 검 뽑힘 화면 흔들림 연출을 대체하는 서버 권한 gameplay 효과다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Poise", meta = (InlineEditConditionToggle))
	bool bApplySwordRisePoiseDamage = true;

	// God Fall 검 뽑힘 시 플레이어에게 적용할 강인도/그로기 피해량이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Poise", meta = (ClampMin = "0.0", EditCondition = "bApplySwordRisePoiseDamage"))
	float SwordRisePoiseDamage = 101.0f;

	// God Fall 시전 시작 후 SwordRisePoiseDamage를 적용할 지연 시간이다. 기존 화면 흔들림 지연값을 대체한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Poise", meta = (ClampMin = "0.0", EditCondition = "bApplySwordRisePoiseDamage"))
	float SwordRisePoiseDamageDelaySeconds = 0.0f;

	// 검 뽑힘 Poise 피해에 사용할 GameplayEffect다. 비워 두면 ImpactDamageEffectClass, Registry Enemy Damage GE 순으로 fallback한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Poise", meta = (EditCondition = "bApplySwordRisePoiseDamage"))
	TSubclassOf<UGameplayEffect> SwordRisePoiseDamageEffectClass;

	// God Fall 본체 시전 중 보스 몸에 붙여 재생할 Niagara cue 목록이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|VFX|Body")
	TArray<FPRFaerinGodFallBodyNiagaraCue> BodyNiagaraCues;

	// Phase3에서 검 충전 시간과 경고 시간을 곱할 배율이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.01"))
	float Phase3GodFallTimingScale = 0.65f;

	// Phase4에서 검 충전 시간과 경고 시간을 곱할 배율이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Timing", meta = (ClampMin = "0.01"))
	float Phase4GodFallTimingScale = 0.45f;

	// 검 높이에서 아래로 내리는 지면 trace 길이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Target", meta = (ClampMin = "0.0"))
	float GroundTraceDownDistance = 3000.0f;

	// 지면 투영에 사용할 trace channel이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Target")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	// impact overlap 판정에 사용할 channel이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Damage")
	TEnumAsByte<ECollisionChannel> ImpactOverlapChannel = ECC_Pawn;

	// impact 피해에 사용할 GameplayEffect다. 비워 두면 Registry의 Enemy damage GE를 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Damage")
	TSubclassOf<UGameplayEffect> ImpactDamageEffectClass;

	// Enemy AttackPower에 곱할 impact 피해 배율이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Damage", meta = (ClampMin = "0.0"))
	float ImpactDamageMultiplier = 1.15f;

	// impact가 플레이어에게 적용하는 강인도 피해량이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Damage", meta = (ClampMin = "0.0"))
	float ImpactPoiseDamage = 18.0f;

	// impact 피해 반경이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Damage", meta = (ClampMin = "0.0"))
	float ImpactRadius = 180.0f;

	// PIE 검증용 debug draw 표시 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|GodFall|Debug")
	bool bDrawDebug = false;
};
