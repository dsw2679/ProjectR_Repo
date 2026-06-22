// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Menu Game Mode 구현)
#include "PRMenuGameMode.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Game/PRMenuHUD.h"
#include "ProjectR/Player/PRMenuPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"

APRMenuGameMode::APRMenuGameMode()
{
	// 메뉴는 복제 불필요. GameMode 기본값 유지
	bUseSeamlessTravel = true;

	// 메뉴 플레이어 입력과 HUD 표시 클래스 지정
	PlayerControllerClass = APRMenuPlayerController::StaticClass();
	PlayerStateClass = APRPlayerState::StaticClass();
	HUDClass = APRMenuHUD::StaticClass();
	DefaultPawnClass = nullptr;
	PreviewCharacterClass = APRPlayerCharacter::StaticClass();
}

/*~ AGameModeBase Interface ~*/

void APRMenuGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 메뉴 진입 직후 프리뷰 런타임 소스 준비
	EnsurePreviewCharacter(NewPlayer);
}

void APRMenuGameMode::Logout(AController* Exiting)
{
	if (PreviewController.Get() == Exiting && IsValid(PreviewCharacter))
	{
		PreviewCharacter->Destroy();
		PreviewCharacter = nullptr;
		PreviewController = nullptr;
	}

	Super::Logout(Exiting);
}

/*~ APRMenuGameMode Interface ~*/

bool APRMenuGameMode::ApplyPreviewSaveData(APlayerController* RequestingController, const FPRCharacterSaveData& InSaveData)
{
	APRPlayerState* PlayerState = GetPreviewPlayerState(RequestingController);
	if (!IsValid(PlayerState))
	{
		return false;
	}

	APRPlayerCharacter* TargetPreviewCharacter = EnsurePreviewCharacter(RequestingController);
	if (!IsValid(TargetPreviewCharacter))
	{
		return false;
	}

	// 실제 PlayerState 시스템에 세이브 데이터 적용
	PlayerState->ApplySaveData(InSaveData);
	ConfigurePreviewCharacter(TargetPreviewCharacter);
	return true;
}

APRPlayerState* APRMenuGameMode::GetPreviewPlayerState(APlayerController* RequestingController) const
{
	APlayerController* SourceController = IsValid(RequestingController)
		? RequestingController
		: PreviewController.Get();
	return IsValid(SourceController) ? SourceController->GetPlayerState<APRPlayerState>() : nullptr;
}

APRPlayerCharacter* APRMenuGameMode::GetPreviewCharacter(APlayerController* RequestingController) const
{
	if (!IsValid(PreviewCharacter))
	{
		return nullptr;
	}

	if (IsValid(RequestingController) && PreviewController.IsValid() && PreviewController.Get() != RequestingController)
	{
		return nullptr;
	}

	return PreviewCharacter;
}

APRPlayerCharacter* APRMenuGameMode::EnsurePreviewCharacter(APlayerController* RequestingController)
{
	if (!IsValid(RequestingController))
	{
		return nullptr;
	}

	if (PreviewController.IsValid() && PreviewController.Get() != RequestingController && IsValid(PreviewCharacter))
	{
		PreviewCharacter->Destroy();
		PreviewCharacter = nullptr;
	}

	PreviewController = RequestingController;
	bool bSpawnedPreviewCharacter = false;
	if (!IsValid(PreviewCharacter))
	{
		UWorld* World = GetWorld();
		if (!IsValid(World))
		{
			return nullptr;
		}

		TSubclassOf<APRPlayerCharacter> SpawnClass = PreviewCharacterClass;
		if (!IsValid(SpawnClass.Get()))
		{
			SpawnClass = APRPlayerCharacter::StaticClass();
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = RequestingController;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const FTransform SpawnTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, -90000.0f));
		PreviewCharacter = World->SpawnActor<APRPlayerCharacter>(SpawnClass, SpawnTransform, SpawnParameters);
		bSpawnedPreviewCharacter = IsValid(PreviewCharacter);
	}

	if (!IsValid(PreviewCharacter))
	{
		return nullptr;
	}

	ConfigurePreviewCharacter(PreviewCharacter);
	if (bSpawnedPreviewCharacter)
	{
		PreviewCharacter->InitCharacter(RequestingController);
	}

	return PreviewCharacter;
}

void APRMenuGameMode::ConfigurePreviewCharacter(APRPlayerCharacter* InPreviewCharacter) const
{
	if (!IsValid(InPreviewCharacter))
	{
		return;
	}

	// 메뉴 월드 직접 노출과 충돌 차단
	InPreviewCharacter->SetActorLocation(FVector(0.0f, 0.0f, -90000.0f));
	InPreviewCharacter->SetActorEnableCollision(false);
	InPreviewCharacter->SetActorTickEnabled(false);
	if (UCapsuleComponent* CapsuleComponent = InPreviewCharacter->GetCapsuleComponent())
	{
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
