// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AutoShuffleWindowPrivatePCH.h"

#include "SlateBasics.h"
#include "SlateExtras.h"

#include "AutoShuffleWindowStyle.h"
#include "AutoShuffleWindowCommands.h"

#include "LevelEditor.h"

/** The following header files are not from the template */
#include "Json.h"

static const FName AutoShuffleWindowTabName("AutoShuffleWindow");

#define LOCTEXT_NAMESPACE "FAutoShuffleWindowModule"
DEFINE_LOG_CATEGORY(LogAutoShuffle);

#define VERBOSE_AUTO_SHUFFLE
#define AUTO_SHUFFLE_Y_TWO_END_OFFSET 50.
#define AUTO_SHUFFLE_MAX_TRY_TIMES 1
#define AUTO_SHUFFLE_INC_STEP 0.5f
#define AUTO_SHUFFLE_INC_BOUND 1000

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
TArray<FAutoShuffleShelf>* FAutoShuffleWindowModule::ShelvesWhitelist = nullptr;
TArray<FAutoShuffleProductGroup>* FAutoShuffleWindowModule::ProductsWhitelist = nullptr;

void FAutoShuffleWindowModule::AutoShuffleImplementation()
{
    float Density = FAutoShuffleWindowModule::DensitySpinBox->GetValue();
    float Proxmity = FAutoShuffleWindowModule::ProxmitySpinBox->GetValue();
    bool Result = FAutoShuffleWindowModule::ReadWhitelist();
    if (!Result)
    {
        UE_LOG(LogAutoShuffle, Warning, TEXT("Whitelist read wrong. Module quits."));
    }
    AddNoiseToShelf("BP_ShelfMain_002", 50);
    PlaceProducts(Density, Proxmity);
}

bool FAutoShuffleWindowModule::ReadWhitelist()
{
    // Always read the list even if the whitelists have been initialized
    // This is to make sure that changes of the configurations can be directly reflected every time the button is clicked
    if (FAutoShuffleWindowModule::ShelvesWhitelist != nullptr)
    {
        delete FAutoShuffleWindowModule::ShelvesWhitelist;
        FAutoShuffleWindowModule::ShelvesWhitelist = nullptr;
    }
    if (FAutoShuffleWindowModule::ProductsWhitelist != nullptr)
    {
        delete FAutoShuffleWindowModule::ProductsWhitelist;
        FAutoShuffleWindowModule::ProductsWhitelist = nullptr;
    }
    auto EditorWorld = GEditor->GetEditorWorldContext().World();
    // NOTE: EditorWorld must do InitializeActorsForPlay to make overlapping detection work
    if (!EditorWorld->AreActorsInitialized())
    {
        FURL URL;
        EditorWorld->InitializeActorsForPlay(URL);
    }
    FString PluginDir = FPaths::Combine(*FPaths::GamePluginsDir(), TEXT("AutoShuffleWindow"));
    FString ResourseDir = FPaths::Combine(*PluginDir, TEXT("Resources"));
    FString FileDir = FPaths::Combine(*ResourseDir, TEXT("Whitelist.json"));
    FString Filedata = "";
    FFileHelper::LoadFileToString(Filedata, *FileDir);
    TSharedPtr<FJsonObject> WhitelistJson = FAutoShuffleWindowModule::ParseJSON(Filedata, TEXT("Whitelist"), false);
    
    // Start reading JsonObject "Whitelist"
    {
        if (!WhitelistJson.IsValid())
        {
            UE_LOG(LogAutoShuffle, Warning, TEXT("Nothing to shuffle. Module quites."));
            return false;
        }
        if (WhitelistJson->HasField("Whitelist"))
        {
            UE_LOG(LogAutoShuffle, Log, TEXT("Reading Whitelist..."));
        }
        else
        {
            UE_LOG(LogAutoShuffle, Warning, TEXT("Whitelist reading failed. Module quites."));
            return false;
        }
    }
    TSharedPtr<FJsonObject> WhitelistObjectJson = WhitelistJson->GetObjectField("Whitelist");
    
    // Start reading JsonArray "Shelves"
    if (WhitelistObjectJson->HasField("Shelves"))
    {
        UE_LOG(LogAutoShuffle, Log, TEXT("Reading Shelves..."));
    }
    else
    {
        UE_LOG(LogAutoShuffle, Warning, TEXT("Shelves reading failed. Module quits."));
        return false;
    }
    TArray<TSharedPtr<FJsonValue>> ShelvesArrayJson = WhitelistObjectJson->GetArrayField("Shelves");
    FAutoShuffleWindowModule::ShelvesWhitelist = new TArray<FAutoShuffleShelf>();
    for (auto JsonValueIt = ShelvesArrayJson.CreateIterator(); JsonValueIt; ++JsonValueIt)
    {
        TSharedPtr<FJsonObject> ShelfObjectJson = (*JsonValueIt)->AsObject();
        FString NewName = ShelfObjectJson->GetStringField("Name");
        AActor* NewObjectActor = nullptr;
        for (TActorIterator<AActor> ActorIt(EditorWorld); ActorIt; ++ActorIt)
        {
            if (ActorIt->GetName().Contains(NewName))
            {
                NewObjectActor = *ActorIt;
#ifdef VERBOSE_AUTO_SHUFFLE
                UE_LOG(LogAutoShuffle, Log, TEXT("Found %s for %s"), *NewObjectActor->GetName(), *NewName);
#endif
                break;
            }
        }
        if (NewObjectActor == nullptr)
        {
#ifdef VERBOSE_AUTO_SHUFFLE
            UE_LOG(LogAutoShuffle, Log, TEXT("Found 0 object for %s"), *NewName);
#endif
            continue;
        }
        float NewScale = ShelfObjectJson->GetNumberField("Scale");
        TArray<float>* NewShelfBase = new TArray<float>();
        TArray<TSharedPtr<FJsonValue>> Shelfbase = ShelfObjectJson->GetArrayField("Shelfbase");
        for (auto BaseValueIt = Shelfbase.CreateIterator(); BaseValueIt; ++BaseValueIt)
        {
            NewShelfBase->Add((*BaseValueIt)->AsNumber());
        }
        FVector NewPosition = NewObjectActor->GetActorLocation();
        ShelvesWhitelist->Add(FAutoShuffleShelf());
        ShelvesWhitelist->Top().SetShelfBase(NewShelfBase);
        ShelvesWhitelist->Top().SetScale(NewScale);
        ShelvesWhitelist->Top().SetName(NewName);
        ShelvesWhitelist->Top().SetObjectActor(NewObjectActor);
        ShelvesWhitelist->Top().SetPosition(NewPosition);
    }
    
#ifdef VERBOSE_AUTO_SHUFFLE
    for (auto ShelfIt = ShelvesWhitelist->CreateIterator(); ShelfIt; ++ShelfIt)
    {
        UE_LOG(LogAutoShuffle, Log, TEXT("Shelf: %s in Scale: %f"), *(ShelfIt->GetName()), ShelfIt->GetScale());
        for (auto BaseIt = ShelfIt->GetShelfBase()->CreateIterator(); BaseIt; ++BaseIt)
        {
            UE_LOG(LogAutoShuffle, Log, TEXT("Shelfbase: %f"), *BaseIt);
        }
    }
#endif
    
    // Start reading JsonArray "Products"
    if (WhitelistObjectJson->HasField("Products"))
    {
        UE_LOG(LogAutoShuffle, Log, TEXT("Reading Products..."));
    }
    else
    {
        UE_LOG(LogAutoShuffle, Warning, TEXT("Products reading failed. Module quits."));
        return false;
    }
    TArray<TSharedPtr<FJsonValue>> ProductsArrayJson = WhitelistObjectJson->GetArrayField("Products");
    FAutoShuffleWindowModule::ProductsWhitelist = new TArray<FAutoShuffleProductGroup>();
    for (auto JsonValueIt = ProductsArrayJson.CreateIterator(); JsonValueIt; ++JsonValueIt)
    {
        TSharedPtr<FJsonObject> ProductObjectJson = (*JsonValueIt)->AsObject();
        FString NewGroupName = ProductObjectJson->GetStringField("GroupName");
        TArray<FAutoShuffleObject>* NewMembers = new TArray<FAutoShuffleObject>();
        TArray<TSharedPtr<FJsonValue>> Members = ProductObjectJson->GetArrayField("Members");
        for (auto MemberValueIt = Members.CreateIterator(); MemberValueIt; ++MemberValueIt)
        {
            TSharedPtr<FJsonObject> ProductMember = (*MemberValueIt)->AsObject();
            FString NewName = ProductMember->GetStringField("Name");
            AActor* NewObjectActor = nullptr;
            for (TActorIterator<AActor> ActorIt(EditorWorld); ActorIt; ++ActorIt)
            {
                if (ActorIt->GetName().Contains(NewName))
                {
                    NewObjectActor = *ActorIt;
#ifdef VERBOSE_AUTO_SHUFFLE
                    UE_LOG(LogAutoShuffle, Log, TEXT("Found %s for %s"), *ActorIt->GetName(), *NewName);
#endif
                    break;
                }
            }
            if (NewObjectActor == nullptr)
            {
#ifdef VERBOSE_AUTO_SHUFFLE
                UE_LOG(LogAutoShuffle, Log, TEXT("Found 0 Object for %s"), *NewName);
#endif
                continue;
            }
            float NewScale = ProductMember->GetNumberField("Scale");
            NewMembers->Add(FAutoShuffleObject());
            NewMembers->Top().SetName(NewName);
            NewMembers->Top().SetScale(NewScale);
            NewMembers->Top().SetObjectActor(NewObjectActor);
        }
        ProductsWhitelist->Add(FAutoShuffleProductGroup());
        ProductsWhitelist->Top().SetName(NewGroupName);
        ProductsWhitelist->Top().SetMembers(NewMembers);
    }
    
#ifdef VERBOSE_AUTO_SHUFFLE
    for (auto GroupIt = ProductsWhitelist->CreateIterator(); GroupIt; ++GroupIt)
    {
        UE_LOG(LogAutoShuffle, Log, TEXT("Product Group: %s"), *(GroupIt->GetName()));
        for (auto ProductIt = GroupIt->GetMembers()->CreateIterator(); ProductIt; ++ProductIt)
        {
            UE_LOG(LogAutoShuffle, Log, TEXT("Product Name: %s in Scale: %f"), *(ProductIt->GetName()), ProductIt->GetScale());
        }
    }
#endif
    
    UE_LOG(LogAutoShuffle, Log, TEXT("Collected %d Shelves and %d Products Group"), ShelvesArrayJson.Num(), ProductsArrayJson.Num());
    
    return true;
}

void FAutoShuffleWindowModule::AddNoiseToShelf(const FString& ShelfName, float NoiseScale)
{
    // Get the first shelf's name and its presumably fixed Position.Z
    if (FAutoShuffleWindowModule::ShelvesWhitelist->Num() < 1)
    {
        UE_LOG(LogAutoShuffle, Warning, TEXT("No shelves loaded."));
        return;
    }
    FAutoShuffleShelf& FirstShelf = FAutoShuffleWindowModule::ShelvesWhitelist->operator[](0);
    FString Name = FirstShelf.GetName();
    FVector RefPosition = FirstShelf.GetPosition();
    if (Name == ShelfName)
    {
        UE_LOG(LogAutoShuffle, Warning, TEXT("Shelf %s is fixed."), *Name);
        return;
    }
    FAutoShuffleShelf* ShelfToChange = nullptr;
    for (auto ShelfIt = ShelvesWhitelist->CreateIterator(); ShelfIt; ++ShelfIt)
    {
        if (ShelfIt->GetName() == ShelfName)
        {
            ShelfToChange = &*ShelfIt;
            break;
        }
    }
    if (ShelfToChange == nullptr)
    {
        UE_LOG(LogAutoShuffle, Warning, TEXT("No shelf matches the name %s"), *ShelfName);
        return;
    }
    FVector NewPosition = ShelfToChange->GetPosition();
    NewPosition.Z = RefPosition.Z - NoiseScale * (float(FMath::Rand()) / RAND_MAX);
    ShelfToChange->SetPosition(NewPosition);
}

TSharedPtr<FJsonObject> FAutoShuffleWindowModule::ParseJSON(const FString& FileContents, const FString& NameForErrors, bool bSilent)
{
    // Parsing source code from:
    // https://github.com/EpicGames/UnrealEngine/blob/master/Engine/Plugins/2D/Paper2D/Source/PaperSpriteSheetImporter/Private/PaperJsonSpriteSheetImporter.cpp
    if (!FileContents.IsEmpty())
    {
        const TSharedRef<TJsonReader<>>& Reader = TJsonReaderFactory<>::Create(FileContents);
        TSharedPtr<FJsonObject> SpriteDescriptorObject;
        if (FJsonSerializer::Deserialize(Reader, SpriteDescriptorObject) && SpriteDescriptorObject.IsValid())
        {
            // File was loaded and deserialized OK!
            return SpriteDescriptorObject;
        }
        else
        {
            if (!bSilent)
            {
                UE_LOG(LogAutoShuffle, Warning, TEXT("Faled to parse sprite descriptor file '%s'. Error: '%s'"), *NameForErrors, *Reader->GetErrorMessage());
            }
            return nullptr;
        }
    }
    else
    {
        if (!bSilent)
        {
            UE_LOG(LogAutoShuffle, Warning, TEXT("Sprite descriptor file '%s' was empty. This sprite cannot be imported."), *NameForErrors);
        }
        return nullptr;
    }
}

void FAutoShuffleWindowModule::PlaceProducts(float Density, float Proxmity)
{
    /** Placing the products to the shelf
     * @param Density: How many products are considerd 0 ~ 1 multiplied by all the stuffs
     * @param Proxmity: How close the items in the same group are placed 0: one on another 1: randomly placed
     * @note density and proxmity are w.r.t. one shelf.
     * @note The shelf must be aligned with x, y, and z, and y is the longest side of the shelf
     * @todo Consider two-side placing and product-shelf associations
     */
    
    // iterate through shelves
    for (auto ShelfIt = FAutoShuffleWindowModule::ShelvesWhitelist->CreateIterator(); ShelfIt; ++ShelfIt)
    {
        // find the bounding box of the shelf and the longest between x and y as the places for products to enter from
        FVector BoundingBoxOrigin, BoundingBoxExtent;
        ShelfIt->GetObjectActor()->GetActorBounds(false, BoundingBoxOrigin, BoundingBoxExtent);
#ifdef VERBOSE_AUTO_SHUFFLE
        UE_LOG(LogAutoShuffle, Log, TEXT("Shelf %s has bounding box %s, %s"), *ShelfIt->GetName(), *BoundingBoxOrigin.ToString(), *BoundingBoxExtent.ToString());
#endif
        // find the Z-values of shelf bases
        TArray<float> ShelfBaseZ;
        for (auto ShelfBaseIt = ShelfIt->GetShelfBase()->CreateIterator(); ShelfBaseIt; ++ShelfBaseIt)
        {
            ShelfBaseZ.Add(*ShelfBaseIt * BoundingBoxExtent.Z * 2.f + BoundingBoxOrigin.Z - BoundingBoxExtent.Z);
        }
#ifdef VERBOSE_AUTO_SHUFFLE
        for (auto ShelfBaseIt = ShelfBaseZ.CreateIterator(); ShelfBaseIt; ++ShelfBaseIt)
        {
            UE_LOG(LogAutoShuffle, Log, TEXT("The real Z values of shelf %s: %f"), *ShelfIt->GetName(), *ShelfBaseIt);
        }
#endif
        // iterate through all the product groups
        for (auto ProductGroupIt = ProductsWhitelist->CreateIterator(); ProductGroupIt; ++ProductGroupIt)
        {
            // get a centerilized anchor for placing products
            int ShelfBaseIdx = FMath::RandRange(0, ShelfBaseZ.Num() - 1);
            FVector Anchor;
            Anchor.Z = ShelfBaseZ[ShelfBaseIdx];
            Anchor.Y = FMath::RandRange(float(BoundingBoxOrigin.Y - BoundingBoxExtent.Y + AUTO_SHUFFLE_Y_TWO_END_OFFSET), float(BoundingBoxOrigin.Y + BoundingBoxExtent.Y - AUTO_SHUFFLE_Y_TWO_END_OFFSET));
            // iterate through all the products within the current group
            for (auto ProductIt = ProductGroupIt->GetMembers()->CreateIterator(); ProductIt; ++ProductIt)
            {
                // if rand() <= Density, select
                if (FMath::RandRange(0.f, 1.f) > Density)
                {
                    continue;
                }
                // if rand() >= Proxmity place it randomly
                if (FMath::RandRange(0.f, 1.f) >= Proxmity)
                {
                    // randomly get a start point on the boundary of the shelf; if collided get another one
                    FVector ProductStartPoint;
                    int AlreadyTriedTimes = 0;
                    TArray<AActor*> OverlappingActors;
                    while (true)
                    {
                        if (AlreadyTriedTimes >= AUTO_SHUFFLE_MAX_TRY_TIMES)
                        {
                            AlreadyTriedTimes = -1;
                            break;
                        }
                        AlreadyTriedTimes += 1;
                        int ProductStartPointShelfBaseIdx = FMath::RandRange(0, ShelfBaseZ.Num() - 1);
                        ProductStartPoint.Z = ShelfBaseZ[ProductStartPointShelfBaseIdx];
                        ProductStartPoint.Y = FMath::RandRange(float(BoundingBoxOrigin.Y - BoundingBoxExtent.Y + AUTO_SHUFFLE_Y_TWO_END_OFFSET), float(BoundingBoxOrigin.Y + BoundingBoxExtent.Y - AUTO_SHUFFLE_Y_TWO_END_OFFSET));
                        ProductStartPoint.X = BoundingBoxOrigin.X - BoundingBoxExtent.X;
                        ProductIt->SetPosition(ProductStartPoint);
                        // deal with the offset of the product center and the bottom
                        FVector ProductOrigin, ProductExtent;
                        ProductIt->GetObjectActor()->GetActorBounds(false, ProductOrigin, ProductExtent);
                        float ProductCurrentBottom = ProductOrigin.Z - ProductExtent.Z;
                        float ProductZLift = ProductStartPoint.Z - ProductCurrentBottom;
                        ProductStartPoint.Z += ProductZLift;
                        ProductIt->SetPosition(ProductStartPoint);
                        // find all the overlapped actors
                        ProductIt->GetObjectActor()->GetOverlappingActors(OverlappingActors);
                        UE_LOG(LogAutoShuffle, Log, TEXT("%s has %d overlapping actors"), *ProductIt->GetName(), OverlappingActors.Num());
                        /** @todo consider implementing a collision whitelist, e.g., BP_DemoRoom */
                        if (/** no collision */ OverlappingActors.Num() == 0)
                        {
                            break;
                        }
#ifdef VERBOSE_AUTO_SHUFFLE
                        for (auto OverlappingActorIt = OverlappingActors.CreateIterator(); OverlappingActorIt; ++OverlappingActorIt)
                        {
                            UE_LOG(LogAutoShuffle, Log, TEXT("%s is overlapping with %s"), *ProductIt->GetName(), *(*OverlappingActorIt)->GetName());
                        }
#endif
                    }
                    // try to push the item inside, until collided
                    AlreadyTriedTimes = 0;
                    while (AlreadyTriedTimes++ < AUTO_SHUFFLE_INC_BOUND)
                    {
                        ProductIt->GetObjectActor()->GetOverlappingActors(OverlappingActors);
                        FVector ProductPosition = ProductIt->GetObjectActor()->GetActorLocation();
                        if (OverlappingActors.Num() != 0)
                        {
                            ProductPosition.X -= AUTO_SHUFFLE_INC_STEP;
                            ProductIt->SetPosition(ProductPosition);
                            break;
                        }
                        ProductPosition.X += AUTO_SHUFFLE_INC_STEP;
                        ProductIt->SetPosition(ProductPosition);
                    }
                }
                // else place it near the anchor
                else
                {
                    
                }
            }
        }
    }
}

FAutoShuffleObject::FAutoShuffleObject()
{
    Scale = 1.f;
    Name = TEXT("Uninitialized Object Name");
    Position = FVector(0.f, 0.f, 0.f);
    Rotation = FVector(0.f, 0.f, 0.f);
    ObjectActor = nullptr;
}

FAutoShuffleObject::~FAutoShuffleObject()
{
}

void FAutoShuffleObject::SetName(FString& NewName)
{
    Name = NewName;
}

FString FAutoShuffleObject::GetName() const
{
    return Name;
}

void FAutoShuffleObject::SetPosition(FVector& NewPosition)
{
    Position = NewPosition;
    ObjectActor->SetActorLocation(Position);
}

FVector FAutoShuffleObject::GetPosition() const
{
    return Position;
}

void FAutoShuffleObject::SetRotation(FVector& NewRotation)
{
    Rotation = NewRotation;
}

FVector FAutoShuffleObject::GetRotation() const
{
    return Rotation;
}

void FAutoShuffleObject::SetScale(float NewScale)
{
    Scale = NewScale;
}

float FAutoShuffleObject::GetScale() const
{
    return Scale;
}

void FAutoShuffleObject::SetObjectActor(AActor* NewObjectActor)
{
    // We don't delete anything
    ObjectActor = NewObjectActor;
}

AActor* FAutoShuffleObject::GetObjectActor() const
{
    return ObjectActor;
}

FAutoShuffleShelf::FAutoShuffleShelf() : FAutoShuffleObject()
{
    ShelfBase = nullptr;
}

FAutoShuffleShelf::~FAutoShuffleShelf()
{
    if (ShelfBase != nullptr)
    {
        delete ShelfBase;
    }
}

void FAutoShuffleShelf::SetShelfBase(TArray<float>* NewShelfBase)
{
    if (ShelfBase != nullptr)
    {
        delete ShelfBase;
    }
    ShelfBase = NewShelfBase;
}

TArray<float>* FAutoShuffleShelf::GetShelfBase() const
{
    return ShelfBase;
}

FAutoShuffleProductGroup::FAutoShuffleProductGroup()
{
    Members = nullptr;
}

FAutoShuffleProductGroup::~FAutoShuffleProductGroup()
{
    if (Members != nullptr)
    {
        delete Members;
    }
}

void FAutoShuffleProductGroup::SetMembers(TArray<FAutoShuffleObject>* NewMembers)
{
    if (Members != nullptr)
    {
        delete Members;
    }
    Members = NewMembers;
}

TArray<FAutoShuffleObject>* FAutoShuffleProductGroup::GetMembers() const
{
    return Members;
}

void FAutoShuffleProductGroup::SetName(FString& NewName)
{
    Name = NewName;
}

FString FAutoShuffleProductGroup::GetName() const
{
    return Name;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAutoShuffleWindowModule, AutoShuffleWindow)