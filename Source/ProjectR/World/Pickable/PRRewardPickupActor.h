// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (보상 획득 시 골드/아이템 드롭 및 HUD 팝업 연동 구현)
// Author: 배유찬 (필드 보상 스포너 및 리스폰 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRPickableActor.h"
#include "ProjectR/ItemSystem/Types/PRDropTypes.h"
#include "ProjectR/ItemSystem/Types/PRItemTypes.h"
#include "PRRewardPickupActor.generated.h"

class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UPointLightComponent;
class UStaticMesh;
class UProjectileMovementComponent;
class USphereComponent;
class APRPlayerState;

USTRUCT(BlueprintType)
struct FPRRewardPickupEffectVisualInfo
{
	GENERATED_BODY()

	// 픽업 액터에서 재생할 나이아가라 이펙트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> NiagaraEffect = nullptr;

	// 나이아가라 색상 파라미터 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName NiagaraColorParameterName = TEXT("EffectColor");

	// 나이아가라에 전달할 색상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FLinearColor EffectColor = FLinearColor::White;

	// 픽업 주변 빛 색상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FLinearColor LightColor = FLinearColor::White;

	// 픽업 주변 빛 세기. 0 이하면 비활성화한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float LightIntensity = 0.0f;
};

// 월드에 놓인 드롭 보상의 표현과 상호작용을 담당한다
UCLASS()
class PROJECTR_API APRRewardPickupActor : public APRPickableActor
{
	GENERATED_BODY()

public:
	APRRewardPickupActor();

	/*~ UObject Interface ~*/
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 서버에서 확정 보상 정보를 초기화한다
	UFUNCTION(BlueprintAuthorityOnly)
	void InitializeReward(const FPRResolvedDropReward& InReward);

	// 픽업이 보유한 확정 보상 정보를 반환한다
	const FPRResolvedDropReward& GetReward() const { return Reward; }

	// 서버에서 픽업 보상 수량을 갱신한다
	UFUNCTION(BlueprintAuthorityOnly)
	void SetRewardQuantity(int32 NewQuantity);

	// 이미 지급 처리된 픽업인지 반환한다
	bool IsClaimed() const { return bClaimed; }

	// 해당 상호작용자가 이 보상 픽업을 획득할 수 있는지 확인한다
	bool CanBeClaimedBy(AActor* Interactor) const;

	// 서버에서 지급 처리 완료 상태로 표시한다
	void MarkClaimed();

protected:
	UFUNCTION()
	void OnRep_Reward();

	UFUNCTION()
	void OnRep_DropSettled();

	UFUNCTION()
	void HandleDropMovementStopped(const FHitResult& ImpactResult);

	// 현재 보상 타입에 맞춰 메시와 나이아가라 표현을 갱신한다
	void RefreshVisual();

	// 메시, 나이아가라, 라이트를 지정된 표현 정보로 갱신한다
	void ApplyEffectVisualInfo(const FPRRewardPickupEffectVisualInfo& VisualInfo);

	// 나이아가라와 라이트 표현을 비활성화한다
	void DeactivateEffectVisual();

	// 상호작용자에서 PRPlayerState를 찾는다
	APRPlayerState* ResolveInteractorPlayerState(AActor* Interactor) const;

	// 스폰 직후 낙하 연출을 시작한다
	void StartDropMotion();

	// 낙하 연출의 초기 속도를 만든다
	FVector BuildDropInitialVelocity() const;

	// 낙하 완료 여부에 맞춰 충돌 상태를 갱신한다
	void ApplyDropSettledState();

protected:
	// 바닥 충돌과 낙하 이동에 사용하는 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Drop")
	TObjectPtr<USphereComponent> DropCollision;

	// 스폰 직후 바닥으로 떨어지는 이동을 처리한다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward|Drop")
	TObjectPtr<UProjectileMovementComponent> DropMovement;

	// 보상 픽업의 기본 표시 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward")
	TObjectPtr<UStaticMeshComponent> RewardMesh;

	// 보상 픽업의 나이아가라 표현 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward")
	TObjectPtr<UNiagaraComponent> RewardNiagara;

	// 보상 픽업의 빛 표현 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reward")
	TObjectPtr<UPointLightComponent> RewardLight;

	// 고철 보상 픽업 메시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward|Visual")
	TObjectPtr<UStaticMesh> CurrencyMesh = nullptr;

	// 고철 보상 픽업 메시 스케일
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward|Visual", meta = (ClampMin = "0.0"))
	FVector CurrencyMeshScale = FVector::OneVector;

	// 고철 보상 픽업 표현
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward|Visual")
	FPRRewardPickupEffectVisualInfo CurrencyVisual;

	// 아이템 희귀도별 나이아가라와 라이트 표현
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward|Visual")
	TMap<EPRItemRarity, FPRRewardPickupEffectVisualInfo> ItemRarityVisuals;

	// 낙하 시작 시 수평으로 흩어지는 최소 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward|Drop", meta = (ClampMin = "0.0"))
	float DropHorizontalSpeedMin = 120.0f;

	// 낙하 시작 시 수평으로 흩어지는 최대 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward|Drop", meta = (ClampMin = "0.0"))
	float DropHorizontalSpeedMax = 260.0f;

	// 낙하 시작 시 위로 튀는 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward|Drop", meta = (ClampMin = "0.0"))
	float DropVerticalSpeed = 80.0f;

	// Manager가 확정한 보상 페이로드
	UPROPERTY(ReplicatedUsing = OnRep_Reward, EditAnywhere, BlueprintReadOnly, Category = "Reward")
	FPRResolvedDropReward Reward;

	// 중복 Claim 방어 상태
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Reward")
	bool bClaimed = false;

	// 낙하 연출이 끝나 실제 획득 가능한 상태인지 여부
	UPROPERTY(ReplicatedUsing = OnRep_DropSettled, VisibleInstanceOnly, BlueprintReadOnly, Category = "Reward|Drop")
	bool bDropSettled = false;
};
