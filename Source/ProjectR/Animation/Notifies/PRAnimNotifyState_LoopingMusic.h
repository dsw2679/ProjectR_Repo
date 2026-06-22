// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "PRAnimNotifyState_LoopingMusic.generated.h"

class UAudioComponent;
class USoundAttenuation;
class USoundBase;
class USoundConcurrency;

UCLASS(meta = (DisplayName = "PR Looping Music"))
class PROJECTR_API UPRAnimNotifyState_LoopingMusic : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/*~ UAnimNotifyState Interface ~*/
	// 에디터 타임라인 표시용 Notify State 이름
	virtual FString GetNotifyName_Implementation() const override;

	// 루프 음악 재생 시작 요청
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	// 루프 음악 반복 재생 확인
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	// 루프 음악 재생 종료 요청
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	// 현재 메시에서 재생 가능한지 판정
	bool CanPlayForMesh(const USkeletalMeshComponent* MeshComp) const;

	// 현재 메시의 재생 중인 오디오 컴포넌트 조회
	UAudioComponent* FindActiveAudioComponent(USkeletalMeshComponent* MeshComp) const;

	// 현재 메시의 재생 중인 오디오 컴포넌트 정리
	void StopActiveAudioComponent(USkeletalMeshComponent* MeshComp);

	// 노티파이 구간 동안 반복 재생할 음악
	UPROPERTY(EditAnywhere, Category = "ProjectR|Audio")
	TObjectPtr<USoundBase> Music = nullptr;

	// 로컬 소유자에게만 들리는 재생 여부
	UPROPERTY(EditAnywhere, Category = "ProjectR|Audio")
	bool bOwnerOnly = true;

	// 사운드 에셋이 루프가 아닐 때 노티파이 구간 동안 재시작할지 여부
	UPROPERTY(EditAnywhere, Category = "ProjectR|Audio")
	bool bRestartWhenFinished = true;

	// 주변 재생 시 부착할 소켓 이름
	UPROPERTY(EditAnywhere, Category = "ProjectR|Audio")
	FName AttachSocketName = NAME_None;

	// 주변 재생 시 소켓 기준 위치 오프셋
	UPROPERTY(EditAnywhere, Category = "ProjectR|Audio")
	FVector LocationOffset = FVector::ZeroVector;

	// 주변 재생 시 소켓 기준 회전 오프셋
	UPROPERTY(EditAnywhere, Category = "ProjectR|Audio")
	FRotator RotationOffset = FRotator::ZeroRotator;

	// 재생 볼륨 배율
	UPROPERTY(EditAnywhere, Category = "ProjectR|Audio", meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	// 재생 피치 배율
	UPROPERTY(EditAnywhere, Category = "ProjectR|Audio", meta = (ClampMin = "0.1"))
	float PitchMultiplier = 1.0f;

	// 재생 시작 시간
	UPROPERTY(EditAnywhere, Category = "ProjectR|Audio", meta = (ClampMin = "0.0"))
	float StartTime = 0.0f;

	// 주변 재생용 감쇠 설정
	UPROPERTY(EditAnywhere, Category = "ProjectR|Audio", meta = (EditCondition = "!bOwnerOnly"))
	TObjectPtr<USoundAttenuation> AttenuationSettings = nullptr;

	// 동시 재생 규칙
	UPROPERTY(EditAnywhere, Category = "ProjectR|Audio")
	TObjectPtr<USoundConcurrency> ConcurrencySettings = nullptr;

private:
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TWeakObjectPtr<UAudioComponent>> ActiveAudioComponents;
};
