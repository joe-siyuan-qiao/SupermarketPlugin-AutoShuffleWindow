// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateBasics.h"
#include "AutoShuffleWindowStyle.h"

class FAutoShuffleWindowCommands : public TCommands<FAutoShuffleWindowCommands>
{
public:

	FAutoShuffleWindowCommands()
		: TCommands<FAutoShuffleWindowCommands>(TEXT("AutoShuffleWindow"), NSLOCTEXT("Contexts", "AutoShuffleWindow", "AutoShuffleWindow Plugin"), NAME_None, FAutoShuffleWindowStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};