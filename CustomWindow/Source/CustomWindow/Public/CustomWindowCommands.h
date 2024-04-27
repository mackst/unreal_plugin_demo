// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "CustomWindowStyle.h"

class FCustomWindowCommands : public TCommands<FCustomWindowCommands>
{
public:

	FCustomWindowCommands()
		: TCommands<FCustomWindowCommands>(TEXT("CustomWindow"), NSLOCTEXT("Contexts", "CustomWindow", "CustomWindow Plugin"), NAME_None, FCustomWindowStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};