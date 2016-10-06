// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AutoShuffleWindowPrivatePCH.h"
#include "AutoShuffleWindowCommands.h"

#define LOCTEXT_NAMESPACE "FAutoShuffleWindowModule"

void FAutoShuffleWindowCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "AutoShuffleWindow", "Bring up AutoShuffleWindow window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
