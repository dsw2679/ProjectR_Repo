// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (Barrier Anchor Actor 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectR/AbilitySystem/Data/PRBarrierAbilityDataAsset.h"
#include "PRBarrierAnchorActor.generated.h"

class USpringArmComponent;
class USceneComponent;

// 배리어가 활성 중일 때만 존재하는 스프링 암 앵커 액터
UCLASS()
class PROJECTR_API APRBarrierAnchorActor : public AActor
{
	GENERATED_BODY()

public:
	// 스프링 암 루트 초기화
	APRBarrierAnchorActor();

public:
	/*~ AActor Interface ~*/
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 소스 액터에 앵커를 부착하고 설정을 적용한다
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ProjectR|Barrier")
	void InitializeAnchor(AActor* InSourceActor, const UPRBarrierAbilityDataAsset* BarrierData);

	// 배리어 부착 컴포넌트를 반환한다
	USceneComponent* GetBarrierAttachComponent() const;

	// 배리어 부착 소켓 이름을 반환한다
	FName GetBarrierAttachSocketName() const;

protected:
	// 소스 액터 복제 수신 처리
	UFUNCTION()
	void OnRep_SourceActor();

	// 앵커 설정 복제 수신 처리
	UFUNCTION()
	void OnRep_AnchorConfig();

	// 앵커 설정 적용
	void ApplyAnchorConfig();

	// 소스 조준 회전 적용
	void ApplySourceControlRotation();

protected:
	// 배리어 부착용 스프링 암
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Barrier")
	TObjectPtr<USpringArmComponent> BarrierArm;

private:
	// 부착 기준 소스 액터
	UPROPERTY(ReplicatedUsing = OnRep_SourceActor)
	TObjectPtr<AActor> SourceActor;

	// 복제된 앵커 설정
	UPROPERTY(ReplicatedUsing = OnRep_AnchorConfig)
	FPRBarrierAnchorConfig AnchorConfig;
};
