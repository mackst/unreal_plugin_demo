// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomWindow.h"
#include "CustomWindowStyle.h"
#include "CustomWindowCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "ToolMenus.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Engine/StaticMesh.h"
#include "AssetToolsModule.h"
#include "Factories/MaterialFactoryNew.h"
//#include "Materials/MaterialInstanceConstant.h"
//#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"


static const FName CustomWindowTabName("CustomWindow");

#define LOCTEXT_NAMESPACE "FCustomWindowModule"

void FCustomWindowModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FCustomWindowStyle::Initialize();
	FCustomWindowStyle::ReloadTextures();

	FCustomWindowCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FCustomWindowCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FCustomWindowModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FCustomWindowModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(CustomWindowTabName, FOnSpawnTab::CreateRaw(this, &FCustomWindowModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FCustomWindowTabTitle", "CustomWindow"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FCustomWindowModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FCustomWindowStyle::Shutdown();

	FCustomWindowCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(CustomWindowTabName);
}

TSharedRef<SDockTab> FCustomWindowModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FCustomWindowModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("CustomWindow.cpp"))
		);

	FSlateFontInfo TitleTextFont = GetEmbossedTextFont();
	TitleTextFont.Size = 30;
	FSlateFontInfo SecTitleTextFont = GetEmbossedTextFont();
	SecTitleTextFont.Size = 24;

	PrimID = 1;
	MatName = FText::FromString(TEXT("M_Test"));
	AssetPath = FText::FromString(TEXT("/Game/Assets/"));

	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// 主控件
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				// 顶头标题
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("自定义工具")))
				.Font(TitleTextFont)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FColor::White)
			]

			+SVerticalBox::Slot()
			[
				// 滚动盒
				SNew(SScrollBox)
				+SScrollBox::Slot()
				//.Padding(1.0f, 15.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 10.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("关卡工具")))
						.Font(SecTitleTextFont)
						.Justification(ETextJustify::Center)
						.ColorAndOpacity(FColor::White)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 10.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("添加原始形状模型")))
						.Justification(ETextJustify::Center)
						.ColorAndOpacity(FColor::Cyan)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						// 参考
						// \Engine\Source\Runtime\AppFramework\Private\Framework\Testing\SStarshipGallery.cpp
						SNew(SSegmentedControl<int32>)
						.Value(0)
						.OnValueChanged_Lambda([this](int32 InValue) { PrimID = InValue; })
						// 控制控件值
						.Value_Lambda([this] { return PrimID; })

						+ SSegmentedControl<int32>::Slot(0)
						.Icon(FAppStyle::Get().GetBrush("Icons.box-perspective"))
						.Text(LOCTEXT("Box", "方块"))

						+ SSegmentedControl<int32>::Slot(1)
						.Icon(FAppStyle::Get().GetBrush("Icons.cylinder"))
						.Text(LOCTEXT("Cylinder", "圆柱"))

						+ SSegmentedControl<int32>::Slot(2)
						.Icon(FAppStyle::Get().GetBrush("Icons.pyramid"))
						.Text(LOCTEXT("Pyramid", "金字塔"))

						+ SSegmentedControl<int32>::Slot(3)
						.Icon(FAppStyle::Get().GetBrush("Icons.sphere"))
						.Text(LOCTEXT("Sphere", "球"))
					]

					/*+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSegmentedControl<int32>)
						.Value(0)
						.OnValueChanged_Lambda([this](int32 InValue) { PrimID = InValue; })
						.Value_Lambda([this] { return PrimID; })

						+ SSegmentedControl<int32>::Slot(0)
						.Icon(FAppStyle::Get().GetBrush("Icons.box-perspective"))

						+ SSegmentedControl<int32>::Slot(1)
						.Icon(FAppStyle::Get().GetBrush("Icons.cylinder"))

						+ SSegmentedControl<int32>::Slot(2)
						.Icon(FAppStyle::Get().GetBrush("Icons.pyramid"))

						+ SSegmentedControl<int32>::Slot(3)
						.Icon(FAppStyle::Get().GetBrush("Icons.sphere"))
					]*/

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(100.0f, 5.0f)
					[
						SNew(SButton)
						//.Text(FText::FromString(TEXT("添加")))
						//.HAlign(EHorizontalAlignment::HAlign_Center)	
						.OnClicked(FOnClicked::CreateRaw(this, &FCustomWindowModule::OnAddPrimBtnClicked))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("添加")))
							.Justification(ETextJustify::Center)
							.ColorAndOpacity(FColor::Yellow)
						]
					]

					// 资产相关
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 20.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("资产工具")))
						.Font(SecTitleTextFont)
						.Justification(ETextJustify::Center)
						.ColorAndOpacity(FColor::White)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(10.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("资产路径：")))
							.Justification(ETextJustify::Left)
						]

						+ SHorizontalBox::Slot()
						[
							SNew(SEditableTextBox)
							.Text(AssetPath)
							.OnTextCommitted_Lambda([this](FText NewText, ETextCommit::Type TextType) { AssetPath = NewText; })
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 10.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("添加材质资产")))
						.Justification(ETextJustify::Center)
						.ColorAndOpacity(FColor::Cyan)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(10.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("材质名称：")))
							.Justification(ETextJustify::Left)
						]

						+ SHorizontalBox::Slot()
						[
							SNew(SEditableTextBox)
							.Text(MatName)
							//.OnTextCommitted(FOnTextCommitted::CreateRaw)
							.OnTextCommitted_Lambda([this](FText NewText, ETextCommit::Type TextType) { MatName = NewText; })
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(10.0f, 0.0f)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("添加")))
							.HAlign(EHorizontalAlignment::HAlign_Center)
							.OnClicked(FOnClicked::CreateRaw(this, &FCustomWindowModule::OnAddMaterialBtnClicked))
						]
					]
				]
			]
		];

	return Tab;
}

// 添加形状物体到关卡按钮函数回调
FReply FCustomWindowModule::OnAddPrimBtnClicked()
{
	// 获取EditorActorSubsystem
	UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
	if (EditorActorSubsystem == nullptr) return FReply::Handled();

	// 根据用户选择加载相关模型资产
	UStaticMesh* StaticMesh = nullptr;
	switch (PrimID)
	{
	case 0:
		StaticMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		break;
	case 1:
		StaticMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		break;
	case 2:
		StaticMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/StarterContent/Shapes/Shape_TriPyramid.Shape_TriPyramid"));
		break;
	case 3:
		StaticMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		break;
	default:
		break;
	}
	
	if (StaticMesh)
	{
		// 添加到关卡中
		AActor* Actor = EditorActorSubsystem->SpawnActorFromObject(StaticMesh, FVector(0.0f, 0.0f, 200.0f));
		//Actor->SetActorLabel();
	}
	return FReply::Handled();
}

FReply FCustomWindowModule::OnAddMaterialBtnClicked()
{
	// 加载AssetTools模块
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	// 创建材质需要UMaterialFactoryNew
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

	// 获取一个独一无二的名字
	FString AssetName = MatName.ToString();
	FString InPackgePath = AssetPath.ToString();
	if (!InPackgePath.EndsWith("/"))
		InPackgePath += "/";
	InPackgePath += AssetName;

	FString UniquePath;
	AssetToolsModule.Get().CreateUniqueAssetName(InPackgePath, "", UniquePath, AssetName);
	// 创建材质
	UObject* MatAsset = AssetToolsModule.Get().CreateAsset(AssetName, AssetPath.ToString(), UMaterial::StaticClass(), MaterialFactory);

	// 提示信息
	FText Message = FText::Format(
		FText::FromString(L"材质创建完成\n材质名称：{0}\n路径：{1}"),
		FText::FromString(AssetName),
		AssetPath
	);
	FNotificationInfo NotifyInfo(Message);
	NotifyInfo.bUseLargeFont = true;
	NotifyInfo.FadeOutDuration = 6.0f;

	FSlateNotificationManager::Get().AddNotification(NotifyInfo);

	return FReply::Handled();
}

void FCustomWindowModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(CustomWindowTabName);
}

void FCustomWindowModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FCustomWindowCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FCustomWindowCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCustomWindowModule, CustomWindow)