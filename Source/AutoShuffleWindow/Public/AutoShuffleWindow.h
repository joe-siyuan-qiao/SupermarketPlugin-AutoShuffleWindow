// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;
class FAutoShuffleObject;
class FAutoShuffleShelf;
class FAutoShuffleProductGroup;

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

	/** Check box for toggling product organizing */
	static TSharedRef<SCheckBox> OrganizeCheckBox;

	/** The status of the organize checkbox when button clicked */
	static bool bIsOrganizeChecked;

	/** Check box for per group organizing */
	static TSharedRef<SCheckBox> PerGroupCheckBox;

	/** The status of the pergroup checkbox when button clicked */
	static bool bIsPerGroupChecked;
    
    /** The regions for the discarded products */
    static FVector DiscardedProductsRegions;
    
    /** Read the Whitelist of shelves and products from configure file */
    static bool ReadWhitelist();
    
    /** Whitelist of the shelves */
    static TArray<FAutoShuffleShelf>* ShelvesWhitelist;
    
    /** Whitelist of the products */
    static TArray<FAutoShuffleProductGroup>* ProductsWhitelist;
    
    /** Add noise to position.Z of the shelf of given name w.r.t. the first shelf (fixed) */
    static void AddNoiseToShelf(const FString& ShelfName, float NoiseScale);
    
    /** Place the products onto the shelves */
    static void PlaceProducts(float Density, float Proxmity);
    
    /** Lower all the products so that they can almost touch the shevles */
    static void LowerProducts();
    
    /** Organize the objects that are already on the shelves -- push all of them to left of the shelf until collided. Should respond to a checkbox on the plugin window */
    static void OrganizeProducts();
    
    /** Predicate used for sorting AActors in OrganizeProducts from low to high */
    static bool OrganizeProductsPredicateLowToHigh(const AActor &Actor1, const AActor &Actor2);

	/** Predicate used for sorting AActors in OrganizeProducts from high to low */
	static bool OrganizeProductsPredicateHighToLow(const AActor &Actor1, const AActor &Actor2);

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
    
    /** Shrink the scale: keep x, y and 1/3 z. Used to fit to the shelf */
    void ShrinkScale();
    
    /** Expand the Scale: set scale of x, y to z, then expand as big as possible before original scale of x and y. Used to fit to the shelf */
    void ExpandScale();
    
    /** Set the ObjectActor */
    void SetObjectActor(AActor* NewObjectActor);
    
    /** Get the ObjectActor */
    AActor* GetObjectActor() const;
    
    /** Set the bIsDiscarded */
    void Discard();
    
    /** Get the bIsDiscarded */
    bool IsDiscarded();
    
    /** Reset the bIsDiscarded */
    void ResetDiscard();
    
    /** Set the fShelfOffset */
    void SetShelfOffset(float NewShelfOffset);
    
    /** Get the fShelfOffset */
    float GetShelfOffset() const;
    
    /** Return if the object is contained in the given bound */
    bool IsContainedIn(FVector& BoundOrigin, FVector& BoundExtent);
    
    /** Return if the object is on shelf */
    bool IsOnShelf() const;
    
    /** Reset the bIsOnShelf */
    void ResetOnShelf();
    
    /** Put on Shelf */
    void SetOnShelf();
    
    
private:
    /** The rendering scale of the shelf in the editor world */
    float Scale;
    
    /** The name of the object in the editor world */
    FString Name;
    
    /** The Position of the object in the editor world */
    FVector Position;
    
    /** The Rotation of the object in the editor world */
    FVector Rotation;
    
    /** Pointer to the AActor in the Editor World */
    AActor* ObjectActor;
    
    /** Whether this object has been discarded */
    bool bIsDiscarded;
    
    /** Because the collision model of the shelf isn't perfect, we need to add offset to the object 
     *  to make it touch the surface of the shelf even if collision is already detected. 
     *  @note this value is not a relative value. It is the real value. 
     *  @todo find more elegant way of doing this */
    float fShelfOffset;
    
    /** Whether the object is already placed on the shelf */
    bool bIsOnShelf;
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
    
    /** Get the shelf offset */
    TArray<float>* GetShelfOffset() const;
    
    /** Set the shelf offset */
    void SetShelfOffset(TArray<float>* NewShelfOffset);
    
private:
    /** The relative heights of each level of bases measured from bottom */
    TArray<float>* ShelfBase;
    
    /** The relative offset of each level of bases measured from bottom */
    TArray<float>* ShelfOffset;
};

class FAutoShuffleProductGroup
{
public:
    /** Construct and Deconstruct */
    FAutoShuffleProductGroup();
    ~FAutoShuffleProductGroup();
    
    /** Set the members. Each member is an instance of the product group */
    void SetMembers(TArray<FAutoShuffleObject>* NewMembers);
    
    /** Get the members */
    TArray<FAutoShuffleObject>* GetMembers() const;
    
    /** Set the group name */
    void SetName(FString& NewName);

    /** Get the group name */
    FString GetName() const;
    
    /** Set the shelf name */
    void SetShelfName(FString& NewShelfName);
    
    /** Get the shelf name */
    FString GetShelfName() const;

	/** Discard the whole groups. Defined in the whitelist */
	void Discard();

	/** Reset the bIsDiscarded variable */
	void ResetDiscard();

	/** Return if the whole group is discarded */
	bool IsDiscarded();
    
private:
    /** The members of the product. Proxmity is used for deciding placing members */
    TArray<FAutoShuffleObject>* Members;
    
    /** The group name */
    FString Name;
    
    /** The name of the shelf that this group belongs to */
    FString ShelfName;

	/** Whether the whole group is discarded by whitelist */
	/** This is extremely important */
	bool bIsDiscarded;
};



