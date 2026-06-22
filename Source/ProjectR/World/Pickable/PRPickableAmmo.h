// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 김동석 (드롭 시스템 연동 탄약 픽업 구현)
// Author: 배유찬 (탄약 획득 상호작용 및 탄종별 획득 판정 구현)
// Author: 이건주 (Mod 버프 연동 탄약 회복 보정 구현)
#pragma once

#include "CoreMinimal.h"
#include "PRPickableActor.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRPickableAmmo.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UNiagaraComponent;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FPRAmmoVisualInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMesh> AmmoMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UNiagaraSystem> AmmoNiagaraEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor EffectColor;
};

UCLASS()
class PROJECTR_API APRPickableAmmo : public APRPickableActor
{
	GENERATED_BODY()

public:
	APRPickableAmmo();

	// 픽업이 적립할 탄약 타입 반환
	UFUNCTION(BlueprintPure)
	EPRAmmoType GetAmmoType() const {return AmmoType;}

	// 픽업의 탄약량을 반환한다
	UFUNCTION(BlueprintPure)
	float GetAmmoAmount() const {return AmmoAmount;}

	// 서버에서 픽업 데이터를 갱신한다
	UFUNCTION(BlueprintAuthorityOnly)
	void SetAmmo(EPRAmmoType InAmmoType, float InAmmoAmount);

	// 대상 ASC의 슬롯 예비탄에 탄약을 적립하고, 한도 초과로 적립되지 못한 잔여량을 반환한다
	// 모두 적립되면 0 반환, 캡으로 일부만 적립되면 잔여량 반환, 무효 입력이면 AmmoAmount 그대로 반환
	UFUNCTION(BlueprintAuthorityOnly)
	float GrantAmmoAndGetRemaining(UAbilitySystemComponent* TargetASC);

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;

	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ APRPickableAmmo Interface ~*/
	// 현재 탄약 타입에 대응하는 메시·나이아가라 시스템·색상을 비주얼 컴포넌트에 적용
	void RefreshVisual();

	UFUNCTION()
	void OnRep_AmmoType();

	UFUNCTION()
	void OnRep_AmmoAmount();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Ammo")
	TObjectPtr<UNiagaraComponent> AmmoNiagara;

	UPROPERTY(VisibleAnywhere, Category = "Ammo")
	TObjectPtr<UStaticMeshComponent> AmmoMesh;

	// 탄약 타입별 비주얼 데이터
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	TMap<EPRAmmoType, FPRAmmoVisualInfo> AmmoTypeVisual;

	// 탄약 타입별 픽업 적용 GE. 각 GE는 SetByCaller.AmmoMagnitude 키로 해당 슬롯의 ReserveAmmo에 가산
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	TMap<EPRAmmoType, TSubclassOf<UGameplayEffect>> AmmoGEMap;

	// 픽업이 적립할 탄약 슬롯 종류
	UPROPERTY(EditAnywhere, ReplicatedUsing = "OnRep_AmmoType")
	EPRAmmoType AmmoType = EPRAmmoType::Primary;

	// 픽업의 탄약량
	UPROPERTY(EditAnywhere, ReplicatedUsing = "OnRep_AmmoAmount")
	float AmmoAmount = 10.f;
};
