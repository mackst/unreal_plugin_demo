// Copyright Epic Games, Inc. All Rights Reserved.

#include "PointCloudRenderCommands.h"

#define LOCTEXT_NAMESPACE "FPointCloudRenderModule"

void FPointCloudRenderCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "PointCloudRender", "Bring up PointCloudRender window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
