// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AutoShuffleWindowPrivatePCH.h"

#include "SlateBasics.h"
#include "SlateExtras.h"

#include "AutoShuffleWindowStyle.h"
#include "AutoShuffleWindowCommands.h"

#include "LevelEditor.h"

static const FName AutoShuffleWindowTabName("AutoShuffleWindow");

#define LOCTEXT_NAMESPACE "FAutoShuffleWindowModule"

void FAutoShuffleWindowModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FAutoShuffleWindowStyle::Initialize();
	FAutoShuffleWindowStyle::ReloadTextures();

	FAutoShuffleWindowCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FAutoShuffleWindowCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FAutoShuffleWindowModule::PluginButtonClicked),
		FCanExecuteAction());
		
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FAutoShuffleWindowModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
	
	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FAutoShuffleWindowModule::AddToolbarExtension));
		
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(AutoShuffleWindowTabName, FOnSpawnTab::CreateRaw(this, &FAutoShuffleWindowModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FAutoShuffleWindowTabTitle", "AutoShuffleWindow"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FAutoShuffleWindowModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FAutoShuffleWindowStyle::Shutdown();

	FAutoShuffleWindowCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AutoShuffleWindowTabName);
}

TSharedRef<SDockTab> FAutoShuffleWindowModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
    
    TSharedRef<SSpinBox<float>> SpinBox = SNew(SSpinBox<float>);
    TSharedRef<SButton> Button = SNew(SButton);
    Button->SetVAlign(VAlign_Center);
    Button->SetHAlign(HAlign_Center);
    
    auto OnButtonClicked = []() -> FReply
    {
        UE_LOG(LogClass, Log, TEXT("Button Clicked."));
        return FReply::Handled();
    };
    
    Button->SetOnClicked(FOnClicked::CreateLambda(OnButtonClicked));
    FText Density = FText::FromString(TEXT("Density      "));
    FText Proxmity = FText::FromString(TEXT("Proxmity   "));
    
    return SNew(SDockTab)
    .TabRole(ETabRole::NomadTab)
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .Padding(30.f, 10.f)
        .AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Center)
            .AutoWidth()
            [
                SNew(STextBlock)
                .Text(Density)
            ]
            + SHorizontalBox::Slot()
            .HAlign(HAlign_Fill)
            [
                SpinBox
            ]
        ]
        + SVerticalBox::Slot()
        .Padding(30.f, 10.f)
        .AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Center)
            .AutoWidth()
            [
                SNew(STextBlock)
                .Text(Proxmity)
            ]
            + SHorizontalBox::Slot()
            .HAlign(HAlign_Fill)
            [
                SpinBox
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(30.f, 10.f)
        [
            Button
        ]
    ];
}

void FAutoShuffleWindowModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(AutoShuffleWindowTabName);
}

void FAutoShuffleWindowModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FAutoShuffleWindowCommands::Get().OpenPluginWindow);
}

void FAutoShuffleWindowModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FAutoShuffleWindowCommands::Get().OpenPluginWindow);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAutoShuffleWindowModule, AutoShuffleWindow)