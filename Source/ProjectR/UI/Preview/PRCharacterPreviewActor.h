// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (인벤토리 연동 장착 장비 실시간 비주얼 변경 구현)
// Author: 손승우 (프리뷰 상태 리셋 연동)
// Author: 이건주 (캐릭터 마우스 조작 회전 및 조명 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/ItemSystem/Types/PRWeaponTypes.h"
#include "PRCharacterPreviewActor.generated.h"

class APRPlayerCharacter;
class APRWeaponActor;
class UDirectionalLightComponent;
class UMeshComponent;
class USceneCaptureComponent2D;
class USceneComponent;
class USkeletalMeshComponent;
class USpringArmComponent;
class USpotLightComponent;
class UTextureRenderTarget2D;
class UPREquipmentDataAsset;
class UPRWeaponManagerComponent;

// 인벤토리 캐릭터 프리뷰를 위한 별도 3D 렌더 액터
UCLASS()
class PROJECTR_API APRCharacterPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	// 프리뷰 액터 기본 컴포넌트 초기화
	APRCharacterPreviewActor();

	// 프리뷰 캡처가 사용할 렌더 타겟 설정
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Preview")
	void SetRenderTargetToSceneCapture(UTextureRenderTarget2D* InRenderTarget);

	// 플레이어 캐릭터의 외형과 장착 무기 상태를 프리뷰에 반영함
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Preview")
	void RefreshPreviewActorFromPlayer(APRPlayerCharacter* SourceCharacter, UPRWeaponManagerComponent* WeaponManagerComponent);

	// 저장 데이터의 외형과 장착 무기 상태를 프리뷰에 반영함
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Preview")
	void RefreshPreviewActorFromSaveData(const FPRCharacterSaveData& SaveData);

	// 프리뷰 캡처 즉시 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Preview")
	void CapturePreview();

	// 프리뷰 캡처 매 프레임 갱신 여부 설정
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Preview")
	void SetContinuousCapture(bool bEnabled);

	// 프리뷰 메시 텍스처 스트리밍 준비
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Preview")
	void InitTextureStreaming(float StreamingDuration, float StreamingDistanceMultiplier);

protected:
	/*~ AActor Interface ~*/
	// 액터 제거 시 생성한 무기 프리뷰 액터 정리
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 캐릭터 메시를 소스 캐릭터 기준으로 갱신함
	void RefreshCharacterMesh(APRPlayerCharacter* SourceCharacter);

	// 프리뷰 파츠 메시를 소스 파츠 메시 기준으로 갱신
	void RefreshCharacterPartMesh(USkeletalMeshComponent* PreviewPartMeshComponent, USkeletalMeshComponent* SourcePartMeshComponent);

	// 무기 매니저의 공개 비주얼 상태를 기준으로 무기 프리뷰 갱신
	void RefreshWeaponPreview(UPRWeaponManagerComponent* WeaponManagerComponent);

	// 저장 데이터의 공개 비주얼 상태를 기준으로 무기 프리뷰 갱신
	void RefreshWeaponPreviewFromSaveData(const FPRCharacterSaveData& SaveData);

	// 저장 데이터의 장비 상태를 기준으로 캐릭터 파츠 메시 갱신
	void RefreshCharacterMeshFromSaveData(const FPRCharacterSaveData& SaveData);

	// 프리뷰 얼굴 파츠 표시 상태 갱신
	void UpdatePreviewPlayerFaceVisibility(const UPREquipmentDataAsset* HeadEquipmentData);

	// 프리뷰 캐릭터 클래스의 기본 오브젝트 조회
	const APRPlayerCharacter* GetPreviewCharacterDefaultObject() const;

	// 프리뷰 캐릭터 기본 메시를 저장 데이터 프리뷰의 기본 외형으로 적용
	void ApplyDefaultPreviewCharacterMesh();

	// 프리뷰 캐릭터 기본 장비 메시와 파츠 트랜스폼을 저장 데이터 프리뷰 기본 파츠로 적용
	void ApplyDefaultPreviewPartMesh(USkeletalMeshComponent* PreviewPartMeshComponent, const APRPlayerCharacter* DefaultCharacter, const USkeletalMeshComponent* SourcePartMeshComponent, EPREquipmentSlotType SlotType, USkeletalMesh* FallbackMesh) const;

	// 저장 데이터에서 지정 슬롯의 장비 메시 조회
	USkeletalMesh* ResolveEquipmentMeshFromSaveData(const FPRCharacterSaveData& SaveData, EPREquipmentSlotType SlotType) const;

	// 저장 데이터에서 지정 슬롯의 장비 데이터 조회
	const UPREquipmentDataAsset* ResolveEquipmentDataFromSaveData(const FPRCharacterSaveData& SaveData, EPREquipmentSlotType SlotType) const;

	// 저장 데이터에서 지정 슬롯의 무기 공개 비주얼 정보 조회
	FPRWeaponVisualInfo ResolveWeaponVisualInfoFromSaveData(const FPRCharacterSaveData& SaveData, EPRWeaponSlotType SlotType) const;

	// 지정 슬롯의 무기 프리뷰 액터 생성 또는 갱신
	void RefreshWeaponActorForSlot(EPRWeaponSlotType SlotType, const FPRWeaponVisualInfo& VisualInfo);

	// 지정 슬롯의 무기 프리뷰 액터 제거
	void DestroyWeaponActorForSlot(EPRWeaponSlotType SlotType);

	// 지정 슬롯의 무기 프리뷰 액터 반환
	APRWeaponActor* GetWeaponActorBySlot(EPRWeaponSlotType SlotType) const;

	// 지정 슬롯의 무기 프리뷰 액터 참조 설정
	void SetWeaponActorBySlot(EPRWeaponSlotType SlotType, APRWeaponActor* NewWeaponActor);

	// 지정 슬롯의 무기 프리뷰 액터 참조 비움
	void ClearWeaponActorBySlot(EPRWeaponSlotType SlotType);

	// 프리뷰 메시 컴포넌트 텍스처 스트리밍 설정
	void InitMeshTextureStreaming(USkeletalMeshComponent* MeshComponent, float StreamingDuration, float StreamingDistanceMultiplier) const;

protected:
	// 프리뷰 카메라와 조명의 고정 기준 루트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USceneComponent> PreviewRootComponent;

	// 프리뷰 포즈와 무기 소켓 기준으로 사용하는 숨김 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USkeletalMeshComponent> PreviewMeshComponent;

	// 프리뷰 머리 파츠 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USkeletalMeshComponent> PreviewHeadMeshComponent;

	// 프리뷰 플레이어 얼굴 파츠 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USkeletalMeshComponent> PreviewPlayerFaceMeshComponent;

	// 프리뷰 몸통 파츠 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USkeletalMeshComponent> PreviewBodyMeshComponent;

	// 프리뷰 손 파츠 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USkeletalMeshComponent> PreviewHandsMeshComponent;

	// 프리뷰 다리 파츠 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USkeletalMeshComponent> PreviewLegsMeshComponent;

	// 프리뷰 캡처 거리와 각도를 제어하는 스프링 암
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USpringArmComponent> CaptureSpringArmComponent;

	// 프리뷰 이미지를 생성하는 캡처 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USceneCaptureComponent2D> SceneCaptureComponent;

	// 프리뷰 조명 거리와 각도를 제어하는 스프링 암
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USpringArmComponent> LightSpringArmComponent;

	// 프리뷰 전용 조명
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USpotLightComponent> KeyLightComponent;

	// 프리뷰 전용 방향광
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<UDirectionalLightComponent> DirectionalLightComponent;

	// 저장 데이터 프리뷰 기본 외형을 읽을 플레이어 캐릭터 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TSubclassOf<APRPlayerCharacter> PreviewCharacterClass;

	// PreviewCharacterClass 기본 오브젝트에 리더 메시가 없을 때 사용할 폴백 리더 메시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USkeletalMesh> DefaultPreviewMesh;

	// PreviewCharacterClass 기본 오브젝트에 머리 메시가 없을 때 사용할 폴백 머리 메시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USkeletalMesh> DefaultPreviewHeadMesh;

	// PreviewCharacterClass 기본 오브젝트에 얼굴 메시가 없을 때 사용할 폴백 얼굴 메시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USkeletalMesh> DefaultPreviewPlayerFaceMesh;

	// PreviewCharacterClass 기본 오브젝트에 몸통 메시가 없을 때 사용할 폴백 몸통 메시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USkeletalMesh> DefaultPreviewBodyMesh;

	// PreviewCharacterClass 기본 오브젝트에 손 메시가 없을 때 사용할 폴백 손 메시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USkeletalMesh> DefaultPreviewHandsMesh;

	// PreviewCharacterClass 기본 오브젝트에 다리 메시가 없을 때 사용할 폴백 다리 메시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Inventory|Preview")
	TObjectPtr<USkeletalMesh> DefaultPreviewLegsMesh;

private:
	// 주무기 프리뷰 액터
	UPROPERTY(Transient)
	TObjectPtr<APRWeaponActor> PrimaryWeaponActor;

	// 보조무기 프리뷰 액터
	UPROPERTY(Transient)
	TObjectPtr<APRWeaponActor> SecondaryWeaponActor;
};
