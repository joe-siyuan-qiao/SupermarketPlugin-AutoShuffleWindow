// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

class FAutoShuffleWindowModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	
private:

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
    

/** The following are the implementations of the auto shuffle */
private:
    /** The main entry of the algorithm */
    static void AutoShuffleImplementation();
    
    /** SpinBox for Density -- the density of the productions */
    static TSharedRef<SSpinBox<float>> DensitySpinBox;
    
    /** SpinBox for Proxmity -- how similar products are placed */
    static TSharedRef<SSpinBox<float>> ProxmitySpinBox;
    
    /** Read the Whitelist of shelves and products from configure file */
    static void ReadWhitelist();
    
    /** Whitelist of the shelves */
    static TList<FText>* ShelvesWhitelist;
    
    /** Whitelist of the products */
    static TList<FText>* ProductsWhitelist;
    
};
