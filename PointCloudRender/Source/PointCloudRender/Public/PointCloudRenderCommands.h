// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "PointCloudRenderStyle.h"

class FPointCloudRenderCommands : public TCommands<FPointCloudRenderCommands>
{
public:

	FPointCloudRenderCommands()
		: TCommands<FPointCloudRenderCommands>(TEXT("PointCloudRender"), NSLOCTEXT("Contexts", "PointCloudRender", "PointCloudRender Plugin"), NAME_None, FPointCloudRenderStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};