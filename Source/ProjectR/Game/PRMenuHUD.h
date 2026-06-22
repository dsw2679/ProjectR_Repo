// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Menu HUD 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PRMenuHUD.generated.h"

class UPRMainTitleWidget;
class UPRStartMenuWidget;

// 메인 메뉴 전용 HUD. 타이틀 화면과 시작 메뉴 표시를 담당
UCLASS()
class PROJECTR_API APRMenuHUD : public AHUD
{
	GENERATED_BODY()

protected:
	/*~ AActor Interface ~*/
	// HUD 시작 시 로컬 플레이어 메뉴 화면 표시
	virtual void BeginPlay() override;

	// HUD 종료 시 메뉴 위젯 정리
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 메인 타이틀 화면 표시
	void ShowMainTitle();

	// 시작 메뉴 화면 표시
	void ShowStartMenu();

	// 메인 타이틀 계속 진행 요청 처리
	void HandleMainTitleContinueRequested();

protected:
	// 메뉴 레벨에서 먼저 표시할 메인 타이틀 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Menu")
	TSubclassOf<UPRMainTitleWidget> MainTitleWidgetClass;

	// 메뉴 레벨에서 표시할 시작 메뉴 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Menu")
	TSubclassOf<UPRStartMenuWidget> StartMenuWidgetClass;

private:
	// 현재 표시 중인 메인 타이틀 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRMainTitleWidget> MainTitleWidget;

	// 현재 표시 중인 시작 메뉴 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPRStartMenuWidget> StartMenuWidget;
};
