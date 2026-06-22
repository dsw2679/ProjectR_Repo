// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (Input 설정 데이터 에셋 설정 클래스 정의)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PRInputConfigDataAsset.generated.h"

class UInputAction;

// IA : InputTag 매핑 엔트리
USTRUCT(BlueprintType)
struct PROJECTR_API FPRInputActionBinding
{
	GENERATED_BODY()

	// 바인딩할 Enhanced Input Action
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction;

	// ASC로 전달할 InputTag (Input.Ability.* 네임스페이스)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Input.Ability"))
	FGameplayTag InputTag;
};

// IA : InputTag 매핑 테이블. PlayerController SetupInputComponent에서 소비
UCLASS(BlueprintType)
class PROJECTR_API UPRInputConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 어빌리티 입력 바인딩 목록
	UPROPERTY(EditAnywhere, Category = "Input")
	TArray<FPRInputActionBinding> AbilityInputBindings;

	// InputTag로 IA 조회
	const UInputAction* FindInputActionForTag(const FGameplayTag& InputTag) const;
};
