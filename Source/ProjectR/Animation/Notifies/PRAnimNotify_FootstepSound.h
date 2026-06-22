// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (애니메이션 Footstep 사운드 윈도우/트리거 노티파이 구현)
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ProjectR/Animation/PRFootstepSoundDataAsset.h"
#include "PRAnimNotify_FootstepSound.generated.h"

class UPRFootstepSoundDataAsset;

// 발 접지 시점에 표면 재질별 발소리와 AI 청각 자극을 처리하는 Notify
UCLASS(meta = (DisplayName = "PR Footstep Sound"))
class PROJECTR_API UPRAnimNotify_FootstepSound : public UAnimNotify
{
	GENERATED_BODY()

public:
	/*~ UAnimNotify Interface ~*/
	// 에디터 타임라인 표시 이름 반환
	virtual FString GetNotifyName_Implementation() const override;

	// 발소리 표면 판정과 재생 요청
	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	// 트레이스 기준 위치 계산
	bool ResolveTraceOrigin(const USkeletalMeshComponent* MeshComp, FVector& OutTraceOrigin) const;

	// 표면 판정용 라인트레이스 수행
	bool TraceFootstepSurface(const USkeletalMeshComponent* MeshComp, const FVector& TraceOrigin, FHitResult& OutHit) const;

	// 발소리 재생
	void PlayFootstepSound(
		USkeletalMeshComponent* MeshComp,
		const FPRFootstepSoundEntry& SoundEntry,
		const FPRFootstepMoveModifier& MoveModifier,
		const FVector& SoundLocation) const;

	// AI 청각 자극 보고
	void ReportFootstepNoise(
		const USkeletalMeshComponent* MeshComp,
		const FPRFootstepSoundEntry& SoundEntry,
		const FPRFootstepMoveModifier& MoveModifier,
		const FVector& NoiseLocation) const;

protected:
	// 표면별 발소리 데이터
	UPROPERTY(EditAnywhere, Category = "ProjectR|Footstep")
	TObjectPtr<UPRFootstepSoundDataAsset> FootstepSoundData = nullptr;

	// 트레이스에 사용할 발 구분
	UPROPERTY(EditAnywhere, Category = "ProjectR|Footstep")
	EPRFootstepFoot Foot = EPRFootstepFoot::Left;

	// 발소리 크기를 보정할 이동 방식
	UPROPERTY(EditAnywhere, Category = "ProjectR|Footstep")
	EPRFootstepMoveType MoveType = EPRFootstepMoveType::Jog;

	// 발 소켓 위쪽 시작 보정
	UPROPERTY(EditAnywhere, Category = "ProjectR|Footstep|Trace", meta = (ClampMin = "0.0"))
	float TraceStartUpOffset = 5.0f;

	// 발 소켓 아래 검사 거리
	UPROPERTY(EditAnywhere, Category = "ProjectR|Footstep|Trace", meta = (ClampMin = "0.0"))
	float TraceDistance = 80.0f;

	// 바닥 판정 라인트레이스 채널
	UPROPERTY(EditAnywhere, Category = "ProjectR|Footstep|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// 복잡 콜리전 사용 여부
	UPROPERTY(EditAnywhere, Category = "ProjectR|Footstep|Trace")
	bool bTraceComplex = false;

	// 소켓이 없을 때 캡슐 하단으로 대체할지 여부
	UPROPERTY(EditAnywhere, Category = "ProjectR|Footstep|Trace")
	bool bFallbackToCapsuleBottom = true;

	// 서버 권위에서 AI 청각 자극 보고 여부
	UPROPERTY(EditAnywhere, Category = "ProjectR|Footstep|AI")
	bool bReportNoiseToAI = true;

	// 기본 청각 자극 세기
	UPROPERTY(EditAnywhere, Category = "ProjectR|Footstep|AI", meta = (ClampMin = "0.0"))
	float NoiseLoudness = 1.0f;

	// 청각 자극 최대 거리
	UPROPERTY(EditAnywhere, Category = "ProjectR|Footstep|AI", meta = (ClampMin = "0.0"))
	float NoiseMaxRange = 500.0f;

	// AI Perception에 전달할 소음 태그
	UPROPERTY(EditAnywhere, Category = "ProjectR|Footstep|AI")
	FName NoiseTag = FName(TEXT("Footstep"));

	// 트레이스 디버그 표시 여부
	UPROPERTY(EditAnywhere, Category = "ProjectR|Footstep|Debug")
	bool bDebugDraw = false;
};
