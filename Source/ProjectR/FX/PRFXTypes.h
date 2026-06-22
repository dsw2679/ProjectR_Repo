// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (피격 데칼 및 탄착점 이펙트 관련 데이터 구조 정의)
// Author: 손승우 (보스 시퀀스 관련 이펙트 타입 정의)
#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "PRFXTypes.generated.h"

class UPRFXCue;
class UPRWeaponDataAsset;
class AActor;
class UPrimitiveComponent;
class USceneComponent;

// FX 호출 흐름 구분값
UENUM(BlueprintType)
enum class EPRFXPlaybackMode : uint8
{
	LocalOnly,
	PredictiveLocal,
	ServerConfirmed,
	Multicast,
	TargetedClient
};

// 예측 재생 후 서버 확정 재생이 도착했을 때 소유 클라이언트의 처리 방식
UENUM(BlueprintType)
enum class EPRFXOwnerReplayPolicy : uint8
{
	PlayAgain,
	SkipIfPredicted,
	CorrectIfPredicted
};

// Cue UObject 재사용 기준
UENUM(BlueprintType)
enum class EPRFXCueInstancingPolicy : uint8
{
	NonInstanced,
	InstancedPerExecution,
	InstancedPerPlayer
};

// 지속 FX 요청 의도
UENUM(BlueprintType)
enum class EPRFXPersistentAction : uint8
{
	Start,
	Stop
};

// 예측 재생과 서버 확정 재생의 중복 제거 기준
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFXPredictionKey
{
	GENERATED_BODY()

	// 예측 요청 주체를 구분하기 위한 로컬 식별자
	UPROPERTY(BlueprintReadWrite, Category = "ProjectR|FX")
	FGuid SourceId;

	// 같은 요청 주체 안에서 개별 FX를 구분하기 위한 증가값
	UPROPERTY(BlueprintReadWrite, Category = "ProjectR|FX")
	int32 Sequence = 0;

	// 예측 중복 제거 대상 여부
	UPROPERTY(BlueprintReadWrite, Category = "ProjectR|FX")
	bool bValid = false;

	// 예측 키 유효성 확인
	bool IsValid() const;

	// 예측 키 비교
	bool operator==(const FPRFXPredictionKey& Other) const;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPRFXPredictionKey> : public TStructOpsTypeTraitsBase2<FPRFXPredictionKey>
{
	enum { WithNetSerializer = true };
};

// 지속 FX 시작과 종료를 연결하는 식별자
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFXPersistentId
{
	GENERATED_BODY()

	// 같은 액터에서 여러 지속 FX 슬롯을 구분하기 위한 태그
	UPROPERTY(BlueprintReadWrite, Category = "ProjectR|FX")
	FGameplayTag SlotTag;

	// 같은 슬롯 안에서 재시작된 FX를 구분하기 위한 증가값
	UPROPERTY(BlueprintReadWrite, Category = "ProjectR|FX")
	int32 InstanceSequence = 0;

	// 지속 FX 식별자 유효성 확인
	bool IsValid() const;

	// 지속 FX 식별자 비교
	bool operator==(const FPRFXPersistentId& Other) const;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPRFXPersistentId> : public TStructOpsTypeTraitsBase2<FPRFXPersistentId>
{
	enum { WithNetSerializer = true };
};

// Registry가 태그별로 보관하는 Cue 클래스와 최소 재생 정책
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFXRegistryEntry
{
	GENERATED_BODY()

	// 태그가 실행할 Cue 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|FX")
	TArray<TSoftClassPtr<UPRFXCue>> CueClasses;

	// 로컬 예측 재생 허용 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|FX")
	bool bAllowPrediction = false;

	// 서버가 주변 클라이언트를 선별할 때 사용하는 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|FX", meta = (ClampMin = "0.0", Units = "cm"))
	float ReplicationRadius = 3000.0f;

	// 소유 클라이언트의 예측 FX와 확정 FX 중복 처리 규칙
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|FX")
	EPRFXOwnerReplayPolicy OwnerReplayPolicy = EPRFXOwnerReplayPolicy::SkipIfPredicted;

	// 서버 전파를 Reliable RPC로 처리해야 하는 중요 연출 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|FX")
	bool bReliableNetworkEvent = false;
};

// FX 호출과 네트워크 전송에 사용하는 요청 데이터
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFXRequest
{
	GENERATED_BODY()

	// 실행할 FX 태그
	UPROPERTY(BlueprintReadWrite, Category = "ProjectR|FX")
	FGameplayTag FXTag;

	// FX 유형별 데이터
	UPROPERTY(BlueprintReadWrite, Category = "ProjectR|FX")
	FInstancedStruct Payload;

	// 예측 재생과 서버 확정 재생을 연결하는 식별자
	UPROPERTY(BlueprintReadWrite, Category = "ProjectR|FX")
	FPRFXPredictionKey PredictionKey;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPRFXRequest> : public TStructOpsTypeTraitsBase2<FPRFXRequest>
{
	enum { WithNetSerializer = true };
};

// Cue가 실행 시점의 로컬 환경을 읽기 위한 문맥
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFXCueContext
{
	GENERATED_BODY()

	// 실행할 FX 태그
	UPROPERTY(BlueprintReadWrite, Category = "ProjectR|FX")
	FGameplayTag FXTag;

	// 현재 FX 호출의 재생 흐름
	UPROPERTY(BlueprintReadWrite, Category = "ProjectR|FX")
	EPRFXPlaybackMode PlaybackMode = EPRFXPlaybackMode::LocalOnly;

	// 예측 재생과 서버 확정 재생을 연결하는 식별자
	UPROPERTY(BlueprintReadWrite, Category = "ProjectR|FX")
	FPRFXPredictionKey PredictionKey;
	
	UPROPERTY(BlueprintReadWrite, Category = "ProjectR|FX")
	UObject* WorldContext;
};

// 모든 FX Payload가 공유하는 수명 입력값
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFXPayloadBase
{
	GENERATED_BODY()

	// Cue 또는 에셋 기본 수명 대신 Payload 수명을 사용할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	bool bHasLifeTime = false;

	// bHasLifeTime이 true일 때 Cue가 읽는 수명 파라미터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX", meta = (ClampMin = "0.0", Units = "s"))
	float LifeTime = 0.0f;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPRFXPayloadBase> : public TStructOpsTypeTraitsBase2<FPRFXPayloadBase>
{
	enum { WithNetSerializer = true };
};

// 월드 위치에서 단발로 재생되는 FX 정보
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFXLocationPayload : public FPRFXPayloadBase
{
	GENERATED_BODY()

	// FX가 생성될 월드 위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FVector Location = FVector::ZeroVector;

	// FX가 바라볼 월드 회전
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FRotator Rotation = FRotator::ZeroRotator;

	// 위치 기반 FX의 크기 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FVector Scale = FVector::OneVector;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPRFXLocationPayload> : public TStructOpsTypeTraitsBase2<FPRFXLocationPayload>
{
	enum { WithNetSerializer = true };
};

// 액터 또는 컴포넌트에 부착되는 FX 정보
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFXAttachPayload : public FPRFXPayloadBase
{
	GENERATED_BODY()

	// 부착 기준 액터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	TObjectPtr<AActor> AttachActor;

	// 액터보다 구체적인 부착 기준이 필요할 때 사용하는 컴포넌트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	TObjectPtr<USceneComponent> AttachComponent;

	// 부착할 소켓 또는 본 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FName SocketName = NAME_None;

	// 소켓 기준 상대 위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FVector RelativeLocation = FVector::ZeroVector;

	// 소켓 기준 상대 회전
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPRFXAttachPayload> : public TStructOpsTypeTraitsBase2<FPRFXAttachPayload>
{
	enum { WithNetSerializer = true };
};

// 총기 Trail, Beam, Tracer 재생 정보
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFXTrailPayload : public FPRFXPayloadBase
{
	GENERATED_BODY()

	// 발사를 발생시킨 Actor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	TObjectPtr<AActor> SourceActor;

	// 발사 시점의 무기 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	TObjectPtr<UPRWeaponDataAsset> WeaponData;

	// Trail 시작 위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FVector StartLocation = FVector::ZeroVector;

	// Trail 종료 위치 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	TArray<FVector> TrailEnds;

	// 충돌 지점에 도달한 Trail인지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	bool bBlockingHit = false;

	// bBlockingHit이 true일 때만 네트워크로 전송되는 실제 충돌 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FHitResult HitResult;

	// Trail 방향 계산과 Niagara 파라미터 전달에 사용할 방향
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FVector Direction = FVector::ForwardVector;

	// Trail 종료 위치 추가
	void AddTrailEnd(const FVector& InEndLocation);

	// 대표 Trail 종료 위치 반환
	FVector GetPrimaryTrailEnd() const;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRFXFirePayload : public FPRFXPayloadBase
{
	GENERATED_BODY()

	// 발사를 발생시킨 Actor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	TObjectPtr<AActor> SourceActor;

	// 발사 시점의 무기 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	TObjectPtr<UPRWeaponDataAsset> WeaponData;
	
	// 방향 계산과 Niagara 파라미터 전달에 사용할 방향
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FVector Direction = FVector::ForwardVector;
	
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPRFXTrailPayload> : public TStructOpsTypeTraitsBase2<FPRFXTrailPayload>
{
	enum { WithNetSerializer = true };
};

// 탄착과 피격 FX의 표면 분기 정보
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFXHitPayload : public FPRFXPayloadBase
{
	GENERATED_BODY()

	// 탄착 또는 피격 위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FVector ImpactLocation = FVector::ZeroVector;

	// 표면 방향과 Decal 방향 계산에 사용할 법선
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FVector ImpactNormal = FVector::UpVector;

	// 표면별 VFX와 SFX 분기에 사용할 GameplayTag
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FGameplayTag SurfaceTag;

	// 피격된 액터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	TObjectPtr<AActor> HitActor;

	// 피격된 컴포넌트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	TObjectPtr<UPrimitiveComponent> HitComponent;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPRFXHitPayload> : public TStructOpsTypeTraitsBase2<FPRFXHitPayload>
{
	enum { WithNetSerializer = true };
};

// 복합 연출 액터 스폰 정보
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFXActorSpawnPayload : public FPRFXPayloadBase
{
	GENERATED_BODY()

	// Actor가 생성될 위치와 회전
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FTransform SpawnTransform = FTransform::Identity;

	// 스폰 Actor의 소유자
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	TObjectPtr<AActor> OwnerActor;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPRFXActorSpawnPayload> : public TStructOpsTypeTraitsBase2<FPRFXActorSpawnPayload>
{
	enum { WithNetSerializer = true };
};

// 상태 지속 FX의 시작과 종료 정보
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFXPersistentPayload : public FPRFXPayloadBase
{
	GENERATED_BODY()

	// 시작 요청과 종료 요청을 연결하는 식별자
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FPRFXPersistentId PersistentId;

	// 지속 FX를 시작할지 종료할지 구분하는 요청 의도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	EPRFXPersistentAction Action = EPRFXPersistentAction::Start;

	// 지속 FX가 붙거나 따라갈 대상 액터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	TObjectPtr<AActor> TargetActor;

	// 대상 액터 내부 부착 소켓
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FName SocketName = NAME_None;

	// 상태 출처나 세부 분기에 사용할 Context 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|FX")
	FGameplayTag ContextTag;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPRFXPersistentPayload> : public TStructOpsTypeTraitsBase2<FPRFXPersistentPayload>
{
	enum { WithNetSerializer = true };
};

uint32 GetTypeHash(const FPRFXPredictionKey& Key);
uint32 GetTypeHash(const FPRFXPersistentId& Id);
