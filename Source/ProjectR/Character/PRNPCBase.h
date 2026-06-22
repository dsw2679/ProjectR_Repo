// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProjectR/Interaction/PRInteractionInterface.h"
#include "ProjectR/UI/WorldMarker/PRPingMarkerTargetInterface.h"
#include "PRNPCBase.generated.h"

UCLASS()
class PROJECTR_API APRNPCBase : public ACharacter, public IPRInteractionInterface , public IPRPingMarkerTargetInterface
{
	GENERATED_BODY()

public:
	APRNPCBase();
	
	/*~ IPRInteractionInterface ~*/
	virtual UPRInteractableComponent* GetInteractableComponent() const override;
	
	/*~ IPRPingMarkerTargetInterface ~*/
	virtual FPRWorldMarkerVisualData GetPingMarkerVisualData_Implementation() const override;
	virtual FVector GetPingMarkerWorldLocation_Implementation() const override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UPRInteractableComponent> NPCInteractable; 
	
	// ===== Configs =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|UI")
	FVector PingMarkerOffset = FVector(0.0f, 0.0f, 30.0f);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|UI")
	bool bOverridesPingMarker = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bOverridesPingMarker == true" ),  Category = "ProjectR|UI")
	FPRWorldMarkerVisualData PingMarkerVisualOverride;
};
