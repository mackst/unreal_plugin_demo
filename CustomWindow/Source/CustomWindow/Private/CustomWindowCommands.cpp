// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomWindowCommands.h"

#define LOCTEXT_NAMESPACE "FCustomWindowModule"

void FCustomWindowCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "CustomWindow", "Bring up CustomWindow window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
