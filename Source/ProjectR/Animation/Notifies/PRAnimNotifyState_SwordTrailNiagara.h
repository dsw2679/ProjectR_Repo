// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (애니메이션 State 검격 Trail Niagara 윈도우/트리거 노티파이 구현)
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "PRAnimNotifyState_SwordTrailNiagara.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USkeletalMeshComponent;
class UAnimSequenceBase;

USTRUCT(BlueprintType)
struct PROJECTR_API FPRSwordTrailSocketPair
{
	GENERATED_BODY()

public:
	// false면 이 슬롯은 무시한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon|Trail|Socket")
	bool bEnabled = true;

	// 검날 하단 / 손잡이 위쪽 소켓명
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon|Trail|Socket")
	FName BladeBottomSocketName = FName(TEXT("trail-bottom"));

	// 검끝 소켓명
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon|Trail|Socket")
	FName BladeTopSocketName = FName(TEXT("trail-top"));
};

/**
 * 검이 포함된 SkeletalMeshComponent의 소켓 쌍을 기준으로 Niagara 검 궤적을 재생하는 Notify State.
 *
 * 사용 전제:
 * - 검 메쉬가 캐릭터/몬스터 Skeletal Mesh에 포함되어 있어야 한다.
 * - 같은 Skeletal Mesh 또는 Skeleton에 BladeBottomSocketName / BladeTopSocketName 소켓이 있어야 한다.
 * - AdditionalSocketPairs에 trail-top2 / trail-bottom2 같은 추가 소켓 쌍을 넣으면 같은 Notify 구간에서 Trail이 추가 생성된다.
 * - SwordEffects 계열 Trail Niagara는 User.TrailSize를 사용하므로 bSetUserTrailSize를 기본 true로 둔다.
 */
UCLASS(meta = (DisplayName = "PR Sword Trail Niagara"))
class PROJECTR_API UPRAnimNotifyState_SwordTrailNiagara : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPRAnimNotifyState_SwordTrailNiagara();

	/*~ UAnimNotifyState Interface ~*/
	virtual FString GetNotifyName_Implementation() const override;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	// 재생할 검 Trail Niagara 시스템. 예: NS_SwordEffect1_Trails ~ NS_SwordEffect7_Trails
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Trail")
	TObjectPtr<UNiagaraSystem> TrailSystem = nullptr;

	// 기존 Notify 호환용 기본 소켓 쌍 사용 여부.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Trail|Socket")
	bool bUsePrimarySocketPair = true;

	// 기본 검날 하단 / 손잡이 위쪽 소켓명.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Trail|Socket")
	FName BladeBottomSocketName = FName(TEXT("trail-bottom"));

	// 기본 검끝 소켓명.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Trail|Socket")
	FName BladeTopSocketName = FName(TEXT("trail-top"));

	// 추가 Trail 소켓 쌍. 예: trail-bottom2 / trail-top2, trail-bottom3 / trail-top3.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Trail|Socket")
	TArray<FPRSwordTrailSocketPair> AdditionalSocketPairs;

	// 기본 TrailSize Niagara 변수명
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Trail|Niagara")
	FName TrailSizeParameterName = FName(TEXT("TrailSize"));

	// SwordEffects Niagara가 사용하는 User.TrailSize도 같이 세팅할지 여부
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Trail|Niagara")
	bool bSetUserTrailSize = true;

	// 측정된 두 소켓 사이 거리 배율
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Trail|Niagara", meta = (ClampMin = "0.0"))
	float TrailSizeScale = 1.0f;

	// 측정된 두 소켓 사이 거리에 더할 보정값
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Trail|Niagara")
	float TrailSizeOffset = 0.0f;

	// Notify Tick마다 Trail 위치/회전/TrailSize를 갱신한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Trail")
	bool bUpdateEveryTick = true;

	// Dedicated Server에서는 VFX를 생성하지 않는다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Trail|Network")
	bool bSkipDedicatedServer = true;

	// 애니메이션 에디터/Persona Preview에서 Trail을 볼 수 있게 허용한다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Trail|Editor")
	bool bAllowEditorPreview = true;

	// Notify 종료 시 Niagara를 Deactivate한다. false면 Notify 종료 후에도 시스템이 계속 활성 상태로 남을 수 있다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Trail|End")
	bool bDeactivateOnEnd = true;

	// Notify 종료 시 즉시 Destroy한다. false면 Deactivate 후 Niagara auto-destroy/fade-out에 맡긴다.
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon|Trail|End")
	bool bDestroyImmediatelyOnEnd = false;

private:
	struct FActiveTrailInstance
	{
		TWeakObjectPtr<UNiagaraComponent> Component;
		FName BladeBottomSocketName = NAME_None;
		FName BladeTopSocketName = NAME_None;
	};

	struct FActiveTrailData
	{
		TArray<FActiveTrailInstance> Instances;
	};

	TMap<FObjectKey, FActiveTrailData> ActiveTrails;

	bool ShouldRunForMesh(const USkeletalMeshComponent* MeshComp) const;
	bool BuildTrailTransform(const USkeletalMeshComponent* MeshComp,
		FName InBladeBottomSocketName,
		FName InBladeTopSocketName,
		FVector& OutLocation,
		FRotator& OutRotation,
		float& OutTrailSize) const;
	void UpdateTrailComponent(const USkeletalMeshComponent* MeshComp, FActiveTrailInstance& TrailInstance) const;
	void EndTrailForMesh(USkeletalMeshComponent* MeshComp);
};
