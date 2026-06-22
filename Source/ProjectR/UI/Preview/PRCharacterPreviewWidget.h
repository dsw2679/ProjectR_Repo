// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (프리뷰 조명 제어 및 탭 전환 연동 구현)
// Author: 이건주 (캐릭터 회전 조작 감도 설정 연동 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "PRCharacterPreviewWidget.generated.h"

class APRCharacterPreviewActor;
class APRPlayerCharacter;
class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPRWeaponManagerComponent;
class UTextureRenderTarget2D;
class UWidget;
struct FPRAssetLoadResult;

// UI에 캐릭터 3D 프리뷰 이미지를 표시하는 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRCharacterPreviewWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRCharacterPreviewWidget();
	
	// 프리뷰에 사용할 플레이어 캐릭터와 무기 상태 소스 설정
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Preview")
	void SetPreviewSources(APRPlayerCharacter* InSourceCharacter, UPRWeaponManagerComponent* InWeaponManagerComponent);

	// 프리뷰에 사용할 저장 데이터 소스 설정
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Preview")
	void SetPreviewSaveData(const FPRCharacterSaveData& InSaveData);

	// 현재 소스 상태를 기준으로 프리뷰 갱신
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory|Preview")
	void RefreshPreview();

	// 현재 사용 중인 프리뷰 액터 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory|Preview")
	APRCharacterPreviewActor* GetPreviewActor() const { return PreviewActor; }

protected:
	/*~ UUserWidget Interface ~*/
	// 화면에 표시될 때 프리뷰 액터와 이미지 준비
	virtual void NativeConstruct() override;

	// 화면에서 제거될 때 프리뷰 액터 참조 정리
	virtual void NativeDestruct() override;

private:
	// 프리뷰 로딩 파이프라인 시작
	void StartPreviewLoadPipeline();

	// 프리뷰 의존 에셋 로드 완료 처리
	void HandlePreviewDependentAssetsLoaded(const FPRAssetLoadResult& Result, uint64 PreviewRequestSerial);

	// 로드된 에셋 기준 프리뷰 즉시 적용
	void ApplyLoadedPreview();

	// 프리뷰 로딩 표시
	void ShowPreviewLoading();

	// 프리뷰 준비 완료 표시
	void ShowPreviewReady();

	// 프리뷰 액터 생성 또는 기존 액터 재사용
	void EnsurePreviewActor();

	// 프리뷰 액터 정리
	void DestroyPreviewActor();

	// 렌더 타겟을 이미지 위젯에 연결
	void RefreshPreviewBrush();

	// 소스가 비어 있으면 Owning Player 기준으로 프리뷰 소스 조회
	void SyncWithPreviewSources();

protected:
	// UMG에서 바인딩할 캐릭터 프리뷰 이미지
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Preview")
	TObjectPtr<UImage> CharacterPreviewImage;

	// UMG에서 바인딩할 프리뷰 로딩 표시
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "ProjectR|Preview")
	TObjectPtr<UWidget> PreviewLoadingThrobber;

	// 프리뷰에 사용할 액터 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preview")
	TSubclassOf<APRCharacterPreviewActor> PreviewActorClass;

	// 프리뷰 캡처 결과를 받을 렌더 타겟
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preview")
	TObjectPtr<UTextureRenderTarget2D> PreviewRenderTarget;

	// 프리뷰 렌더 타겟을 표시할 UI 머티리얼
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preview")
	TObjectPtr<UMaterialInterface> PreviewMaterial;

	// 프리뷰 메시 텍스처 스트리밍 거리 보정값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preview", meta = (ClampMin = "0.0"))
	float PreviewStreamingDistanceMultiplier = 5.0f;

	// 프리뷰 텍스처 프리스트림 유지 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Preview", meta = (ClampMin = "0.0"))
	float PreviewTexturePrestreamSeconds = 1.0f;

private:
	// 프리뷰 메시와 캡처를 담당하는 액터
	UPROPERTY(Transient)
	TObjectPtr<APRCharacterPreviewActor> PreviewActor;

	// 프리뷰 기준이 되는 실제 플레이어 캐릭터
	UPROPERTY(Transient)
	TObjectPtr<APRPlayerCharacter> SourceCharacter;

	// 장착 무기 상태를 읽을 무기 매니저
	UPROPERTY(Transient)
	TObjectPtr<UPRWeaponManagerComponent> WeaponManagerComponent;

	// 동적으로 복제된 고유 렌더 타겟 (PIE 환경에서 여러 클라가 에셋을 공유하는 문제 방지)
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> DynamicRenderTarget;

	// 동적으로 생성된 UI 머티리얼 인스턴스
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	// 저장 데이터 기반 프리뷰 사용 여부
	bool bUseSaveDataPreview = false;

	// 저장 데이터 기반 프리뷰 입력값
	FPRCharacterSaveData PreviewSaveData;

	// 최신 프리뷰 로드 요청 식별자
	uint64 PreviewLoadRequestSerial = 0;
};
