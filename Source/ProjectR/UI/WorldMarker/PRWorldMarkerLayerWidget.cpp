// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (World 핑/마커 Layer UI 위젯 구현)
#include "PRWorldMarkerLayerWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "PRPingMarkerTargetInterface.h"
#include "PRWorldMarkerWidget.h"

// ===== UUserWidget =====

void UPRWorldMarkerLayerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// HUD 입력 관통
	SetVisibility(ESlateVisibility::HitTestInvisible);

	// 이벤트 버스 조회
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPREventManagerSubsystem* EventManager = World->GetSubsystem<UPREventManagerSubsystem>();
	if (!IsValid(EventManager))
	{
		return;
	}

	// 마커 이벤트 구독
	WorldMarkerEventHandle = EventManager->Listen(
		PRGameplayTags::Event_Player_WorldMarker,
		FPREventMulticast::FDelegate::CreateUObject(this, &ThisClass::HandleWorldMarkerEvent));
}

void UPRWorldMarkerLayerWidget::NativeDestruct()
{
	// 이벤트 구독 해제
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		if (UPREventManagerSubsystem* EventManager = World->GetSubsystem<UPREventManagerSubsystem>())
		{
			EventManager->UnlistenAll(WorldMarkerEventHandle);
		}
	}

	ClearMarkers();
	Super::NativeDestruct();
}

void UPRWorldMarkerLayerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RetireExpiredMarkers();

	// 거리 기준 폰 조회
	APlayerController* OwningPlayerController = GetOwningPlayer();
	APawn* OwningPawn = IsValid(OwningPlayerController) ? OwningPlayerController->GetPawn() : nullptr;
	if (!IsValid(OwningPlayerController) || !IsValid(OwningPawn))
	{
		return;
	}

	// 레이어 크기 검증
	const FVector2D LayerSize = MyGeometry.GetLocalSize();
	if (LayerSize.X <= KINDA_SMALL_NUMBER || LayerSize.Y <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	for (const TPair<FGuid, TObjectPtr<UPRWorldMarkerWidget>>& Pair : ActiveMarkerWidgets)
	{
		// 마커 인스턴스 검증
		UPRWorldMarkerWidget* MarkerWidget = Pair.Value;
		if (!IsValid(MarkerWidget))
		{
			continue;
		}

		FVector WorldLocation = FVector::ZeroVector;
		FPRWorldMarkerVisualData VisualData;
		bool bMarkerVisible = true;
		// 추적 대상 위치 갱신
		if (!ResolveMarkerWorldLocation(MarkerWidget->GetMarkerData(), WorldLocation, VisualData, bMarkerVisible))
		{
			continue;
		}

		if (!bMarkerVisible)
		{
			// 인터페이스 숨김
			MarkerWidget->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		// 표시 복구
		MarkerWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

		FVector2D ScreenPosition = FVector2D::ZeroVector;
		bool bOnScreen = false;
		float ArrowAngle = 0.0f;
		// 화면 좌표 변환
		if (!ProjectMarkerToLayer(WorldLocation, LayerSize, ScreenPosition, bOnScreen, ArrowAngle))
		{
			continue;
		}

		const float DistanceMeters = FVector::Dist(OwningPawn->GetActorLocation(), WorldLocation) * 0.01f;
		MarkerWidget->ApplyVisualData(VisualData);
		MarkerWidget->RefreshMarker(ScreenPosition, DistanceMeters, bOnScreen, ArrowAngle);
	}
}

// ===== 마커 관리 =====

void UPRWorldMarkerLayerWidget::AddOrUpdateMarker(const FPRWorldMarkerData& MarkerData)
{
	// 위젯 생성 조건
	if (!IsValid(MarkerCanvas) || !IsValid(MarkerWidgetClass.Get()) || !MarkerData.MarkerId.IsValid())
	{
		return;
	}

	UPRWorldMarkerWidget* MarkerWidget = ActiveMarkerWidgets.FindRef(MarkerData.MarkerId);
	if (!IsValid(MarkerWidget))
	{
		// 신규 마커 생성
		APlayerController* OwningPlayerController = GetOwningPlayer();
		if (!IsValid(OwningPlayerController))
		{
			return;
		}

		MarkerWidget = CreateWidget<UPRWorldMarkerWidget>(OwningPlayerController, MarkerWidgetClass);
		if (!IsValid(MarkerWidget))
		{
			return;
		}

		UCanvasPanelSlot* CanvasSlot = MarkerCanvas->AddChildToCanvas(MarkerWidget);
		if (IsValid(CanvasSlot))
		{
			// 중심 정렬
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		}

		ActiveMarkerWidgets.Add(MarkerData.MarkerId, MarkerWidget);
	}

	MarkerWidget->InitializeMarker(MarkerData);
}

void UPRWorldMarkerLayerWidget::RemoveMarker(FGuid MarkerId)
{
	// 위젯 제거
	UPRWorldMarkerWidget* MarkerWidget = ActiveMarkerWidgets.FindRef(MarkerId);
	if (IsValid(MarkerWidget))
	{
		MarkerWidget->RemoveFromParent();
	}

	ActiveMarkerWidgets.Remove(MarkerId);
}

void UPRWorldMarkerLayerWidget::ClearMarkers()
{
	// 전체 마커 제거
	for (TPair<FGuid, TObjectPtr<UPRWorldMarkerWidget>>& Pair : ActiveMarkerWidgets)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->RemoveFromParent();
		}
	}

	ActiveMarkerWidgets.Empty();
}

// ===== 이벤트 =====

void UPRWorldMarkerLayerWidget::HandleWorldMarkerEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	// 페이로드 타입 확인
	const FPRWorldMarkerEventPayload* MarkerPayload = Payload.GetPtr<FPRWorldMarkerEventPayload>();
	if (MarkerPayload == nullptr)
	{
		return;
	}

	if (MarkerPayload->EventType == EPRWorldMarkerEventType::Removed)
	{
		// 제거 이벤트 처리
		RemoveMarker(MarkerPayload->MarkerId);
		return;
	}

	// 추가 이벤트 처리
	AddOrUpdateMarker(MarkerPayload->MarkerData);
}

// ===== 위치 해석 =====

bool UPRWorldMarkerLayerWidget::ResolveMarkerWorldLocation(const FPRWorldMarkerData& MarkerData, FVector& OutWorldLocation, FPRWorldMarkerVisualData& OutVisualData, bool& bOutVisible) const
{
	// 고정 위치 기본값
	OutWorldLocation = MarkerData.WorldLocation;
	OutVisualData = MarkerData.VisualData;
	bOutVisible = true;

	if (MarkerData.LocationMode != EPRWorldMarkerLocationMode::InterfaceTarget)
	{
		return true;
	}

	AActor* TargetActor = MarkerData.TargetActor;
	if (!IsValid(TargetActor) || !TargetActor->GetClass()->ImplementsInterface(UPRPingMarkerTargetInterface::StaticClass()))
	{
		// 추적 대상 유실 시 폴백 위치
		return true;
	}

	// 인터페이스 최신값
	OutWorldLocation = IPRPingMarkerTargetInterface::Execute_GetPingMarkerWorldLocation(TargetActor);
	OutVisualData = IPRPingMarkerTargetInterface::Execute_GetPingMarkerVisualData(TargetActor);
	bOutVisible = IPRPingMarkerTargetInterface::Execute_ShouldPingMarkerVisible(TargetActor);
	return true;
}

// ===== 투영 =====

bool UPRWorldMarkerLayerWidget::ProjectMarkerToLayer(
	const FVector& WorldLocation,
	const FVector2D& LayerSize,
	FVector2D& OutScreenPosition,
	bool& bOutOnScreen,
	float& OutArrowAngle) const
{
	// 플레이어 시점 조회
	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (!IsValid(OwningPlayerController))
	{
		return false;
	}

	// 월드 좌표 투영
	FVector2D ProjectedPosition = FVector2D::ZeroVector;
	const bool bProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		OwningPlayerController,
		WorldLocation,
		ProjectedPosition,
		true);

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	OwningPlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	// 카메라 후방 판정
	const FVector ToMarker = WorldLocation - ViewLocation;
	const FVector LocalDirection = ViewRotation.UnrotateVector(ToMarker.GetSafeNormal());
	const bool bBehindCamera = LocalDirection.X < 0.0f;
	const FVector2D LayerCenter = LayerSize * 0.5f;
	const FVector2D MinBounds(EdgePadding, EdgePadding);
	const FVector2D MaxBounds(
		FMath::Max(EdgePadding, LayerSize.X - EdgePadding),
		FMath::Max(EdgePadding, LayerSize.Y - EdgePadding));

	if (!bProjected || bBehindCamera)
	{
		// 후방 대상 방향 보정
		FVector2D Direction(LocalDirection.Y, -LocalDirection.Z);
		if (bBehindCamera)
		{
			// 후방 마커 하단 고정
			Direction = FVector2D(LocalDirection.Y, 1.0f);
		}

		if (Direction.IsNearlyZero())
		{
			Direction = FVector2D(0.0f, 1.0f);
		}

		ProjectedPosition = LayerCenter + Direction.GetSafeNormal() * LayerSize.Size();
	}

	// 화면 안전 영역 판정
	const bool bInsideBounds =
		ProjectedPosition.X >= MinBounds.X &&
		ProjectedPosition.X <= MaxBounds.X &&
		ProjectedPosition.Y >= MinBounds.Y &&
		ProjectedPosition.Y <= MaxBounds.Y &&
		!bBehindCamera;

	bOutOnScreen = bInsideBounds;
	OutScreenPosition = FVector2D(
		FMath::Clamp(ProjectedPosition.X, MinBounds.X, MaxBounds.X),
		FMath::Clamp(ProjectedPosition.Y, MinBounds.Y, MaxBounds.Y));

	// 화살표 방향 산출
	const FVector2D ArrowDirection = (OutScreenPosition - LayerCenter).GetSafeNormal();
	OutArrowAngle = ConvertDirectionToArrowAngle(ArrowDirection);
	return true;
}

float UPRWorldMarkerLayerWidget::ConvertDirectionToArrowAngle(const FVector2D& Direction) const
{
	if (Direction.IsNearlyZero())
	{
		return 0.0f;
	}

	// 0도 아래 기준 변환
	return -FMath::RadiansToDegrees(FMath::Atan2(Direction.X, Direction.Y));
}

// ===== 만료 정리 =====

void UPRWorldMarkerLayerWidget::RetireExpiredMarkers()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float CurrentTimeSeconds = World->GetTimeSeconds();
	TArray<FGuid> ExpiredMarkerIds;
	for (const TPair<FGuid, TObjectPtr<UPRWorldMarkerWidget>>& Pair : ActiveMarkerWidgets)
	{
		// 만료 후보 수집
		if (!IsValid(Pair.Value) || Pair.Value->IsExpired(CurrentTimeSeconds))
		{
			ExpiredMarkerIds.Add(Pair.Key);
		}
	}

	for (const FGuid& MarkerId : ExpiredMarkerIds)
	{
		// 지연 제거
		RemoveMarker(MarkerId);
	}
}
