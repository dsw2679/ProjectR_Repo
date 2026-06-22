// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (피격 데칼 및 탄착점 이펙트 관련 데이터 구조 정의)
// Author: 손승우 (보스 시퀀스 관련 이펙트 타입 정의)
#include "PRFXTypes.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"

namespace PRFXTypesPrivate
{
	// RPC로 전송할 Trail 종료 위치 최대 개수
	constexpr int32 MaxTrailEndLocationCount = 32;

	// UObject 참조를 Unreal 네트워크 ObjectReference 방식으로 저장하고 읽는 보조 함수
	template <typename TObjectType>
	void NetSerializeObject(FArchive& Ar, TObjectPtr<TObjectType>& Object)
	{
		UObject* RawObject = Object.Get();
		Ar << RawObject;
		if (Ar.IsLoading())
		{
			// 수신된 UObject를 호출자가 기대한 클래스 타입으로 제한해서 저장
			Object = Cast<TObjectType>(RawObject);
		}
	}

	// UENUM 값을 RPC 바이트 스트림에 넣기 위한 uint8 변환 함수
	template <typename TEnum>
	void NetSerializeEnum(FArchive& Ar, TEnum& Value)
	{
		uint8 RawValue = static_cast<uint8>(Value);
		Ar << RawValue;
		if (Ar.IsLoading())
		{
			Value = static_cast<TEnum>(RawValue);
		}
	}

	// bool 값을 1비트로 압축해서 RPC 바이트 스트림에 넣는 보조 함수
	void NetSerializeBool(FArchive& Ar, bool& bValue)
	{
		uint8 RawValue = bValue ? 1 : 0;
		Ar.SerializeBits(&RawValue, 1);
		if (Ar.IsLoading())
		{
			bValue = RawValue != 0;
		}
	}
}

void FPRFXTrailPayload::AddTrailEnd(const FVector& InEndLocation)
{
	// 다중 Trail 종료 위치 누적
	TrailEnds.Add(InEndLocation);
}

FVector FPRFXTrailPayload::GetPrimaryTrailEnd() const
{
	// 단일 Trail 연출에서 사용할 대표 종료 위치
	return TrailEnds.Num() > 0 ? TrailEnds[0] : FVector::ZeroVector;
}

bool FPRFXPredictionKey::IsValid() const
{
	return bValid && SourceId.IsValid() && Sequence > 0;
}

bool FPRFXPredictionKey::operator==(const FPRFXPredictionKey& Other) const
{
	return SourceId == Other.SourceId
		&& Sequence == Other.Sequence
		&& bValid == Other.bValid;
}

bool FPRFXPredictionKey::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	// 로컬 예측 재생과 서버 확정 재생을 비교하기 위한 식별자 전송
	Ar << SourceId;
	Ar << Sequence;
	PRFXTypesPrivate::NetSerializeBool(Ar, bValid);

	bOutSuccess = true;
	return true;
}

bool FPRFXPersistentId::IsValid() const
{
	return SlotTag.IsValid() && InstanceSequence > 0;
}

bool FPRFXPersistentId::operator==(const FPRFXPersistentId& Other) const
{
	return SlotTag == Other.SlotTag
		&& InstanceSequence == Other.InstanceSequence;
}

bool FPRFXPersistentId::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool bLocalSuccess = true;

	// 지속 FX 시작 요청과 종료 요청을 같은 슬롯으로 연결하기 위한 태그와 순번 전송
	SlotTag.NetSerialize(Ar, Map, bLocalSuccess);
	Ar << InstanceSequence;

	bOutSuccess = bLocalSuccess;
	return true;
}

bool FPRFXRequest::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool bLocalSuccess = true;
	bool bFieldSuccess = true;

	// RPC에서 Cue 클래스와 Registry Entry를 제외하고 태그, Payload, 예측 키만 전송
	FXTag.NetSerialize(Ar, Map, bFieldSuccess);
	bLocalSuccess &= bFieldSuccess;
	Payload.NetSerialize(Ar, Map, bFieldSuccess);
	bLocalSuccess &= bFieldSuccess;
	PredictionKey.NetSerialize(Ar, Map, bFieldSuccess);
	bLocalSuccess &= bFieldSuccess;

	bOutSuccess = bLocalSuccess;
	return true;
}

bool FPRFXPayloadBase::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	// 모든 Payload가 공유하는 선택적 수명 파라미터 전송
	PRFXTypesPrivate::NetSerializeBool(Ar, bHasLifeTime);
	Ar << LifeTime;

	bOutSuccess = true;
	return true;
}

bool FPRFXLocationPayload::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool bLocalSuccess = true;

	// 월드 위치만으로 재생되는 FX에 필요한 위치, 회전, 스케일 전송
	FPRFXPayloadBase::NetSerialize(Ar, Map, bLocalSuccess);
	Location.NetSerialize(Ar, Map, bLocalSuccess);
	Rotation.NetSerialize(Ar, Map, bLocalSuccess);
	Scale.NetSerialize(Ar, Map, bLocalSuccess);

	bOutSuccess = bLocalSuccess;
	return true;
}

bool FPRFXAttachPayload::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool bLocalSuccess = true;

	// 부착 대상 객체, 소켓 이름, 상대 위치와 회전을 함께 전송
	FPRFXPayloadBase::NetSerialize(Ar, Map, bLocalSuccess);
	PRFXTypesPrivate::NetSerializeObject(Ar, AttachActor);
	PRFXTypesPrivate::NetSerializeObject(Ar, AttachComponent);
	Ar << SocketName;
	RelativeLocation.NetSerialize(Ar, Map, bLocalSuccess);
	RelativeRotation.NetSerialize(Ar, Map, bLocalSuccess);

	bOutSuccess = bLocalSuccess;
	return true;
}

bool FPRFXTrailPayload::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool bLocalSuccess = true;

	// 무기 발사 Trail 식별값과 경로 데이터 전송
	FPRFXPayloadBase::NetSerialize(Ar, Map, bLocalSuccess);
	PRFXTypesPrivate::NetSerializeObject(Ar, SourceActor);
	PRFXTypesPrivate::NetSerializeObject(Ar, WeaponData);
	StartLocation.NetSerialize(Ar, Map, bLocalSuccess);

	int32 TrailEndLocationCount = TrailEnds.Num();
	if (Ar.IsSaving())
	{
		TrailEndLocationCount = FMath::Min(TrailEndLocationCount, PRFXTypesPrivate::MaxTrailEndLocationCount);
	}
	Ar << TrailEndLocationCount;
	TrailEndLocationCount = FMath::Clamp(TrailEndLocationCount, 0, PRFXTypesPrivate::MaxTrailEndLocationCount);
	if (Ar.IsLoading())
	{
		TrailEnds.SetNum(TrailEndLocationCount);
	}
	for (int32 EndLocationIndex = 0; EndLocationIndex < TrailEndLocationCount; ++EndLocationIndex)
	{
		// 다중 Trail 종료 위치 전송
		TrailEnds[EndLocationIndex].NetSerialize(Ar, Map, bLocalSuccess);
	}

	PRFXTypesPrivate::NetSerializeBool(Ar, bBlockingHit);
	if (bBlockingHit)
	{
		// 충돌 Trail에서만 피격 Actor와 표면 정보를 포함한 HitResult 전송
		HitResult.NetSerialize(Ar, Map, bLocalSuccess);
	}
	else if (Ar.IsLoading())
	{
		// 비충돌 Trail 수신 시 이전 구조체 재사용 값 제거
		HitResult = FHitResult();
	}
	Direction.NetSerialize(Ar, Map, bLocalSuccess);

	bOutSuccess = bLocalSuccess;
	return true;
}

bool FPRFXFirePayload::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool bLocalSuccess = true;
	// 무기 발사 Trail 식별값과 경로 데이터 전송
	FPRFXPayloadBase::NetSerialize(Ar, Map, bOutSuccess);
	PRFXTypesPrivate::NetSerializeObject(Ar, SourceActor);
	PRFXTypesPrivate::NetSerializeObject(Ar, WeaponData);
	
	bOutSuccess = bLocalSuccess;
	return true;
}

bool FPRFXHitPayload::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool bLocalSuccess = true;

	// 탄착 위치, 표면 분기 태그, 피격 Actor와 Component 참조 전송
	FPRFXPayloadBase::NetSerialize(Ar, Map, bLocalSuccess);
	ImpactLocation.NetSerialize(Ar, Map, bLocalSuccess);
	ImpactNormal.NetSerialize(Ar, Map, bLocalSuccess);
	SurfaceTag.NetSerialize(Ar, Map, bLocalSuccess);
	PRFXTypesPrivate::NetSerializeObject(Ar, HitActor);
	PRFXTypesPrivate::NetSerializeObject(Ar, HitComponent);

	bOutSuccess = bLocalSuccess;
	return true;
}

bool FPRFXActorSpawnPayload::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool bLocalSuccess = true;
	FPRFXPayloadBase::NetSerialize(Ar, Map, bLocalSuccess);

	// FTransform을 위치, 회전, 스케일로 분리해서 Unreal 기본 NetSerialize 사용
	FVector SpawnLocation = SpawnTransform.GetLocation();
	FRotator SpawnRotation = SpawnTransform.Rotator();
	FVector SpawnScale = SpawnTransform.GetScale3D();
	SpawnLocation.NetSerialize(Ar, Map, bLocalSuccess);
	SpawnRotation.NetSerialize(Ar, Map, bLocalSuccess);
	SpawnScale.NetSerialize(Ar, Map, bLocalSuccess);
	if (Ar.IsLoading())
	{
		// 수신된 위치, 회전, 스케일을 다시 FTransform으로 조립
		SpawnTransform = FTransform(SpawnRotation, SpawnLocation, SpawnScale);
	}

	PRFXTypesPrivate::NetSerializeObject(Ar, OwnerActor);

	bOutSuccess = bLocalSuccess;
	return true;
}

bool FPRFXPersistentPayload::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool bLocalSuccess = true;
	bool bFieldSuccess = true;

	// 지속 FX 시작과 종료를 PersistentId로 매칭하기 위한 요청 데이터 전송
	FPRFXPayloadBase::NetSerialize(Ar, Map, bLocalSuccess);
	PersistentId.NetSerialize(Ar, Map, bFieldSuccess);
	bLocalSuccess &= bFieldSuccess;
	PRFXTypesPrivate::NetSerializeEnum(Ar, Action);
	PRFXTypesPrivate::NetSerializeObject(Ar, TargetActor);
	Ar << SocketName;
	ContextTag.NetSerialize(Ar, Map, bFieldSuccess);
	bLocalSuccess &= bFieldSuccess;

	bOutSuccess = bLocalSuccess;
	return true;
}

uint32 GetTypeHash(const FPRFXPredictionKey& Key)
{
	return HashCombine(GetTypeHash(Key.SourceId), HashCombine(GetTypeHash(Key.Sequence), GetTypeHash(static_cast<uint8>(Key.bValid ? 1 : 0))));
}

uint32 GetTypeHash(const FPRFXPersistentId& Id)
{
	return HashCombine(GetTypeHash(Id.SlotTag), GetTypeHash(Id.InstanceSequence));
}
