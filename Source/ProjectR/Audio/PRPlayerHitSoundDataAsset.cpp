// Copyright ProjectR. All Rights Reserved.
#include "PRPlayerHitSoundDataAsset.h"

#include "Sound/SoundBase.h"

USoundBase* UPRPlayerHitSoundDataAsset::ResolveHealthDamageSound(FName SourceCharacterID) const
{
	if (!SourceCharacterID.IsNone())
	{
		if (const TObjectPtr<USoundBase>* FoundSound = HealthDamageSoundsByCharacterID.Find(SourceCharacterID))
		{
			if (IsValid(FoundSound->Get()))
			{
				return FoundSound->Get();
			}
		}
	}

	return DefaultHealthDamageSound.Get();
}
