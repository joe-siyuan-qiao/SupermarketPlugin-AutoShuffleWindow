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
    // init or re-init the SpinBoxes
    DensitySpinBox = SNew(SSpinBox<float>);
    DensitySpinBox->SetMinValue(0.f);
    DensitySpinBox->SetMaxValue(1.f);
    DensitySpinBox->SetMinSliderValue(0.f);
    DensitySpinBox->SetMaxSliderValue(1.f);
    DensitySpinBox->SetValue(0.5f);
    ProxmitySpinBox = SNew(SSpinBox<float>);
    ProxmitySpinBox->SetMinValue(0.f);
    ProxmitySpinBox->SetMaxValue(1.f);
    ProxmitySpinBox->SetMinSliderValue(0.f);
    ProxmitySpinBox->SetMaxSliderValue(1.f);
    ProxmitySpinBox->SetValue(0.5f);
    
    
    TSharedRef<SButton> Button = SNew(SButton);
    Button->SetVAlign(VAlign_Center);
    Button->SetHAlign(HAlign_Center);
    Button->SetContent(SNew(STextBlock).Text(FText::FromString(TEXT("Auto Shuffle"))));
    
    auto OnButtonClickedLambda = []() -> FReply
    {
        AutoShuffleImplementation();
        return FReply::Handled();
    };
    
    Button->SetOnClicked(FOnClicked::CreateLambda(OnButtonClickedLambda));
    FText Density = FText::FromString(TEXT("Density      "));
    FText Proxmity = FText::FromString(TEXT("Proxmity   "));
    
    return SNew(SDockTab).TabRole(ETabRole::NomadTab)
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().Padding(30.f, 10.f).AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center).AutoWidth()
            [
                SNew(STextBlock).Text(Density)
            ]
            + SHorizontalBox::Slot().HAlign(HAlign_Fill)
            [
                DensitySpinBox
            ]
        ]
        + SVerticalBox::Slot().Padding(30.f, 10.f).AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center).AutoWidth()
            [
                SNew(STextBlock).Text(Proxmity)
            ]
            + SHorizontalBox::Slot().HAlign(HAlign_Fill)
            [
                ProxmitySpinBox
            ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(30.f, 10.f)
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

/** The folloinwg are the implemetations of the auto shuffle */
TSharedRef<SSpinBox<float>> FAutoShuffleWindowModule::DensitySpinBox = SNew(SSpinBox<float>);
TSharedRef<SSpinBox<float>> FAutoShuffleWindowModule::ProxmitySpinBox = SNew(SSpinBox<float>);
TList<FText>* FAutoShuffleWindowModule::ShelvesWhitelist = nullptr;
TList<FText>* FAutoShuffleWindowModule::ProductsWhitelist = nullptr;

void FAutoShuffleWindowModule::AutoShuffleImplementation()
{
    float Density = FAutoShuffleWindowModule::DensitySpinBox->GetValue();
    float Proxmity = FAutoShuffleWindowModule::ProxmitySpinBox->GetValue();
    FAutoShuffleWindowModule::ReadWhitelist();
    UE_LOG(LogClass, Log, TEXT("Density: %f Proxmity: %f"), Density, Proxmity);
}

void FAutoShuffleWindowModule::ReadWhitelist()
{
    // Always read the list even if the whitelists have been initialized
    // This is to make sure that changes of the configurations can be directly reflected every time the button is clicked
    // NOTE: the file may be in sandbox which is not changable after loading
    if (FAutoShuffleWindowModule::ShelvesWhitelist != nullptr)
    {
        delete [] FAutoShuffleWindowModule::ShelvesWhitelist;
        FAutoShuffleWindowModule::ShelvesWhitelist = nullptr;
    }
    if (FAutoShuffleWindowModule::ProductsWhitelist != nullptr)
    {
        delete [] FAutoShuffleWindowModule::ProductsWhitelist;
        FAutoShuffleWindowModule::ProductsWhitelist = nullptr;
    }
    FString PluginDir = FPaths::Combine(*FPaths::GamePluginsDir(), TEXT("AutoShuffleWindow"));
    FString ResourseDir = FPaths::Combine(*PluginDir, TEXT("Resources"));
    FString FileDir = FPaths::Combine(*ResourseDir, TEXT("Whitelist"));
    FString Filedata = "";
    FFileHelper::LoadFileToString(Filedata, *FileDir);
    UE_LOG(LogClass, Log, TEXT("FileDit: %s"), *FileDir);
    UE_LOG(LogClass, Log, TEXT("From txt file: %s"), *Filedata);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAutoShuffleWindowModule, AutoShuffleWindow)