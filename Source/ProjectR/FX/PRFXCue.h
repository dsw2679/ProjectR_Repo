// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (총기 사격 궤적(Tracer) 및 피격 폭발 비주얼 이펙트 연동 구현)
// Author: 손승우 (보스 순간이동 및 포털 특수 VFX 트리거 구현)
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "NiagaraComponentPoolMethodEnum.h"
#include "PRFXTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Object.h"
#include "PRFXCue.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPRFX, Log, All);

class UAudioComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class APawn;
class UInitialActiveSoundParams;
class USoundAttenuation;
class USoundBase;
class USoundConcurrency;

// 모든 FX Cue의 공통 타입 소거 진입점
UCLASS(Abstract, Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXCue : public UObject
{
	GENERATED_BODY()

public:
	// FX 시스템이 호출하는 공통 실행 함수
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload);

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const;

	// Cue UObject의 재사용 또는 생성 기준 반환
	EPRFXCueInstancingPolicy GetInstancingPolicy() const;

	// Cue 문맥 기준 World에 Actor 생성
	UFUNCTION(BlueprintCallable, Category = "ProjectR|FX|Cue", meta = (DeterminesOutputType = "ActorClass"))
	AActor* SpawnActor(
		const FPRFXCueContext& Context,
		TSubclassOf<AActor> ActorClass,
		const FTransform& SpawnTransform,
		ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined,
		AActor* Owner = nullptr,
		APawn* Instigator = nullptr);

	// Cue 문맥 기준 World 위치에 Niagara 생성
	UFUNCTION(BlueprintCallable, Category = "ProjectR|FX|Cue")
	UNiagaraComponent* SpawnNiagaraAtLocation(
		const FPRFXCueContext& Context,
		UNiagaraSystem* SystemTemplate,
		FVector Location,
		FRotator Rotation = FRotator::ZeroRotator,
		FVector Scale = FVector(1.0f),
		bool bAutoDestroy = true,
		bool bAutoActivate = true,
		ENCPoolMethod PoolingMethod = ENCPoolMethod::None,
		bool bPreCullCheck = true);

	// Cue 문맥 기준 World에서 SceneComponent에 Niagara 부착 생성
	UFUNCTION(BlueprintCallable, Category = "ProjectR|FX|Cue")
	UNiagaraComponent* SpawnNiagaraAttached(
		const FPRFXCueContext& Context,
		UNiagaraSystem* SystemTemplate,
		USceneComponent* AttachToComponent,
		FName AttachPointName = NAME_None,
		FVector Location = FVector::ZeroVector,
		FRotator Rotation = FRotator::ZeroRotator,
		EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset,
		bool bAutoDestroy = true,
		bool bAutoActivate = true,
		ENCPoolMethod PoolingMethod = ENCPoolMethod::None,
		bool bPreCullCheck = true);

	// Cue 문맥 기준 World 위치에 사운드 재생
	UFUNCTION(BlueprintCallable, Category = "ProjectR|FX|Cue")
	void PlaySoundAtLocation(
		const FPRFXCueContext& Context,
		USoundBase* Sound,
		FVector Location,
		FRotator Rotation = FRotator::ZeroRotator,
		float VolumeMultiplier = 1.0f,
		float PitchMultiplier = 1.0f,
		float StartTime = 0.0f,
		USoundAttenuation* AttenuationSettings = nullptr,
		USoundConcurrency* ConcurrencySettings = nullptr,
		const AActor* OwningActor = nullptr,
		const UInitialActiveSoundParams* InitialParams = nullptr);

	// Cue 문맥 기준 World 위치에 사운드 컴포넌트 생성
	UFUNCTION(BlueprintCallable, Category = "ProjectR|FX|Cue")
	UAudioComponent* SpawnSoundAtLocation(
		const FPRFXCueContext& Context,
		USoundBase* Sound,
		FVector Location,
		FRotator Rotation = FRotator::ZeroRotator,
		float VolumeMultiplier = 1.0f,
		float PitchMultiplier = 1.0f,
		float StartTime = 0.0f,
		USoundAttenuation* AttenuationSettings = nullptr,
		USoundConcurrency* ConcurrencySettings = nullptr,
		bool bAutoDestroy = true);

	// Cue 문맥 기준 SceneComponent에 사운드 컴포넌트 부착 생성
	UFUNCTION(BlueprintCallable, Category = "ProjectR|FX|Cue")
	UAudioComponent* SpawnSoundAttached(
		const FPRFXCueContext& Context,
		USoundBase* Sound,
		USceneComponent* AttachToComponent,
		FName AttachPointName = NAME_None,
		FVector Location = FVector(ForceInit),
		FRotator Rotation = FRotator::ZeroRotator,
		EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset,
		bool bStopWhenAttachedToDestroyed = false,
		float VolumeMultiplier = 1.0f,
		float PitchMultiplier = 1.0f,
		float StartTime = 0.0f,
		USoundAttenuation* AttenuationSettings = nullptr,
		USoundConcurrency* ConcurrencySettings = nullptr,
		bool bAutoDestroy = true);

	// Cue 문맥 기준 World에 2D 사운드 재생
	UFUNCTION(BlueprintCallable, Category = "ProjectR|FX|Cue")
	void PlaySound2D(
		const FPRFXCueContext& Context,
		USoundBase* Sound,
		float VolumeMultiplier = 1.0f,
		float PitchMultiplier = 1.0f,
		float StartTime = 0.0f,
		USoundConcurrency* ConcurrencySettings = nullptr,
		const AActor* OwningActor = nullptr,
		bool bIsUISound = true);

	// Cue 문맥 기준 World에 2D 사운드 컴포넌트 생성
	UFUNCTION(BlueprintCallable, Category = "ProjectR|FX|Cue")
	UAudioComponent* SpawnSound2D(
		const FPRFXCueContext& Context,
		USoundBase* Sound,
		float VolumeMultiplier = 1.0f,
		float PitchMultiplier = 1.0f,
		float StartTime = 0.0f,
		USoundConcurrency* ConcurrencySettings = nullptr,
		bool bPersistAcrossLevelTransition = false,
		bool bAutoDestroy = true);

public:
	// Cue UObject의 재사용 또는 생성 기준
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|FX")
	EPRFXCueInstancingPolicy InstancingPolicy = EPRFXCueInstancingPolicy::NonInstanced;
};

// 단순 신호 Cue
UCLASS(Abstract, Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXSimpleCue : public UPRFXCue
{
	GENERATED_BODY()

public:
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload) override;

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const override;

	// Blueprint와 C++ 파생 클래스가 구현하는 재생 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	void ExecuteFX(const FPRFXCueContext& Context, const FPRFXPayloadBase& Payload);
};


// 월드 위치 기반 Cue
UCLASS(Abstract, Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXLocationCue : public UPRFXCue
{
	GENERATED_BODY()

public:
	// 위치 Payload 검증 후 typed ExecuteFX로 전달
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload) override;

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const override;

	// Blueprint와 C++ 파생 클래스가 구현하는 위치 기반 재생 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	void ExecuteFX(const FPRFXCueContext& Context, const FPRFXLocationPayload& Payload);
};

// 액터 또는 컴포넌트 부착 기반 Cue
UCLASS(Abstract, Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXAttachCue : public UPRFXCue
{
	GENERATED_BODY()

public:
	// 부착 Payload 검증 후 typed ExecuteFX로 전달
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload) override;

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const override;

	// Blueprint와 C++ 파생 클래스가 구현하는 부착 기반 재생 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	void ExecuteFX(const FPRFXCueContext& Context, const FPRFXAttachPayload& Payload);
};



class APRWeaponActor;

// Trail 기반 Cue
UCLASS(Abstract, Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXTrailCue : public UPRFXCue
{
	GENERATED_BODY()

public:
	// Trail Payload 검증 후 typed ExecuteFX로 전달
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload) override;

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const override;

	// Blueprint와 C++ 파생 클래스가 구현하는 Trail 재생 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	void ExecuteFX(const FPRFXCueContext& Context, const FPRFXTrailPayload& Payload);
	// 현재 손에 든 무기 Actor와 Payload 무기 데이터 일치 확인
	UFUNCTION(BlueprintPure, Category = "ProjectR|FX")
	APRWeaponActor* ResolveMatchingWeaponActor(const FPRFXTrailPayload& Payload) const;
};

// 무기 발사 Trail Cue
UCLASS(Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXWeaponFireCue : public UPRFXCue
{
	GENERATED_BODY()
	
public:
	// Trail Payload 검증 후 typed ExecuteFX로 전달
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload) override;

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const override;

	// Blueprint와 C++ 파생 클래스가 구현하는 Trail 재생 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	void ExecuteFX(const FPRFXCueContext& Context, const FPRFXFirePayload& Payload);
	
	// 현재 손에 든 무기 Actor와 Payload 무기 데이터 일치 확인
	UFUNCTION(BlueprintPure, Category = "ProjectR|FX")
	APRWeaponActor* ResolveMatchingWeaponActor(const FPRFXFirePayload& Payload) const;
};

// 탄착과 피격 기반 Cue
UCLASS(Abstract, Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXHitCue : public UPRFXCue
{
	GENERATED_BODY()

public:
	// Hit Payload 검증 후 typed ExecuteFX로 전달
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload) override;

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const override;

	// Blueprint와 C++ 파생 클래스가 구현하는 탄착과 피격 재생 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	void ExecuteFX(const FPRFXCueContext& Context, const FPRFXHitPayload& Payload);
};

// Actor Spawn 기반 Cue
UCLASS(Abstract, Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXActorSpawnCue : public UPRFXCue
{
	GENERATED_BODY()

public:
	// Actor Spawn Payload 검증 후 typed ExecuteFX로 전달
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload) override;

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const override;

	// Blueprint와 C++ 파생 클래스가 구현하는 Actor Spawn 재생 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	void ExecuteFX(const FPRFXCueContext& Context, const FPRFXActorSpawnPayload& Payload);
};

// TODO: AActor Spawn 대신 Instance 자체 관리
// 상태 지속 기반 Cue, WIP
UCLASS(Abstract, Blueprintable, EditInlineNew)
class PROJECTR_API UPRFXPersistentCue : public UPRFXCue
{
	GENERATED_BODY()

public:
	// 지속 FX Payload의 Action을 읽고 시작 또는 종료 함수로 분기
	virtual void NativeExecuteFX(const FPRFXCueContext& Context, const FInstancedStruct& Payload) override;

	// Cue 클래스가 처리할 수 있는 Payload 구조체 타입 반환
	virtual const UScriptStruct* GetExpectedPayloadType() const override;

	// Blueprint와 C++ 파생 클래스가 구현하는 지속 FX Actor 생성 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	AActor* StartFX(const FPRFXCueContext& Context, const FPRFXPersistentPayload& Payload);

	// Blueprint와 C++ 파생 클래스가 구현하는 지속 FX Actor 종료 함수
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|FX")
	void StopFX(const FPRFXCueContext& Context, const FPRFXPersistentPayload& Payload, AActor* PersistentActor);
};
