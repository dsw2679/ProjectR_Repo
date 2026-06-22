// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRTextButton.h"

#include "Components/TextBlock.h"

void UPRTextButton::SetText(FText InText)
{
	if (IsValid(ButtonText))
	{
		ButtonText->SetText(InText);
	}
}
