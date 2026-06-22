// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 Pattern VFX 컴포넌트 구현)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRFaerinPatternVFXComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;

// Faerin 패턴별 VFX를 보스 본체 생명주기에 묶어 관리하는 컴포넌트다.
// 바닥 원형 VFX는 기본적으로 Faerin의 상시 표식이므로 BeginPlay 때 자동 표시한다.
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRFaerinPatternVFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRFaerinPatternVFXComponent();

	/*~ UActorComponent Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// GroundCircleVFXSystem을 생성/활성화한다. 이미 생성되어 있으면 다시 활성화만 한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|PatternVFX")
	void ShowGroundCircleVFX();

	// GroundCircleVFX를 비활성화한다. 컴포넌트는 재사용을 위해 유지한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|PatternVFX")
	void HideGroundCircleVFX();

	// GroundCircleVFX 표시 상태를 설정한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|PatternVFX")
	void SetGroundCircleVFXVisible(bool bVisible);

	// 이 컴포넌트가 관리하는 패턴 VFX를 모두 초기 상태로 되돌린다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Faerin|PatternVFX")
	void ResetPatternVFX();

	// 현재 생성된 GroundCircle NiagaraComponent를 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Faerin|PatternVFX")
	UNiagaraComponent* GetGroundCircleVFXComponent() const { return GroundCircleVFXComponent; }

protected:
	// 호출 시 생성/표시할 바닥 원형 Niagara 시스템이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PatternVFX|GroundCircle")
	TObjectPtr<UNiagaraSystem> GroundCircleVFXSystem;

	// VFX를 붙일 컴포넌트를 직접 지정한다. 비워 두면 Owner RootComponent에 붙인다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PatternVFX|GroundCircle")
	TObjectPtr<USceneComponent> GroundCircleAttachComponent;

	// AttachComponent에 붙일 소켓 이름이다. None이면 컴포넌트 기준으로 붙인다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PatternVFX|GroundCircle")
	FName GroundCircleAttachSocketName = NAME_None;

	// 부착 기준 상대 위치다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PatternVFX|GroundCircle")
	FVector GroundCircleRelativeLocation = FVector::ZeroVector;

	// 부착 기준 상대 회전이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PatternVFX|GroundCircle")
	FRotator GroundCircleRelativeRotation = FRotator::ZeroRotator;

	// 부착 기준 상대 스케일이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PatternVFX|GroundCircle")
	FVector GroundCircleRelativeScale = FVector::OneVector;

	// BeginPlay 때 GroundCircleVFXSystem을 자동 생성/표시할지 여부다.
	// Faerin 바닥 원형진은 기본 상시 표식이므로 기본값은 true다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PatternVFX|GroundCircle")
	bool bShowGroundCircleOnBeginPlay = true;

	// ResetPatternVFX 호출 시 바닥 원형진을 다시 보이는 상태로 되돌릴지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PatternVFX|GroundCircle")
	bool bResetGroundCircleToVisible = true;

	// 이전 패치 호환용 값이다. bShowGroundCircleOnBeginPlay가 true면 이 값은 무시된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Faerin|PatternVFX|GroundCircle|Legacy")
	bool bHideGroundCircleOnBeginPlay = false;

private:
	USceneComponent* ResolveGroundCircleAttachComponent() const;
	UNiagaraComponent* EnsureGroundCircleVFXComponent();
	void DestroyGroundCircleVFXComponent();

private:
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> GroundCircleVFXComponent;
};
