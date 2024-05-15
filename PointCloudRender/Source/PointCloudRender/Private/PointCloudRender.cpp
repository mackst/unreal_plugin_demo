// Copyright Epic Games, Inc. All Rights Reserved.

#include "PointCloudRender.h"
#include "PointCloudRenderStyle.h"
#include "PointCloudRenderCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"

#include "VulkanApp.h"


static const FName PointCloudRenderTabName("PointCloudRender");

#define LOCTEXT_NAMESPACE "FPointCloudRenderModule"

void FPointCloudRenderModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FPointCloudRenderStyle::Initialize();
	FPointCloudRenderStyle::ReloadTextures();

	FPointCloudRenderCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FPointCloudRenderCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FPointCloudRenderModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPointCloudRenderModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(PointCloudRenderTabName, FOnSpawnTab::CreateRaw(this, &FPointCloudRenderModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FPointCloudRenderTabTitle", "PointCloudRender"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FPointCloudRenderModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FPointCloudRenderStyle::Shutdown();

	FPointCloudRenderCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PointCloudRenderTabName);
}

TSharedRef<SDockTab> FPointCloudRenderModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
				SNew(SImage)
				.Image_Lambda([this]() { return &ImageBrush; })
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Render")))
				.HAlign(EHorizontalAlignment::HAlign_Center)
				.OnClicked(FOnClicked::CreateRaw(this, &FPointCloudRenderModule::OnRenderButtonClicked))
			]
		];
}

FReply FPointCloudRenderModule::OnRenderButtonClicked()
{
	VulkanApp::PointCloudRender(800, 800, ImageBrush);
	return FReply::Handled();
}

void FPointCloudRenderModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(PointCloudRenderTabName);
}

void FPointCloudRenderModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FPointCloudRenderCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FPointCloudRenderCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPointCloudRenderModule, PointCloudRender)