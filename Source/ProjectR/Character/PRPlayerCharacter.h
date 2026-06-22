// Author: 김동석 (이동 물리(Sprint/Crouch), 조준 카메라 줌 및 캐릭터 다운/사망 상태 관리 구현)
// Author: 배유찬 (장비 외형 장착, 플래시라이트 및 HUD/상호작용 연동 등 플레이어 코어 기능 구현)
// Author: 손승우 (보스 조우 시네마틱 및 보스전 이벤트 연동)
// Author: 이건주 (인벤토리 기반 무기 장착 연동 및 카메라 감도 설정 구현)
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PRCharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/Interaction/PRInteractionInterface.h"
#include "ProjectR/ItemSystem/Types/PREquipmentTypes.h"
#include "PRPlayerCharacter.generated.h"

class APRPlayerState;
class UPRCameraModifier;
class UPREquipmentDataAsset;
class UPREquipmentManagerComponent;
class USphereComponent;
class USkeletalMesh;
class UPRInteractableComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UPRWeaponManagerComponent;
class UPRSpringArmComponent;
class UPRActionInputRouterComponent;
class UPRFirePreviewComponent;
class UPRFlashlightComponent;
class UPRConsumableDataAsset;
class UStaticMeshComponent;
class UPRPlayerHitSoundDataAsset;
class USoundBase;
struct FInputActionValue;
struct FOnAttributeChangeData;
//무기 테스트용
class UPRWeaponDataAsset;

UCLASS()
class PROJECTR_API APRPlayerCharacter : public APRCharacterBase, public IPRInteractionInterface
{
	GENERATED_BODY()

public:
	APRPlayerCharacter();
	
	/** 멀티플레이어 변수 복제 설정 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/*~ APawn Interface ~*/
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	/*~ ACharacter Interface ~*/
	virtual void Crouch(bool bClientSimulation = false) override;
	virtual void UnCrouch(bool bClientSimulation = false) override;
	
	/*~ APRCharacterBase Interface ~*/
	virtual UPRAbilitySystemComponent* GetPRAbilitySystemComponent() const override;

	/*~ IPRCombatInterface ~*/
	virtual EPRTeam GetTeam() const override { return EPRTeam::Player; }
	// 체력 피해 적용 후 피드백 처리
	virtual void OnPostDamageApplied(const FPRDamageAppliedContext& Context) override;

	/*~ IPRInteractionInterface ~*/
	virtual UPRInteractableComponent* GetInteractableComponent() const override { return InteractableComponent; }
	
	/*~ APRPlayerCharacter Interface ~*/
	/** 애니메이션 인스턴스에서 사용하는 게터 함수들 */                   
	bool IsCrouching() const { return bIsCrouched; } 
	bool IsSprinting() const { return bIsSprinting; } 
	bool IsWalking() const { return bIsWalking; }
	bool IsDodging() const {return bIsDodging; }
	float GetWalkSpeed() const { return WalkSpeed; }
	float GetJogSpeed() const { return JogSpeed; }
	float GetSprintSpeed() const { return SprintSpeed; }
	float GetDownSpeed() const { return DownSpeed; }
	/** 애니메이션 재생 속도에 사용할 이동속도 배율 반환 */
	float GetMovementSpeedMultiplier() const;
	bool IsAiming() const;
	bool IsDown() const;
	bool IsMovementBlocked() const {return bBlockMove;}
	
	/** Sprint Ability가 질주 상태를 캐릭터 이동 상태에 반영 */
	void SetSprintingFromAbility(bool bNewSprinting);

	/** 서버가 결정한 외부 강제 이동을 소유 클라이언트에서도 동일하게 재생한다. */
	UFUNCTION(Client, Reliable)
	void ClientStartExternalForcedMove(FVector_NetQuantize Destination, FRotator Rotation, float Duration, float TickInterval, float EaseExponent, bool bSweep, bool bStopMovement);

	/** 외부 강제 이동 카메라 Lag override 상태 갱신 */
	void SetExternalForcedMoveCameraLagOverride(bool bEnabled);

	
	
	void SetFlashlightEnabled(bool bEnabled) const;
	
	UFUNCTION(Server, Reliable)
	void ServerSetFlashlightEnabled(bool bEnabled) const;
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetFlashlightEnabled(bool bEnabled) const;
	
	bool IsFlashlightEnabled() const;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetMovementMode(EMovementMode NewMovementMode);

	// ===== Component getters =====
	
	/** 액션 입력 라우터 컴포넌트를 반환 */
	UPRActionInputRouterComponent* GetActionInputRouter() const { return ActionInputRouterComponent; }

	/** 플래시라이트 컴포넌트 반환 */
	UFUNCTION(BlueprintPure, Category = "PR|Flashlight")
	UPRFlashlightComponent* GetFlashlightComponent() const { return FlashlightComponent; }

	UFUNCTION(BlueprintPure, Category = "PR|Weapon")
	UPRWeaponManagerComponent* GetWeaponManager() const;

	// 장비 매니저 컴포넌트 반환
	UFUNCTION(BlueprintPure, Category = "PR|Equipment")
	UPREquipmentManagerComponent* GetEquipmentManager() const;

	// 지정 장비 슬롯에 대응하는 기본 메시 조회
	USkeletalMesh* GetDefaultEquipmentMesh(EPREquipmentSlotType SlotType) const;

	// 지정 컨트롤러의 PlayerState를 사용한 캐릭터 런타임 링크 초기화
	void InitCharacter(AController* SourceController);

	// 소비 아이템 PickupMesh 표시 요청
	void RequestConsumablePickupMeshBegin(UPRConsumableDataAsset* ConsumableData, FName AttachSocketName);

	// 소비 아이템 PickupMesh 제거 요청
	void RequestConsumablePickupMeshEnd();

	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/*~ APRCharacterBase Interface ~*/
	virtual void HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool bTagExists) override;

	/*~ APRPlayerCharacter Interface ~*/
	/** 입력 처리 함수 */
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	/** 현재 상태(질주, 조준 등)에 맞춰 MaxWalkSpeed를 업데이트한다 (클라이언트 예측용) */
	void UpdateMaxWalkSpeed();
	
private:
	// 체력 피해 피드백 사운드 소유 클라이언트 재생
	UFUNCTION(Client, Unreliable)
	void ClientPlayHealthDamageHitSound(USoundBase* Sound);

	/** 상태 태그 기준으로 이동 입력이 차단되는지 반환한다 */
	bool IsMoveInputLockedByState() const;

	// 현재 캐릭터 로직이 사용할 PlayerState 조회
	APRPlayerState* ResolvePlayerState() const;

	// PlayerState 기반 ASC와 무기, 장비 외형 링크 초기화
	void InitializeCharacterRuntime(APRPlayerState* SourcePlayerState, bool bInitializeAttributes);

	/** 외부 강제 이동을 현재 머신에서 시작한다. */
	void StartExternalForcedMoveLocal(const FVector& Destination, const FRotator& Rotation, float Duration, float TickInterval, float EaseExponent, bool bSweep, bool bStopMovement);

	/** 외부 강제 이동 보간을 갱신한다. */
	void TickExternalForcedMove();

	/** 외부 강제 이동을 종료하고 최종 위치를 보정한다. */
	void CompleteExternalForcedMove(bool bWasCancelled);
	/** 이동속도 배율 Attribute 변경 시 MaxWalkSpeed를 갱신하도록 바인딩한다 */
	void BindMovementSpeedAttributeChange();

	/** 이동속도 배율 Attribute 변경 바인딩을 해제한다 */
	void UnbindMovementSpeedAttributeChange();

	/** 이동속도 배율 Attribute 변경을 캐릭터 이동속도에 반영한다 */
	void HandleMovementSpeedMultiplierChanged(const FOnAttributeChangeData& ChangeData);

	// 장비 매니저 외형 변경 이벤트 바인딩
	void BindEquipmentManager();

	// 장비 매니저 외형 변경 이벤트 해제
	void UnbindEquipmentManager();

	// 현재 장비 외형 정보로 파츠 메시 갱신
	void ApplyEquipmentVisualsFromManager();

	// BP 파츠 컴포넌트에 지정된 메시를 기본 메시로 보관
	void CacheDefaultEquipmentMeshes();

	// 지정 슬롯에 장비 메시 또는 기본 메시 적용
	void ApplyEquipmentVisual(EPREquipmentSlotType SlotType, const UPREquipmentDataAsset* EquipmentData);

	// 머리 장비 설정에 따른 플레이어 얼굴 표시 상태 갱신
	void UpdatePlayerFaceVisibility(const UPREquipmentDataAsset* HeadEquipmentData);

	// 지정 장비 슬롯에 대응하는 파츠 컴포넌트 조회
	USkeletalMeshComponent* GetEquipmentMeshComponent(EPREquipmentSlotType SlotType) const;

	// 장비 외형 정보 변경 알림 처리
	UFUNCTION()
	void HandleEquipmentVisualInfosChanged(UPREquipmentManagerComponent* ChangedEquipmentManagerComponent);

	// 소비 아이템 PickupMesh 표시 서버 요청
	UFUNCTION(Server, Unreliable)
	void Server_RequestConsumablePickupMeshBegin(UPRConsumableDataAsset* ConsumableData, FName AttachSocketName);

	// 소비 아이템 PickupMesh 제거 서버 요청
	UFUNCTION(Server, Reliable)
	void Server_RequestConsumablePickupMeshEnd();

	// 소비 아이템 PickupMesh 표시 전파
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ShowConsumablePickupMesh(UPRConsumableDataAsset* ConsumableData, FName AttachSocketName);

	// 소비 아이템 PickupMesh 제거 전파
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_HideConsumablePickupMesh();

	// 소비 아이템 PickupMesh 표시 적용
	void ShowConsumablePickupMesh(UPRConsumableDataAsset* ConsumableData, FName AttachSocketName);

	// 소비 아이템 PickupMesh 제거 적용
	void HideConsumablePickupMesh();

public:
	/** 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> Mesh_Head;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> Mesh_PlayerFace;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> Mesh_Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> Mesh_Hands;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> Mesh_Legs;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> Mesh_BackPack;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> Mesh_Flashlight;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> Mesh_FlashlightGlow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UPRSpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** 몽타주 기반 액션 중 입력 차단과 스킵 요청을 중계하는 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UPRActionInputRouterComponent> ActionInputRouterComponent;

	/** 발사 예측 경로 표시 컴포넌트. 로컬 시각 효과 전용 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UPRFirePreviewComponent> FirePreviewComponent;

	/** 로컬 플레이어 조준 방향 플래시라이트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light")
	TObjectPtr<UPRFlashlightComponent> FlashlightComponent;
	
	// 상호작용 타겟 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UPRInteractableComponent> InteractableComponent;
	
	// 상호작용 collider
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USphereComponent> InteractionSphere;
	
	// 감도 최신화
	UFUNCTION()
	void HandleMouseSensitivityChanged(float NewSensitivity);

protected:
	/** Enhanced Input 에셋 (블루프린트에서 할당) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> WalkAction;

	/** 조준/느린 이동 시 속도 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Locomotion")
	float WalkSpeed = 200.0f;

	/** 기본 조깅 속도 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Locomotion")
	float JogSpeed = 350.0f;

	/** 질주 속도 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Locomotion")
	float SprintSpeed = 600.0f;

	/** 다운 상태 기어가기 속도 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Locomotion")
	float DownSpeed = 50.0f;
	
	/** 카메라 감도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Camera")
	float CachedCameraSensitivity = 0.5f;

	// 체력 피해 피드백 사운드 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PR|Audio|Hit")
	TObjectPtr<UPRPlayerHitSoundDataAsset> HitSoundData;

	/** 플래시 라이트 위치 설정 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Flashlight")
	FVector FlashlightStandingLocation = FVector(50.0f, 0.0f, 55.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Flashlight")
	FVector FlashlightCrouchingLocation = FVector(50.0f, 0.0f, 22.0f);

	// 머리 슬롯 해제 시 복원할 기본 메시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PR|Equipment")
	TObjectPtr<USkeletalMesh> DefaultHeadMesh;

	// 몸통 슬롯 해제 시 복원할 기본 메시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PR|Equipment")
	TObjectPtr<USkeletalMesh> DefaultBodyMesh;

	// 손 슬롯 해제 시 복원할 기본 메시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PR|Equipment")
	TObjectPtr<USkeletalMesh> DefaultHandsMesh;

	// 다리 슬롯 해제 시 복원할 기본 메시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PR|Equipment")
	TObjectPtr<USkeletalMesh> DefaultLegsMesh;

private:
	bool bIsSprinting = false;
	bool bIsAiming = false;
	bool bIsWalking = false;
	bool bIsDodging = false;
	
	bool bBlockMove = false;

	// 마지막 체력 피해 사운드 재생 시각
	float LastHealthDamageHitSoundTime = -FLT_MAX;

	// 기본 머리 메시 캐시 완료 여부
	bool bDefaultHeadMeshCached = false;

	// 기본 몸통 메시 캐시 완료 여부
	bool bDefaultBodyMeshCached = false;

	// 기본 손 메시 캐시 완료 여부
	bool bDefaultHandsMeshCached = false;

	// 기본 다리 메시 캐시 완료 여부
	bool bDefaultLegsMeshCached = false;
	
	UPROPERTY()
	TObjectPtr<UPRCameraModifier> CrouchCameraModifier; 

	FTimerHandle ExternalForcedMoveTimerHandle;
	FVector ExternalForcedMoveStartLocation = FVector::ZeroVector;
	FVector ExternalForcedMoveEndLocation = FVector::ZeroVector;
	FRotator ExternalForcedMoveStartRotation = FRotator::ZeroRotator;
	FRotator ExternalForcedMoveEndRotation = FRotator::ZeroRotator;
	float ExternalForcedMoveDuration = 0.0f;
	float ExternalForcedMoveElapsedSeconds = 0.0f;
	float ExternalForcedMoveLastUpdateTime = 0.0f;
	float ExternalForcedMoveTickInterval = 0.016f;
	float ExternalForcedMoveEaseExponent = 2.0f;
	bool bExternalForcedMoveSweep = false;
	bool bExternalForcedMoveStopMovement = true;
	bool bExternalForcedMoveActive = false;

	// 비소유 프리뷰 캐릭터가 참조할 PlayerState
	UPROPERTY(Transient)
	TObjectPtr<APRPlayerState> CachedPlayerState;

	// 현재 외형 변경 이벤트를 받고 있는 장비 매니저
	UPROPERTY(Transient)
	TObjectPtr<UPREquipmentManagerComponent> BoundEquipmentManager;

	// 현재 표시 중인 소비 아이템 PickupMesh 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ActiveConsumablePickupMeshComponent;
};
