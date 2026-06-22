// Copyright ProjectR. All Rights Reserved.
// Author: 손승우 (페어린 보스 God Fall 데이터 에셋 구현)
#include "PRFaerinGodFallDataAsset.h"

#include "ProjectR/AI/Boss/PRFaerinGodFallStaticSwordActor.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UPRFaerinGodFallDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (SwordBoneNames.Num() != 10)
	{
		Context.AddError(FText::FromString(TEXT("God Fall sword bone name count must be exactly 10.")));
		Result = EDataValidationResult::Invalid;
	}

	for (int32 BoneIndex = 0; BoneIndex < SwordBoneNames.Num(); ++BoneIndex)
	{
		if (SwordBoneNames[BoneIndex] == NAME_None)
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("God Fall sword bone name at index %d is None."),
				BoneIndex)));
			Result = EDataValidationResult::Invalid;
		}
	}

	if (!IsValid(StaticSwordActorClass))
	{
		Context.AddError(FText::FromString(TEXT("God Fall StaticSwordActorClass is missing.")));
		Result = EDataValidationResult::Invalid;
	}

	if (!IsValid(RigChargeAnimation))
	{
		Context.AddError(FText::FromString(TEXT("God Fall RigChargeAnimation is missing.")));
		Result = EDataValidationResult::Invalid;
	}

	if (!IsValid(RigTiltPullAnimation))
	{
		Context.AddError(FText::FromString(TEXT("God Fall RigTiltPullAnimation is missing.")));
		Result = EDataValidationResult::Invalid;
	}

	if (Phase2SwordChargeMaxSeconds < Phase2SwordChargeMinSeconds)
	{
		Context.AddError(FText::FromString(TEXT("God Fall Phase2SwordChargeMaxSeconds must be greater than or equal to Phase2SwordChargeMinSeconds.")));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif
