// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;
class FAutoShuffleObject;
class FAutoShuffleShelf;
class FAutoShuffleProduct;

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
    static bool ReadWhitelist();
    
    /** Whitelist of the shelves */
    static TArray<FAutoShuffleShelf>* ShelvesWhitelist;
    
    /** Whitelist of the products */
    static TArray<FAutoShuffleProduct>* ProductsWhitelist;

public:
    /** Static method for parsing the Whitelist written in Json */
    static TSharedPtr<FJsonObject> ParseJSON(const FString& FileContents, const FString& NameForErrors, bool bSilent);
    
};

class FAutoShuffleObject
{
public:
    /** Construct and Deconstruct */
    FAutoShuffleObject();
    ~FAutoShuffleObject();
    
    /** Set the object name */
    void SetName(FString& NewName);
    
    /** Get the object name */
    FString GetName() const;
    
    /** Set the position */
    void SetPosition(FVector& NewPosition);
    
    /** Get the position */
    FVector GetPosition() const;
    
    /** Set the rotation */
    void SetRotation(FVector& NewRotation);
    
    /** Get the rotation */
    FVector GetRotation() const;
    
    /** Set the scale */
    void SetScale(float NewScale);
    
    /** Get the scale */
    float GetScale() const;
    
private:
    /** The rendering scale of the shelf in the editor world */
    float Scale;
    
    /** The name of the object in the editor world */
    FString Name;
    
    /** The Position of the object in the editor world */
    FVector Position;
    
    /** The Rotation of the object in the editor world */
    FVector Rotation;
};

class FAutoShuffleShelf : public FAutoShuffleObject
{
public:
    /** Construct and Deconstruct */
    FAutoShuffleShelf();
    ~FAutoShuffleShelf();
    
    /** Get the shelf base */
    TArray<float>* GetShelfBase() const;
    
    /** Set the shelf base */
    void SetShelfBase(TArray<float>* NewShelfBase);
    
private:
    /** The relative heights of each level of bases measured from bottom */
    TArray<float>* ShelfBase;
};

class FAutoShuffleProduct : public FAutoShuffleObject
{
    
};



