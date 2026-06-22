// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (웨이포인트 Travel Map UI 위젯 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/UI/PRWidgetBase.h"
#include "ProjectR/UI/WaypointTravel/PRWaypointTravelTypes.h"
#include "PRWaypointTravelMapWidget.generated.h"

class UPRWaypointTravelNodeWidget;
class UPRWorldRegistry;

// 월드별 지도 Blueprint의 공통 기반 위젯
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRWaypointTravelMapWidget : public UPRWidgetBase
{
	GENERATED_BODY()

public:
	UPRWaypointTravelMapWidget();
	
	// 지도 화면이 표시할 월드 ID 지정 후 Waypoint 상태 자동 조회
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WaypointTravel")
	void SetMapContext(FGameplayTag InWorldId);

	// 지도 자식 버튼을 수집하고 Waypoint 상태 적용
	UFUNCTION(BlueprintCallable, Category = "ProjectR|WaypointTravel")
	void RefreshWaypointNodes();

	// 현재 지도 화면의 월드 ID 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|WaypointTravel")
	FGameplayTag GetWorldId() const { return WorldId; }

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// WidgetTree에서 Waypoint 버튼 수집
	void CollectWaypointNodeWidgets();

	// 지도 Waypoint 버튼 이벤트 바인딩 해제
	void UnbindWaypointNodeWidgets();

	// 지정 월드의 Waypoint 목록과 활성 상태 생성
	TArray<FPRWaypointTravelNodeOption> BuildWaypointOptions(FGameplayTag InWorldId) const;

	// 프로젝트 월드 Registry 조회
	const UPRWorldRegistry* GetWorldRegistry() const;

private:
	// 현재 지도 화면이 표시하는 월드 ID
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ProjectR|WaypointTravel", meta = (AllowPrivateAccess = "true"))
	FGameplayTag WorldId;

	// 현재 지도 화면에 적용할 Waypoint 상태 목록
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ProjectR|WaypointTravel", meta = (AllowPrivateAccess = "true"))
	TArray<FPRWaypointTravelNodeOption> NodeOptions;

	// WidgetTree에서 수집한 지도 Waypoint 버튼 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPRWaypointTravelNodeWidget>> WaypointNodeWidgets;
};
