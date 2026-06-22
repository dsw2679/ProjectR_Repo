// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (애니메이션 Weapon Niagara 이펙트 윈도우/트리거 노티파이 구현)
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PRAnimNotify_WeaponNiagaraEffect.generated.h"

class UNiagaraSystem;

UENUM(BlueprintType)
enum class EPRWeaponNiagaraSpawnMode : uint8
{
	AttachedToSocket,
	SpawnAtSocket,
};

// 무기 몽타주에서 무기 스켈레탈 메시 소켓 기준 Niagara 이펙트를 재생하는 Notify다.
UCLASS(meta = (DisplayName = "PR Weapon Niagara Effect"))
class PROJECTR_API UPRAnimNotify_WeaponNiagaraEffect : public UAnimNotify
{
	GENERATED_BODY()

public:
	// 에디터 타임라인에 표시할 Notify 이름을 반환한다
	virtual FString GetNotifyName_Implementation() const override;

	// Notify 시점에 무기 메시 소켓 기준으로 Niagara 이펙트를 재생한다
	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	// 타임라인 표시용 이펙트 이름
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Effect")
	FName EffectName = FName(TEXT("MuzzleFlash"));

	// 재생할 Niagara 시스템
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Effect")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	// 기준으로 사용할 무기 스켈레탈 메시 소켓 이름
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Effect")
	FName SocketName = FName(TEXT("Socket_Muzzle"));

	// 이펙트 재생 방식
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Effect")
	EPRWeaponNiagaraSpawnMode SpawnMode = EPRWeaponNiagaraSpawnMode::AttachedToSocket;

	// 소켓 기준 위치 오프셋
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Effect")
	FVector LocationOffset = FVector::ZeroVector;

	// 소켓 기준 회전 오프셋
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Effect")
	FRotator RotationOffset = FRotator::ZeroRotator;

	// 이펙트 스케일
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Effect", meta = (ClampMin = "0.0"))
	FVector Scale = FVector::OneVector;
};
