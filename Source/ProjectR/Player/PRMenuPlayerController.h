// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Menu Player Controller 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PRMenuPlayerController.generated.h"

class UWidget;

// 메인 메뉴 전용 PlayerController. 메뉴 입력 모드와 커서 표시를 담당함
UCLASS()
class PROJECTR_API APRMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 메뉴 UI 전용 입력 모드와 커서 표시를 적용함
	void ApplyMenuInputMode(UWidget* FocusWidget);

protected:
	/*~ AActor Interface ~*/
	// 로컬 메뉴 컨트롤러 시작 시 UI 전용 입력 모드 적용
	virtual void BeginPlay() override;
};
