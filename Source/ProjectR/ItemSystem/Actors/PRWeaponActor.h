// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (무기 스켈레탈 메시 조작 애니메이션 연동)
// Author: 배유찬 (무기 사격 이펙트(Tracer), 재장전 애니메이션 및 투사체 발사 제어 구현)
// Author: 손승우 (보스용 특수 타격 판정 무기 연동)
// Author: 이건주 (인벤토리 연동 및 3D 프리뷰 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectR/ItemSystem/Types/PRWeaponAnimationTypes.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRWeaponActor.generated.h"

enum class EPRArmedState : uint8;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class ACharacter;
class USceneComponent;
class USkeletalMeshComponent;
class UPRWeaponAnimInstance;

// Trail 끝점 파라미터 타입
UENUM(BlueprintType)
enum class EPRTrailEndParamType : uint8
{
	SingleFloat3,
	ArrayFloat3
};

// 무기 Actor가 발사 FX 재생에 사용하는 전용 파라미터
USTRUCT(BlueprintType)
struct PROJECTR_API FPRWeaponFireFXParams
{
	GENERATED_BODY()

	// Trail 시작 위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon|FX")
	FVector StartLocation = FVector::ZeroVector;

	// Trail 종료 위치 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon|FX")
	TArray<FVector> TrailEnds;

	// Trail 충돌 도달 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon|FX")
	bool bBlockingHit = false;

	// Trail 방향
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectR|Weapon|FX")
	FVector Direction = FVector::ForwardVector;

	// Trail 존재 여부
	bool HasTrail() const {return !TrailEnds.IsEmpty();}
	
	// Trail 종료 위치 추가
	void AddTrailEnd(const FVector& InEndLocation);

	// 대표 Trail 종료 위치 반환
	FVector GetPrimaryTrailEnd() const;
};

/**
 * 플레이어 WeaponManager 슬롯에 장착되는 무기의 VisualActor로, 각 클라이언트가 로컬에 Spawn함
 *
 * 발사 FX 재생 흐름
 * UPRFXWeaponFireTrailCue가 현재 손에 든 무기 Actor를 찾은 뒤 TriggerFireFX 호출
 * TriggerFireFX는 PlayFireSFX, PlayMuzzleFlashVFX, PlayTrailVFX를 순서대로 호출
 * 각 함수는 BlueprintNativeEvent이므로 무기 BP에서 필요한 단계만 교체 가능
 *
 * 기본 설정 방법
 * FireSFX에는 발사음 지정
 * MuzzleFlashVFX에는 총구 화염 Niagara System 지정
 * TrailVFX에는 캐시 컴포넌트가 없을 때 SpawnSystemAtLocation으로 생성할 Niagara System 지정
 * TrailStartParamName에는 Trail 시작점 Vector User Parameter 이름 지정
 * TrailEndParamName에는 Trail 끝점 User Parameter 이름 지정
 * TrailEndParamType이 SingleFloat3이면 대표 끝점 하나를 Vector로 주입
 * TrailEndParamType이 ArrayFloat3이면 모든 끝점을 Niagara Array Float 3로 주입
 *
 * 캐시 컴포넌트 방식
 * 무기 BP에 Trail NiagaraComponent를 미리 배치한 경우 GetCachedTrailComponent를 override해 해당 컴포넌트 반환
 * 반환값이 유효하면 PlayTrailVFX는 새 컴포넌트를 생성하지 않고 위치, 회전, User Parameter만 갱신
 * 반환값이 없으면 TrailVFX를 SpawnSystemAtLocation으로 생성
 *
 * Trigger 파라미터 방식
 * Niagara가 User.Trigger 같은 bool 값으로 Spawn을 제어한다면 bUseTrailTriggerParam 활성화
 * TrailTriggerParamName에는 해당 bool User Parameter 이름 지정
 * PlayTrailVFX는 파라미터 주입 후 Trigger를 true로 올리고 TrailTriggerHoldTime 이후 false로 변경
 * 새 발사가 들어오면 이전 Trigger 초기화 타이머를 취소하고 다시 예약
 */
UCLASS()
class PROJECTR_API APRWeaponActor : public AActor
{
	GENERATED_BODY()

public:
	// 로컬 공개 표현 Actor를 초기화한다
	APRWeaponActor();

	/*~ AActor Interface ~*/
	// 초기 생성 이후 추가 초기화 지점을 제공한다
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:
	// 전달받은 소켓 이름 기준으로 캐릭터 메시 소켓에 자신을 부착한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	void AttachToOwnerMesh(ACharacter* OwnerCharacter, FName SocketName);

	// 무기 스켈레탈 메시 컴포넌트를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	USkeletalMeshComponent* GetWeaponMeshComponent() const { return WeaponMeshComponent; }

	// 무기 메시에서 ProjectR 무기 AnimInstance를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon|Animation")
	UPRWeaponAnimInstance* GetWeaponAnimInstance() const;

	// 무기 AnimInstance에 지정 애니메이션 상태 요청을 전달한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|Animation")
	bool RequestWeaponAnimation(EPRWeaponAnimationState AnimationState);

	// 무기 AnimInstance에 발사 애니메이션 요청을 전달한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|Animation")
	bool RequestShootAnimation();

	// 무기 AnimInstance에 재장전 애니메이션 요청을 전달한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|Animation")
	bool RequestReloadAnimation();

	// 무기 AnimInstance를 Idle 상태로 되돌린다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|Animation")
	bool SetWeaponAnimationIdle();

	// 26.04.26, Yuchan, 머즐 트랜스폼 반환 함수가 없어 아래 코드 임시 추가.
	// 총구 트랜스폼 반환 함수, BP에서 override
	UFUNCTION(BlueprintNativeEvent)
	FTransform GetMuzzleTransform() const;

	// 발사 FX 통합 진입점
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon|FX")
	void TriggerFireFX(const FPRWeaponFireFXParams& Params);

	// 발사 SFX 재생
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|Weapon|FX")
	void PlayFireSFX(const FPRWeaponFireFXParams& Params);

	// 총구 화염 VFX 재생
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|Weapon|FX")
	void PlayMuzzleFlashVFX(const FPRWeaponFireFXParams& Params);

	// Trail VFX 재생
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectR|Weapon|FX")
	void PlayTrailVFX(const FPRWeaponFireFXParams& Params);

	// 재사용할 Trail Niagara Component 반환
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "ProjectR|Weapon|FX")
	UNiagaraComponent* GetCachedTrailComponent() const;
	
	// 레거시 Niagara 재생 경로, 제거 예정
	UFUNCTION(BlueprintNativeEvent)
	void PlayNiagaraEffect(EPRWeaponEffectType EffectType, UNiagaraSystem* InNiagaraSystem = nullptr);
	
	// 총기 관련 이펙트 중단, BP에서 선택적 구현
	UFUNCTION(BlueprintImplementableEvent)
	void StopEffect(EPRWeaponEffectType EffectType);
	
	// 무기 기준으로 왼손 IK 적용 가능 여부를 반환
	UFUNCTION(BlueprintPure)
	virtual bool ShouldApplyLeftHandIK() const;
	
	// IK 보정 억제 상태를 설정
	UFUNCTION(BlueprintCallable)
	void SetIsIKSuppressed(bool bInSuppressed, float InitialDelay = 0.f);
	
	// IK 보정 억제 상태를 반환
	UFUNCTION(BLueprintPure)
	bool IsIKSuppressed() const { return bIsIKSuppressed; }
	
protected:
	void Internal_SetIsIKSuppressed();

	// Trail Trigger 파라미터 초기화
	void ResetTrailTriggerParam();
	
public:
	// 이 무기가 왼손 IK 보정을 사용할지 결정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	bool bUseLeftHandIK = true;
	
	// 왼손이 따라갈 무기 메시 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	FName LeftHandIKSocketName = FName(TEXT("Socket_LeftHandIK"));

	// 무기 소켓 위치에서 추가로 적용할 왼손 보정 트랜스폼
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	FTransform LeftHandIKOffset = FTransform::Identity;
	
protected:
	// 발사 시점 SFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Weapon|FX")
	TObjectPtr<USoundBase> FireSFX;

	// 발사 Trail VFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Weapon|FX")
	TObjectPtr<UNiagaraSystem> TrailVFX;

	// 총구 화염 VFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Weapon|FX")
	TObjectPtr<UNiagaraSystem> MuzzleFlashVFX;

	// Trail 시작점 Niagara User Parameter 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Weapon|FX")
	FName TrailStartParamName = FName(TEXT("User.TrailStart"));

	// Trail 끝점 Niagara User Parameter 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Weapon|FX")
	FName TrailEndParamName = FName(TEXT("User.TrailEnd"));

	// Trail 끝점 Niagara User Parameter 타입
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Weapon|FX")
	EPRTrailEndParamType TrailEndParamType = EPRTrailEndParamType::SingleFloat3;

	// Trail Trigger 파라미터 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Weapon|FX")
	bool bUseTrailTriggerParam = false;

	// Trail Trigger Niagara User Parameter 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Weapon|FX")
	FName TrailTriggerParamName = FName(TEXT("User.Trigger"));
	
	// Trail Trigger 유지 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Weapon|FX", meta = (ClampMin = "0.0", Units = "s"))
	float TrailTriggerHoldTime = 0.03f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;
	
private:
	bool bPendingIKSuppressed = false;
	// IK 보정 억제 상태
	bool bIsIKSuppressed = false;
	
	FTimerHandle IKSuppressedTimerHandle;

	// 다음 틱 Trigger 초기화 예약 핸들
	FTimerHandle TrailTriggerResetTimerHandle;

	// Trigger 초기화 대상 Niagara Component
	TWeakObjectPtr<UNiagaraComponent> PendingTrailTriggerComponent;
};
