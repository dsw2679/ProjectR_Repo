// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Menu Game Mode 구현)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "PRMenuGameMode.generated.h"

class AController;
class APlayerController;
class APRPlayerCharacter;
class APRPlayerState;

// 메인 메뉴 전용 GameMode. 메뉴 프리뷰 런타임을 담당
// 인게임 로직은 APRPlayGameMode에서 수행함
UCLASS()
class PROJECTR_API APRMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APRMenuGameMode();

	/*~ AGameModeBase Interface ~*/
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	// 선택 세이브를 메뉴 프리뷰 런타임 소스에 적용
	bool ApplyPreviewSaveData(APlayerController* RequestingController, const FPRCharacterSaveData& InSaveData);

	// 메뉴 프리뷰 ASC 소유 PlayerState 조회
	APRPlayerState* GetPreviewPlayerState(APlayerController* RequestingController) const;

	// 메뉴 프리뷰 캐릭터 조회
	APRPlayerCharacter* GetPreviewCharacter(APlayerController* RequestingController) const;

private:
	// 메뉴 프리뷰 캐릭터 생성 또는 재사용
	APRPlayerCharacter* EnsurePreviewCharacter(APlayerController* RequestingController);

	// 메뉴 프리뷰 캐릭터 노출과 충돌 비활성화
	void ConfigurePreviewCharacter(APRPlayerCharacter* InPreviewCharacter) const;

protected:
	// 메뉴 프리뷰에 사용할 플레이어 캐릭터 클래스
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Menu|Preview")
	TSubclassOf<APRPlayerCharacter> PreviewCharacterClass;

private:
	// 현재 메뉴 프리뷰 소유 컨트롤러
	TWeakObjectPtr<APlayerController> PreviewController;

	// 현재 메뉴 프리뷰 캐릭터
	UPROPERTY(Transient)
	TObjectPtr<APRPlayerCharacter> PreviewCharacter;
};
