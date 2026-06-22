// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (인벤토리 연동 장착 장비 실시간 비주얼 변경 구현)
// Author: 손승우 (프리뷰 상태 리셋 연동)
// Author: 이건주 (캐릭터 마우스 조작 회전 및 조명 연동 구현)
#include "PRCharacterPreviewActor.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/ItemSystem/Actors/PRWeaponActor.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "ProjectR/ItemSystem/Data/PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponModDataAsset.h"

namespace
{
	void ConfigurePreviewLightingChannel(UPrimitiveComponent* PrimitiveComponent)
	{
		if (!IsValid(PrimitiveComponent))
		{
			return;
		}

		// 프리뷰 전용 3번 라이트 채널
		PrimitiveComponent->LightingChannels.bChannel0 = false;
		PrimitiveComponent->LightingChannels.bChannel1 = false;
		PrimitiveComponent->LightingChannels.bChannel2 = true;
	}

	void ConfigurePreviewLightingChannel(ULightComponent* LightComponent)
	{
		if (!IsValid(LightComponent))
		{
			return;
		}

		// 프리뷰 전용 3번 라이트 채널
		LightComponent->LightingChannels.bChannel0 = false;
		LightComponent->LightingChannels.bChannel1 = false;
		LightComponent->LightingChannels.bChannel2 = true;
	}

	void ConfigurePreviewPartMeshComponent(USkeletalMeshComponent* PartMeshComponent, USkeletalMeshComponent* LeaderMeshComponent)
	{
		if (!IsValid(PartMeshComponent) || !IsValid(LeaderMeshComponent))
		{
			return;
		}

		// 실제 플레이어 파츠와 같은 포즈 기준
		PartMeshComponent->SetupAttachment(LeaderMeshComponent);
		PartMeshComponent->SetLeaderPoseComponent(LeaderMeshComponent);
		PartMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PartMeshComponent->SetGenerateOverlapEvents(false);
		PartMeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		PartMeshComponent->bUseAttachParentBound = true;
		ConfigurePreviewLightingChannel(PartMeshComponent);
	}

	void AddPreviewPartToCapture(USceneCaptureComponent2D* SceneCaptureComponent, USkeletalMeshComponent* PartMeshComponent)
	{
		if (!IsValid(SceneCaptureComponent) || !IsValid(PartMeshComponent))
		{
			return;
		}

		// ShowOnly 캡처 대상 파츠
		SceneCaptureComponent->ShowOnlyComponent(PartMeshComponent);
	}

	bool IsPreviewSupportedSlot(EPRWeaponSlotType SlotType)
	{
		return SlotType == EPRWeaponSlotType::Primary || SlotType == EPRWeaponSlotType::Secondary;
	}

	FName GetPreviewAttachSocketName(const UPRWeaponDataAsset* WeaponData, EPRArmedState ArmedState)
	{
		if (!IsValid(WeaponData))
		{
			return NAME_None;
		}

		if (ArmedState == EPRArmedState::Armed)
		{
			return FName(PREnumHelper::EnumToFName(WeaponData->ArmedSocketName));
		}

		return FName(PREnumHelper::EnumToFName(WeaponData->StowedSocketName));
	}

	void AttachWeaponActorToPreviewMesh(APRWeaponActor* WeaponActor, USkeletalMeshComponent* PreviewMeshComponent, const UPRWeaponDataAsset* WeaponData, EPRArmedState ArmedState)
	{
		if (!IsValid(WeaponActor) || !IsValid(PreviewMeshComponent) || !IsValid(WeaponData))
		{
			return;
		}

		const FName AttachSocketName = GetPreviewAttachSocketName(WeaponData, ArmedState);
		if (AttachSocketName.IsNone() || !PreviewMeshComponent->DoesSocketExist(AttachSocketName))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[CharacterPreviewActor] 무기 프리뷰 부착 소켓 없음 | Weapon = %s | Socket = %s"),
				*GetNameSafe(WeaponData),
				*AttachSocketName.ToString());
			return;
		}

		WeaponActor->AttachToComponent(
			PreviewMeshComponent,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocketName);
	}
}

APRCharacterPreviewActor::APRCharacterPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	PreviewCharacterClass = APRPlayerCharacter::StaticClass();

	// 카메라와 조명이 캐릭터 메시 회전에 종속되지 않도록 분리된 기준 루트
	PreviewRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewRootComponent"));
	SetRootComponent(PreviewRootComponent);

	// 숨김 리더 메시
	PreviewMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewMeshComponent"));
	PreviewMeshComponent->SetupAttachment(PreviewRootComponent);
	PreviewMeshComponent->SetVisibility(false);
	PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMeshComponent->SetGenerateOverlapEvents(false);
	PreviewMeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	ConfigurePreviewLightingChannel(PreviewMeshComponent);

	// 플레이어 캐릭터의 모듈러 파츠 구조와 같은 프리뷰 파츠
	PreviewHeadMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewHeadMeshComponent"));
	ConfigurePreviewPartMeshComponent(PreviewHeadMeshComponent, PreviewMeshComponent);

	PreviewPlayerFaceMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewPlayerFaceMeshComponent"));
	ConfigurePreviewPartMeshComponent(PreviewPlayerFaceMeshComponent, PreviewMeshComponent);

	PreviewBodyMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewBodyMeshComponent"));
	ConfigurePreviewPartMeshComponent(PreviewBodyMeshComponent, PreviewMeshComponent);

	PreviewHandsMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewHandsMeshComponent"));
	ConfigurePreviewPartMeshComponent(PreviewHandsMeshComponent, PreviewMeshComponent);

	PreviewLegsMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewLegsMeshComponent"));
	ConfigurePreviewPartMeshComponent(PreviewLegsMeshComponent, PreviewMeshComponent);

	// 씬 캡처 컴포넌트 스프링 암
	CaptureSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("CaptureSpringArmComponent"));
	CaptureSpringArmComponent->SetupAttachment(PreviewRootComponent);
	CaptureSpringArmComponent->TargetArmLength = 230.0f;
	CaptureSpringArmComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	CaptureSpringArmComponent->SetRelativeRotation(FRotator(0.0f, -7.0f, -90.0f));
	CaptureSpringArmComponent->bDoCollisionTest = false;
	CaptureSpringArmComponent->bUsePawnControlRotation = false;
	CaptureSpringArmComponent->bInheritPitch = false;
	CaptureSpringArmComponent->bInheritYaw = false;
	CaptureSpringArmComponent->bInheritRoll = false;

	// ===== 씬 캡처 컴포넌트 =====
	SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent"));
	SceneCaptureComponent->SetupAttachment(CaptureSpringArmComponent, USpringArmComponent::SocketName);
	SceneCaptureComponent->SetRelativeLocation(FVector::ZeroVector);
	SceneCaptureComponent->SetRelativeRotation(FRotator::ZeroRotator);
	SceneCaptureComponent->FOVAngle = 35.0f;
	SceneCaptureComponent->bCaptureEveryFrame = false;
	SceneCaptureComponent->bCaptureOnMovement = false;
	SceneCaptureComponent->CaptureSource = SCS_SceneColorHDR;
	SceneCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

	// 월드 환경 요소 차단과 프리뷰 전용 3번 라이트 채널 사용
	SceneCaptureComponent->ShowFlags.SetAtmosphere(false);
	SceneCaptureComponent->ShowFlags.SetFog(false);
	SceneCaptureComponent->ShowFlags.SetVolumetricFog(false);
	SceneCaptureComponent->ShowFlags.SetSkyLighting(false);

	// 그림자 — 전용 조명이므로 불필요
	SceneCaptureComponent->ShowFlags.SetDynamicShadows(false);
	SceneCaptureComponent->ShowFlags.SetContactShadows(false);
	SceneCaptureComponent->ShowFlags.SetAmbientOcclusion(false);
	SceneCaptureComponent->ShowFlags.SetDistanceFieldAO(false);

	// 격리된 씬이므로 환경 반사·간접광 불필요
	SceneCaptureComponent->ShowFlags.SetGlobalIllumination(false);
	SceneCaptureComponent->ShowFlags.SetReflectionEnvironment(false);
	SceneCaptureComponent->ShowFlags.SetScreenSpaceReflections(false);
	SceneCaptureComponent->ShowFlags.SetIndirectLightingCache(false);

	// 포스트 프로세스 — 저해상도 프리뷰에 불필요한 효과 제거
	SceneCaptureComponent->ShowFlags.SetEyeAdaptation(false);
	SceneCaptureComponent->ShowFlags.SetBloom(false);
	SceneCaptureComponent->ShowFlags.SetMotionBlur(false);
	SceneCaptureComponent->ShowFlags.SetLensFlares(false);
	SceneCaptureComponent->ShowFlags.SetToneCurve(false);

	// 에디터 시각화 요소 차단
	SceneCaptureComponent->ShowFlags.SetCameraFrustums(false);
	SceneCaptureComponent->ShowFlags.SetBillboardSprites(false);

	// ===== 라이트 스프링 암 =====
	LightSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("LightSpringArmComponent"));
	LightSpringArmComponent->SetupAttachment(PreviewRootComponent);
	LightSpringArmComponent->TargetArmLength = 320.0f;
	LightSpringArmComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	LightSpringArmComponent->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f));
	LightSpringArmComponent->bDoCollisionTest = false;
	LightSpringArmComponent->bUsePawnControlRotation = false;
	LightSpringArmComponent->bInheritPitch = false;
	LightSpringArmComponent->bInheritYaw = false;
	LightSpringArmComponent->bInheritRoll = false;

	// ===== 라이트 컴포넌트 =====
	KeyLightComponent = CreateDefaultSubobject<USpotLightComponent>(TEXT("KeyLightComponent"));
	KeyLightComponent->SetupAttachment(LightSpringArmComponent, USpringArmComponent::SocketName);
	KeyLightComponent->SetRelativeLocation(FVector::ZeroVector);
	KeyLightComponent->SetRelativeRotation(FRotator::ZeroRotator);
	KeyLightComponent->SetIntensity(16000.0f);
	KeyLightComponent->SetAttenuationRadius(650.0f);
	KeyLightComponent->SetOuterConeAngle(55.0f);
	ConfigurePreviewLightingChannel(KeyLightComponent);

	// ===== 전용 방향광 컴포넌트 =====
	DirectionalLightComponent = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("DirectionalLightComponent"));
	DirectionalLightComponent->SetupAttachment(PreviewRootComponent);
	DirectionalLightComponent->SetRelativeRotation(FRotator(-35.0f, -45.0f, 0.0f));
	DirectionalLightComponent->SetIntensity(4.0f);
	DirectionalLightComponent->bAffectsWorld = true;
	ConfigurePreviewLightingChannel(DirectionalLightComponent);
}

void APRCharacterPreviewActor::SetRenderTargetToSceneCapture(UTextureRenderTarget2D* InRenderTarget)
{
	if (!IsValid(SceneCaptureComponent))
	{
		return;
	}

	// 씬 캡쳐 컴포넌트의 텍스처 타겟 설정
	SceneCaptureComponent->TextureTarget = InRenderTarget;
}

void APRCharacterPreviewActor::RefreshPreviewActorFromPlayer(APRPlayerCharacter* SourceCharacter, UPRWeaponManagerComponent* WeaponManagerComponent)
{
	RefreshCharacterMesh(SourceCharacter);
	RefreshWeaponPreview(WeaponManagerComponent);
	CapturePreview();
}

void APRCharacterPreviewActor::RefreshPreviewActorFromSaveData(const FPRCharacterSaveData& SaveData)
{
	RefreshCharacterMeshFromSaveData(SaveData);
	RefreshWeaponPreviewFromSaveData(SaveData);
	CapturePreview();
}

void APRCharacterPreviewActor::SetContinuousCapture(bool bEnabled)
{
	if (!IsValid(SceneCaptureComponent))
	{
		return;
	}

	SceneCaptureComponent->bCaptureEveryFrame = bEnabled;
	SceneCaptureComponent->bCaptureOnMovement = bEnabled;
}

void APRCharacterPreviewActor::CapturePreview()
{
	if (!IsValid(SceneCaptureComponent))
	{
		return;
	}

	if (!IsValid(SceneCaptureComponent->TextureTarget))
	{
		return;
	}

	SceneCaptureComponent->ClearShowOnlyComponents();

	// 실제 표시 대상은 숨김 리더가 아닌 모듈러 파츠 메시
	AddPreviewPartToCapture(SceneCaptureComponent, PreviewHeadMeshComponent);
	AddPreviewPartToCapture(SceneCaptureComponent, PreviewPlayerFaceMeshComponent);
	AddPreviewPartToCapture(SceneCaptureComponent, PreviewBodyMeshComponent);
	AddPreviewPartToCapture(SceneCaptureComponent, PreviewHandsMeshComponent);
	AddPreviewPartToCapture(SceneCaptureComponent, PreviewLegsMeshComponent);

	if (APRWeaponActor* PrimaryWeapon = GetWeaponActorBySlot(EPRWeaponSlotType::Primary))
	{
		if (USkeletalMeshComponent* WeaponMeshComponent = PrimaryWeapon->GetWeaponMeshComponent())
		{
			SceneCaptureComponent->ShowOnlyComponent(WeaponMeshComponent);
		}
	}

	if (APRWeaponActor* SecondaryWeapon = GetWeaponActorBySlot(EPRWeaponSlotType::Secondary))
	{
		if (USkeletalMeshComponent* WeaponMeshComponent = SecondaryWeapon->GetWeaponMeshComponent())
		{
			SceneCaptureComponent->ShowOnlyComponent(WeaponMeshComponent);
		}
	}

	SceneCaptureComponent->CaptureScene();
}

void APRCharacterPreviewActor::InitTextureStreaming(float StreamingDuration, float StreamingDistanceMultiplier)
{
	InitMeshTextureStreaming(PreviewMeshComponent, StreamingDuration, StreamingDistanceMultiplier);
	InitMeshTextureStreaming(PreviewHeadMeshComponent, StreamingDuration, StreamingDistanceMultiplier);
	InitMeshTextureStreaming(PreviewPlayerFaceMeshComponent, StreamingDuration, StreamingDistanceMultiplier);
	InitMeshTextureStreaming(PreviewBodyMeshComponent, StreamingDuration, StreamingDistanceMultiplier);
	InitMeshTextureStreaming(PreviewHandsMeshComponent, StreamingDuration, StreamingDistanceMultiplier);
	InitMeshTextureStreaming(PreviewLegsMeshComponent, StreamingDuration, StreamingDistanceMultiplier);

	if (APRWeaponActor* PrimaryWeapon = GetWeaponActorBySlot(EPRWeaponSlotType::Primary))
	{
		InitMeshTextureStreaming(PrimaryWeapon->GetWeaponMeshComponent(), StreamingDuration, StreamingDistanceMultiplier);
	}

	if (APRWeaponActor* SecondaryWeapon = GetWeaponActorBySlot(EPRWeaponSlotType::Secondary))
	{
		InitMeshTextureStreaming(SecondaryWeapon->GetWeaponMeshComponent(), StreamingDuration, StreamingDistanceMultiplier);
	}
}

/*~ AActor Interface ~*/

void APRCharacterPreviewActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyWeaponActorForSlot(EPRWeaponSlotType::Primary);
	DestroyWeaponActorForSlot(EPRWeaponSlotType::Secondary);

	Super::EndPlay(EndPlayReason);
}

void APRCharacterPreviewActor::RefreshCharacterMesh(APRPlayerCharacter* SourceCharacter)
{
	if (!IsValid(PreviewMeshComponent))
	{
		return;
	}

	if (!IsValid(SourceCharacter) || !IsValid(SourceCharacter->GetMesh()))
	{
		PreviewMeshComponent->SetSkeletalMesh(nullptr);
		RefreshCharacterPartMesh(PreviewHeadMeshComponent, nullptr);
		RefreshCharacterPartMesh(PreviewPlayerFaceMeshComponent, nullptr);
		RefreshCharacterPartMesh(PreviewBodyMeshComponent, nullptr);
		RefreshCharacterPartMesh(PreviewHandsMeshComponent, nullptr);
		RefreshCharacterPartMesh(PreviewLegsMeshComponent, nullptr);
		return;
	}

	// 원본 캐릭터의 숨김 리더 메시 복사
	USkeletalMeshComponent* SourceMeshComponent = SourceCharacter->GetMesh();
	PreviewMeshComponent->SetSkeletalMesh(SourceMeshComponent->GetSkeletalMeshAsset());

	// 장비 장착으로 교체된 실제 표시 파츠 복사
	RefreshCharacterPartMesh(PreviewHeadMeshComponent, SourceCharacter->Mesh_Head);
	RefreshCharacterPartMesh(PreviewPlayerFaceMeshComponent, SourceCharacter->Mesh_PlayerFace);
	RefreshCharacterPartMesh(PreviewBodyMeshComponent, SourceCharacter->Mesh_Body);
	RefreshCharacterPartMesh(PreviewHandsMeshComponent, SourceCharacter->Mesh_Hands);
	RefreshCharacterPartMesh(PreviewLegsMeshComponent, SourceCharacter->Mesh_Legs);

	PreviewMeshComponent->UpdateBounds();
}

void APRCharacterPreviewActor::RefreshCharacterPartMesh(USkeletalMeshComponent* PreviewPartMeshComponent, USkeletalMeshComponent* SourcePartMeshComponent)
{
	if (!IsValid(PreviewPartMeshComponent))
	{
		return;
	}

	if (!IsValid(SourcePartMeshComponent))
	{
		PreviewPartMeshComponent->SetSkeletalMesh(nullptr);
		PreviewPartMeshComponent->SetVisibility(false, true);
		PreviewPartMeshComponent->MarkRenderStateDirty();
		PreviewPartMeshComponent->UpdateBounds();
		return;
	}

	// 플레이어 캐릭터 파츠 컴포넌트에 이미 적용된 최종 메시 복사
	PreviewPartMeshComponent->SetSkeletalMesh(SourcePartMeshComponent->GetSkeletalMeshAsset());
	PreviewPartMeshComponent->SetVisibility(SourcePartMeshComponent->IsVisible(), true);
	PreviewPartMeshComponent->MarkRenderStateDirty();
	PreviewPartMeshComponent->UpdateBounds();
}

void APRCharacterPreviewActor::RefreshWeaponPreview(UPRWeaponManagerComponent* WeaponManagerComponent)
{
	// 무기 매니저 컴포넌트가 없다면
	if (!IsValid(WeaponManagerComponent))
	{
		// 프리뷰 무기 디스트로이
		DestroyWeaponActorForSlot(EPRWeaponSlotType::Primary);
		DestroyWeaponActorForSlot(EPRWeaponSlotType::Secondary);
		return;
	}

	const FPRWeaponVisualInfo& PrimaryVisualInfo = WeaponManagerComponent->GetVisualInfoBySlotType(EPRWeaponSlotType::Primary);
	const FPRWeaponVisualInfo& SecondaryVisualInfo = WeaponManagerComponent->GetVisualInfoBySlotType(EPRWeaponSlotType::Secondary);

	// 프리뷰 무기 갱신
	RefreshWeaponActorForSlot(EPRWeaponSlotType::Primary, PrimaryVisualInfo);
	RefreshWeaponActorForSlot(EPRWeaponSlotType::Secondary, SecondaryVisualInfo);
}

void APRCharacterPreviewActor::RefreshWeaponPreviewFromSaveData(const FPRCharacterSaveData& SaveData)
{
	// 저장 데이터 기반 주무기와 보조무기 공개 비주얼 정보
	const FPRWeaponVisualInfo PrimaryVisualInfo = ResolveWeaponVisualInfoFromSaveData(SaveData, EPRWeaponSlotType::Primary);
	const FPRWeaponVisualInfo SecondaryVisualInfo = ResolveWeaponVisualInfoFromSaveData(SaveData, EPRWeaponSlotType::Secondary);

	RefreshWeaponActorForSlot(EPRWeaponSlotType::Primary, PrimaryVisualInfo);
	RefreshWeaponActorForSlot(EPRWeaponSlotType::Secondary, SecondaryVisualInfo);
}

void APRCharacterPreviewActor::RefreshCharacterMeshFromSaveData(const FPRCharacterSaveData& SaveData)
{
	if (!IsValid(PreviewMeshComponent))
	{
		return;
	}

	ApplyDefaultPreviewCharacterMesh();

	// 저장 데이터의 장비 메시가 있으면 기본 파츠 메시를 대체
	if (IsValid(PreviewHeadMeshComponent))
	{
		const UPREquipmentDataAsset* HeadEquipmentData = ResolveEquipmentDataFromSaveData(SaveData, EPREquipmentSlotType::Head);
		if (USkeletalMesh* HeadMesh = IsValid(HeadEquipmentData) ? HeadEquipmentData->GetEquipmentMesh().LoadSynchronous() : nullptr)
		{
			PreviewHeadMeshComponent->SetSkeletalMesh(HeadMesh);
		}

		UpdatePreviewPlayerFaceVisibility(HeadEquipmentData);
	}

	if (IsValid(PreviewBodyMeshComponent))
	{
		if (USkeletalMesh* BodyMesh = ResolveEquipmentMeshFromSaveData(SaveData, EPREquipmentSlotType::Body))
		{
			PreviewBodyMeshComponent->SetSkeletalMesh(BodyMesh);
		}
	}

	if (IsValid(PreviewHandsMeshComponent))
	{
		if (USkeletalMesh* HandsMesh = ResolveEquipmentMeshFromSaveData(SaveData, EPREquipmentSlotType::Hands))
		{
			PreviewHandsMeshComponent->SetSkeletalMesh(HandsMesh);
		}
	}

	if (IsValid(PreviewLegsMeshComponent))
	{
		if (USkeletalMesh* LegsMesh = ResolveEquipmentMeshFromSaveData(SaveData, EPREquipmentSlotType::Legs))
		{
			PreviewLegsMeshComponent->SetSkeletalMesh(LegsMesh);
		}
	}

	PreviewMeshComponent->UpdateBounds();
}

void APRCharacterPreviewActor::UpdatePreviewPlayerFaceVisibility(const UPREquipmentDataAsset* HeadEquipmentData)
{
	if (!IsValid(PreviewPlayerFaceMeshComponent))
	{
		return;
	}

	// 얼굴 전체를 덮는 머리 장비만 프리뷰 얼굴 파츠 숨김
	const bool bShouldShowPlayerFace = !IsValid(HeadEquipmentData) || !HeadEquipmentData->ShouldHidePlayerFace();
	PreviewPlayerFaceMeshComponent->SetVisibility(bShouldShowPlayerFace, true);
}

const APRPlayerCharacter* APRCharacterPreviewActor::GetPreviewCharacterDefaultObject() const
{
	const TSubclassOf<APRPlayerCharacter> CharacterClass = PreviewCharacterClass;
	if (!IsValid(CharacterClass.Get()))
	{
		return nullptr;
	}

	return CharacterClass->GetDefaultObject<APRPlayerCharacter>();
}

void APRCharacterPreviewActor::ApplyDefaultPreviewCharacterMesh()
{
	const APRPlayerCharacter* DefaultCharacter = GetPreviewCharacterDefaultObject();
	const USkeletalMeshComponent* SourceLeaderMeshComponent = IsValid(DefaultCharacter)
		? DefaultCharacter->GetMesh()
		: nullptr;
	USkeletalMesh* LeaderMesh = IsValid(SourceLeaderMeshComponent)
		? SourceLeaderMeshComponent->GetSkeletalMeshAsset()
		: nullptr;
	if (!IsValid(LeaderMesh))
	{
		LeaderMesh = DefaultPreviewMesh.Get();
	}

	if (!IsValid(LeaderMesh))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CharacterPreviewActor] 저장 데이터 프리뷰 기본 리더 메시 설정 없음"));
	}

	// 플레이어 캐릭터 기본 오브젝트 기반 리더 메시 적용
	PreviewMeshComponent->SetSkeletalMesh(LeaderMesh);

	ApplyDefaultPreviewPartMesh(
		PreviewHeadMeshComponent,
		DefaultCharacter,
		IsValid(DefaultCharacter) ? DefaultCharacter->Mesh_Head.Get() : nullptr,
		EPREquipmentSlotType::Head,
		DefaultPreviewHeadMesh.Get());
	ApplyDefaultPreviewPartMesh(
		PreviewPlayerFaceMeshComponent,
		DefaultCharacter,
		IsValid(DefaultCharacter) ? DefaultCharacter->Mesh_PlayerFace.Get() : nullptr,
		EPREquipmentSlotType::None,
		DefaultPreviewPlayerFaceMesh.Get());
	ApplyDefaultPreviewPartMesh(
		PreviewBodyMeshComponent,
		DefaultCharacter,
		IsValid(DefaultCharacter) ? DefaultCharacter->Mesh_Body.Get() : nullptr,
		EPREquipmentSlotType::Body,
		DefaultPreviewBodyMesh.Get());
	ApplyDefaultPreviewPartMesh(
		PreviewHandsMeshComponent,
		DefaultCharacter,
		IsValid(DefaultCharacter) ? DefaultCharacter->Mesh_Hands.Get() : nullptr,
		EPREquipmentSlotType::Hands,
		DefaultPreviewHandsMesh.Get());
	ApplyDefaultPreviewPartMesh(
		PreviewLegsMeshComponent,
		DefaultCharacter,
		IsValid(DefaultCharacter) ? DefaultCharacter->Mesh_Legs.Get() : nullptr,
		EPREquipmentSlotType::Legs,
		DefaultPreviewLegsMesh.Get());

	const bool bHasAnyPartMesh =
		(IsValid(PreviewHeadMeshComponent) && IsValid(PreviewHeadMeshComponent->GetSkeletalMeshAsset()))
		|| (IsValid(PreviewPlayerFaceMeshComponent) && IsValid(PreviewPlayerFaceMeshComponent->GetSkeletalMeshAsset()))
		|| (IsValid(PreviewBodyMeshComponent) && IsValid(PreviewBodyMeshComponent->GetSkeletalMeshAsset()))
		|| (IsValid(PreviewHandsMeshComponent) && IsValid(PreviewHandsMeshComponent->GetSkeletalMeshAsset()))
		|| (IsValid(PreviewLegsMeshComponent) && IsValid(PreviewLegsMeshComponent->GetSkeletalMeshAsset()));
	if (!bHasAnyPartMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CharacterPreviewActor] 저장 데이터 프리뷰 기본 파츠 메시 설정 없음"));
	}
}

void APRCharacterPreviewActor::ApplyDefaultPreviewPartMesh(USkeletalMeshComponent* PreviewPartMeshComponent, const APRPlayerCharacter* DefaultCharacter, const USkeletalMeshComponent* SourcePartMeshComponent, EPREquipmentSlotType SlotType, USkeletalMesh* FallbackMesh) const
{
	if (!IsValid(PreviewPartMeshComponent))
	{
		return;
	}

	USkeletalMesh* PartMesh = IsValid(DefaultCharacter)
		? DefaultCharacter->GetDefaultEquipmentMesh(SlotType)
		: nullptr;
	if (!IsValid(PartMesh) && IsValid(SourcePartMeshComponent))
	{
		PartMesh = SourcePartMeshComponent->GetSkeletalMeshAsset();
	}

	if (!IsValid(PartMesh))
	{
		PartMesh = FallbackMesh;
	}

	// 플레이어 캐릭터 기본 장비 메시 기반 파츠 메시 적용
	PreviewPartMeshComponent->SetSkeletalMesh(PartMesh);
	PreviewPartMeshComponent->SetVisibility(true, true);
	PreviewPartMeshComponent->UpdateBounds();
}

USkeletalMesh* APRCharacterPreviewActor::ResolveEquipmentMeshFromSaveData(const FPRCharacterSaveData& SaveData, EPREquipmentSlotType SlotType) const
{
	const UPREquipmentDataAsset* EquipmentData = ResolveEquipmentDataFromSaveData(SaveData, SlotType);
	return IsValid(EquipmentData) ? EquipmentData->GetEquipmentMesh().LoadSynchronous() : nullptr;
}

const UPREquipmentDataAsset* APRCharacterPreviewActor::ResolveEquipmentDataFromSaveData(const FPRCharacterSaveData& SaveData, EPREquipmentSlotType SlotType) const
{
	for (const FPREquipmentSlotSaveEntry& EquippedSlot : SaveData.Equipment.EquippedSlots)
	{
		if (EquippedSlot.SlotType != SlotType)
		{
			continue;
		}

		if (!SaveData.Inventory.Equipments.IsValidIndex(EquippedSlot.EquipmentItemIndex))
		{
			return nullptr;
		}

		// 장비 저장 인덱스 기반 데이터 조회
		const FPREquipmentItemSaveEntry& EquipmentEntry = SaveData.Inventory.Equipments[EquippedSlot.EquipmentItemIndex];
		return EquipmentEntry.EquipmentData.LoadSynchronous();
	}

	return nullptr;
}

FPRWeaponVisualInfo APRCharacterPreviewActor::ResolveWeaponVisualInfoFromSaveData(const FPRCharacterSaveData& SaveData, EPRWeaponSlotType SlotType) const
{
	FPRWeaponVisualInfo VisualInfo;
	VisualInfo.SlotType = SlotType;

	int32 WeaponIndex = INDEX_NONE;
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		WeaponIndex = SaveData.WeaponManager.PrimaryWeaponIndex;
	}
	else if (SlotType == EPRWeaponSlotType::Secondary)
	{
		WeaponIndex = SaveData.WeaponManager.SecondaryWeaponIndex;
	}

	if (!SaveData.Inventory.Weapons.IsValidIndex(WeaponIndex))
	{
		VisualInfo.Reset(SlotType);
		return VisualInfo;
	}

	// 무기 저장 인덱스 기반 공개 비주얼 정보 구성
	const FPRWeaponItemSaveEntry& WeaponEntry = SaveData.Inventory.Weapons[WeaponIndex];
	VisualInfo.WeaponData = WeaponEntry.WeaponData.LoadSynchronous();

	if (SaveData.Inventory.Mods.IsValidIndex(WeaponEntry.EquippedModIndex))
	{
		const FPRModItemSaveEntry& ModEntry = SaveData.Inventory.Mods[WeaponEntry.EquippedModIndex];
		VisualInfo.ModData = ModEntry.ModData.LoadSynchronous();
	}

	return VisualInfo;
}

void APRCharacterPreviewActor::RefreshWeaponActorForSlot(EPRWeaponSlotType SlotType, const FPRWeaponVisualInfo& VisualInfo)
{
	if (!IsPreviewSupportedSlot(SlotType))
	{
		return;
	}

	if (VisualInfo.IsEmpty() || !IsValid(VisualInfo.WeaponData) || !IsValid(VisualInfo.WeaponData->WeaponActorClass))
	{
		DestroyWeaponActorForSlot(SlotType);
		return;
	}
	
	APRWeaponActor* WeaponActor = GetWeaponActorBySlot(SlotType);
	// 액터 유효성에 따라 재스폰 여부 결정
	const bool bNeedsRespawn = !IsValid(WeaponActor)
		|| WeaponActor->GetClass() != VisualInfo.WeaponData->WeaponActorClass;

	if (bNeedsRespawn)
	{
		//기존 액터 파괴
		DestroyWeaponActorForSlot(SlotType);
		
		if (!IsValid(GetWorld()))
		{
			return;
		}
		
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		// 무기 액터 스폰 
		WeaponActor = GetWorld()->SpawnActor<APRWeaponActor>(
			VisualInfo.WeaponData->WeaponActorClass,
			GetActorTransform(),
			SpawnParameters);

		// 무기 액터 캐싱
		SetWeaponActorBySlot(SlotType, WeaponActor);
	}

	if (!IsValid(WeaponActor))
	{
		return;
	}

	// 무기액터 콜리전, 히든 끄기
	WeaponActor->SetActorEnableCollision(false);
	WeaponActor->SetActorHiddenInGame(false);
	if (USkeletalMeshComponent* WeaponMeshComponent = WeaponActor->GetWeaponMeshComponent())
	{
		ConfigurePreviewLightingChannel(WeaponMeshComponent);
	}
	AttachWeaponActorToPreviewMesh(WeaponActor, PreviewMeshComponent, VisualInfo.WeaponData, EPRArmedState::Unarmed);
}

void APRCharacterPreviewActor::DestroyWeaponActorForSlot(EPRWeaponSlotType SlotType)
{
	APRWeaponActor* WeaponActor = GetWeaponActorBySlot(SlotType);
	// 무기 액터가 유효하면
	if (IsValid(WeaponActor))
	{
		// 무기 액터 파괴
		WeaponActor->Destroy();
	}

	// 캐시된 포인터 정리
	ClearWeaponActorBySlot(SlotType);
}

APRWeaponActor* APRCharacterPreviewActor::GetWeaponActorBySlot(EPRWeaponSlotType SlotType) const
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		return PrimaryWeaponActor;
	}

	if (SlotType == EPRWeaponSlotType::Secondary)
	{
		return SecondaryWeaponActor;
	}

	return nullptr;
}

void APRCharacterPreviewActor::SetWeaponActorBySlot(EPRWeaponSlotType SlotType, APRWeaponActor* NewWeaponActor)
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		PrimaryWeaponActor = NewWeaponActor;
	}
	else if (SlotType == EPRWeaponSlotType::Secondary)
	{
		SecondaryWeaponActor = NewWeaponActor;
	}
}

void APRCharacterPreviewActor::ClearWeaponActorBySlot(EPRWeaponSlotType SlotType)
{
	if (SlotType == EPRWeaponSlotType::Primary)
	{
		PrimaryWeaponActor = nullptr;
	}
	else if (SlotType == EPRWeaponSlotType::Secondary)
	{
		SecondaryWeaponActor = nullptr;
	}
}

void APRCharacterPreviewActor::InitMeshTextureStreaming(USkeletalMeshComponent* MeshComponent, float StreamingDuration, float StreamingDistanceMultiplier) const
{
	if (!IsValid(MeshComponent))
	{
		return;
	}

	// SceneCapture 전용 프리뷰의 높은 밉 요청 보정
	MeshComponent->StreamingDistanceMultiplier = StreamingDistanceMultiplier;
	MeshComponent->PrestreamTextures(StreamingDuration, true);
}
