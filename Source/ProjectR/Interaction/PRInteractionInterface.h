// Copyright (c) 2026 TeamApoc. All Rights Reserved.
// Author: 배유찬 (Interaction 인터페이스 정의)
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PRInteractionInterface.generated.h"

class UPRInteractableComponent;
// This class does not need to be modified.
UINTERFACE()
class UPRInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PROJECTR_API IPRInteractionInterface
{
	GENERATED_BODY()

public:
	virtual UPRInteractableComponent* GetInteractableComponent() const = 0;
};