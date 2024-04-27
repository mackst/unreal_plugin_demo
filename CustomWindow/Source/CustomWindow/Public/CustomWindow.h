// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

class FCustomWindowModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	
private:

	void RegisterMenus();

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

	FSlateFontInfo GetEmbossedTextFont() const {
		return FCoreStyle::Get().GetFontStyle(FName("EmbossedText"));
	};

	FReply OnAddPrimBtnClicked();
	FReply OnAddMaterialBtnClicked();
private:
	TSharedPtr<class FUICommandList> PluginCommands;
	int32 PrimID;
	FText MatName;
	FText AssetPath;
};
