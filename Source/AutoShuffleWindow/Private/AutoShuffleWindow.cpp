// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AutoShuffleWindowPrivatePCH.h"

#include "SlateBasics.h"
#include "SlateExtras.h"

#include "AutoShuffleWindowStyle.h"
#include "AutoShuffleWindowCommands.h"

#include "LevelEditor.h"

/** The following header files are not from the template */
#include "Json.h"
#include "Developer/RawMesh/Public/RawMesh.h"
// the following header files for convex decomposition
#include "Runtime/Engine/Public/StaticMeshResources.h"
#include "BusyCursor.h"
#include "Runtime/Engine/Classes/PhysicsEngine/BodySetup.h"
#include "Editor/UnrealEd/Private/ConvexDecompTool.h"
#include "Editor/UnrealEd/Private/GeomFitUtils.h"

static const FName AutoShuffleWindowTabName("AutoShuffleWindow");

#define LOCTEXT_NAMESPACE "FAutoShuffleWindowModule"
DEFINE_LOG_CATEGORY(LogAutoShuffle);

// #define VERBOSE_AUTO_SHUFFLE
#define AUTO_SHUFFLE_Y_TWO_END_OFFSET 10.f
#define AUTO_SHUFFLE_MAX_TRY_TIMES 50
#define AUTO_SHUFFLE_INC_STEP 0.1f
#define AUTO_SHUFFLE_INC_BOUND 1000
#define AUTO_SHUFFLE_EXPANSION_BOUND 20

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
    OcclusionSpinBox = SNew(SSpinBox<float>);
    OcclusionSpinBox->SetMinValue(0.f);
    OcclusionSpinBox->SetMaxValue(1.f);
    OcclusionSpinBox->SetMinSliderValue(0.f);
    OcclusionSpinBox->SetMaxSliderValue(1.f);
    OcclusionSpinBox->SetValue(0.9f);
    
    // init or re-init the checkboxes
    OrganizeCheckBox = SNew(SCheckBox);

    // set the region of the discarded products to origin
    DiscardedProductsRegions = FVector(0.f, 0.f, 0.f);
    
    TSharedRef<SButton> AutoShuffleButton = SNew(SButton);
    AutoShuffleButton->SetVAlign(VAlign_Center);
    AutoShuffleButton->SetHAlign(HAlign_Center);
    AutoShuffleButton->SetContent(SNew(STextBlock).Text(FText::FromString(TEXT("Auto Shuffle"))));

    TSharedRef<SButton> OcclusionVisibilityButton = SNew(SButton);
    OcclusionVisibilityButton->SetVAlign(VAlign_Center);
    OcclusionVisibilityButton->SetHAlign(HAlign_Center);
    OcclusionVisibilityButton->SetContent(SNew(STextBlock).Text(FText::FromString(TEXT("Generate Occlusion Description File"))));
    
    TSharedRef<SButton> BatchConvexDecompButton = SNew(SButton);
    BatchConvexDecompButton->SetVAlign(VAlign_Center);
    BatchConvexDecompButton->SetHAlign(HAlign_Center);
    BatchConvexDecompButton->SetContent(SNew(STextBlock).Text(FText::FromString(TEXT("Batch Convex Decomposition"))));
    
    TSharedRef<SButton> NonProductsVisibleToggleButton = SNew(SButton);
    NonProductsVisibleToggleButton->SetVAlign(VAlign_Center);
    NonProductsVisibleToggleButton->SetHAlign(HAlign_Center);
    NonProductsVisibleToggleButton->SetContent(SNew(STextBlock).Text(FText::FromString(TEXT("Toggle Non Products Visibility"))));
    bIsNonProductsVisible = true;

    TSharedRef<SButton> ExportActorNameMappingButton = SNew(SButton);
    ExportActorNameMappingButton->SetVAlign(VAlign_Center);
    ExportActorNameMappingButton->SetHAlign(HAlign_Center);
    ExportActorNameMappingButton->SetContent(SNew(STextBlock).Text(FText::FromString(TEXT("Export Actor Name Mapping"))));

    auto OnAutoShuffleButtonClickedLambda = []() -> FReply
    {
        AutoShuffleImplementation();
        return FReply::Handled();
    };

    auto OnOcclusionVisibilityButtonClickedLambda = []() -> FReply
    {
        OcclusionVisibilityImplementation();
        return FReply::Handled();
    };

    auto OnBatchConvexDecompButtonClickedLambda = []() -> FReply
    {
        BatchConvexDecomposition();
        return FReply::Handled();
    };

    auto OnNonProductsVisibleToggleButtonClickedLamda = []() -> FReply
    {
        bIsNonProductsVisible = !bIsNonProductsVisible;
        UE_LOG(LogAutoShuffle, Log, TEXT("Non Product Visibility Set to %d"), bIsNonProductsVisible);
        NonProductsVisibilityTogglingImplementation();
        return FReply::Handled();
    };

    auto OnExportActorNameMappingButtonClickedLambda = []() -> FReply
    {
        ExportMappingBetweenActorIdAndDisplayName();
        return FReply::Handled();
    };
    
    AutoShuffleButton->SetOnClicked(FOnClicked::CreateLambda(OnAutoShuffleButtonClickedLambda));
    OcclusionVisibilityButton->SetOnClicked(FOnClicked::CreateLambda(OnOcclusionVisibilityButtonClickedLambda));
    BatchConvexDecompButton->SetOnClicked(FOnClicked::CreateLambda(OnBatchConvexDecompButtonClickedLambda));
    NonProductsVisibleToggleButton->SetOnClicked(FOnClicked::CreateLambda(OnNonProductsVisibleToggleButtonClickedLamda));
    ExportActorNameMappingButton->SetOnClicked(FOnClicked::CreateLambda(OnExportActorNameMappingButtonClickedLambda));
    FText Density = FText::FromString(TEXT("Density      "));
    FText Proxmity = FText::FromString(TEXT("Proxmity   "));
    FText Organize = FText::FromString(TEXT("Organize   "));
    FText PerGroup = FText::FromString(TEXT("PerGroup   "));
    FText OcclusionThreshold = FText::FromString(TEXT("OccThres   "));
    
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
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center).AutoWidth()
            [
                SNew(STextBlock).Text(Organize)
            ]
            + SHorizontalBox::Slot().HAlign(HAlign_Fill)
            [
                OrganizeCheckBox
            ]
            + SHorizontalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center).AutoWidth()
            [
                SNew(STextBlock).Text(PerGroup)
            ]
            + SHorizontalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center).AutoWidth()
            [
                PerGroupCheckBox
            ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(30.f, 10.f)
        [
            AutoShuffleButton
        ]
        + SVerticalBox::Slot().Padding(30.f, 10.f).AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center).AutoWidth()
            [
                SNew(STextBlock).Text(OcclusionThreshold)
            ]
            + SHorizontalBox::Slot().HAlign(HAlign_Fill)
            [
                OcclusionSpinBox
            ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(30.f, 10.f)
        [
            OcclusionVisibilityButton
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(30.f, 10.f)
        [
            BatchConvexDecompButton
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(30.f, 10.f)
        [
            NonProductsVisibleToggleButton
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(30.f, 10.f)
        [
            ExportActorNameMappingButton
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
TSharedRef<SSpinBox<float>> FAutoShuffleWindowModule::OcclusionSpinBox = SNew(SSpinBox<float>);
TSharedRef<SCheckBox> FAutoShuffleWindowModule::OrganizeCheckBox = SNew(SCheckBox);
TSharedRef<SCheckBox> FAutoShuffleWindowModule::PerGroupCheckBox = SNew(SCheckBox);
TArray<FAutoShuffleShelf>* FAutoShuffleWindowModule::ShelvesWhitelist = nullptr;
TArray<FAutoShuffleProductGroup>* FAutoShuffleWindowModule::ProductsWhitelist = nullptr;
FVector FAutoShuffleWindowModule::DiscardedProductsRegions;
bool FAutoShuffleWindowModule::bIsOrganizeChecked;
bool FAutoShuffleWindowModule::bIsPerGroupChecked;
bool FAutoShuffleWindowModule::bIsNonProductsVisible;

// the rendering device: a very big two-dimensional tarray of index of actors, and depth
TArray<class FOcclusionPixel> FAutoShuffleWindowModule::RenderingDevice[OCCLUSION_VISIBILITY_RESOLUTION_HEIGHT][OCCLUSION_VISIBILITY_RESOLUTION_WIDTH];

void FAutoShuffleWindowModule::AutoShuffleImplementation()
{
    float Density = FAutoShuffleWindowModule::DensitySpinBox->GetValue();
    float Proxmity = FAutoShuffleWindowModule::ProxmitySpinBox->GetValue();
    bIsOrganizeChecked = FAutoShuffleWindowModule::OrganizeCheckBox->IsChecked();
    bIsPerGroupChecked = FAutoShuffleWindowModule::PerGroupCheckBox->IsChecked();
    bool Result = FAutoShuffleWindowModule::ReadWhitelist();
    if (!Result)
    {
        UE_LOG(LogAutoShuffle, Warning, TEXT("Whitelist read wrong. Module quits."));
        return;
    }
    // Activate, put aside and shrink all the Products
    for (auto ProductGroupIt = ProductsWhitelist->CreateIterator(); ProductGroupIt; ++ProductGroupIt)
    {
        for (auto ProductIt = ProductGroupIt->GetMembers()->CreateIterator(); ProductIt; ++ProductIt)
        {
            ProductIt->ResetDiscard();
            ProductIt->ResetOnShelf();
            ProductIt->SetPosition(DiscardedProductsRegions);
            ProductIt->ShrinkScale();
        }
    }
    // AddNoiseToShelf("BP_ShelfMain_002", 50);
    PlaceProducts(Density, Proxmity);
    // Expand all the Products
    for (int ExpendIdx = 0; ExpendIdx < AUTO_SHUFFLE_EXPANSION_BOUND; ++ExpendIdx)
    {
        for (auto ProductGroupIt = ProductsWhitelist->CreateIterator(); ProductGroupIt; ++ProductGroupIt)
        {
            for (auto ProductIt = ProductGroupIt->GetMembers()->CreateIterator(); ProductIt; ++ProductIt)
            {
                if (!ProductIt->IsDiscarded())
                {
                    ProductIt->ExpandScale();
                }
            }
        }
    }
    if (bIsOrganizeChecked)
    {
        for (int i = 0; i < 2; ++i)
        {
            OrganizeProducts();
        }
    }
    LowerProducts();
}

void FAutoShuffleWindowModule::OcclusionVisibilityImplementation()
{
    UE_LOG(LogAutoShuffle, Log, TEXT("Set Occlusion Visibility"));
    float OcclusionThreshold = FAutoShuffleWindowModule::OcclusionSpinBox->GetValue();
    bool Result = FAutoShuffleWindowModule::ReadWhitelist();
    if (!Result)
    {
        UE_LOG(LogAutoShuffle, Warning, TEXT("Whitelist read wrong. Module quits."));
        return;
    }
    // find the boundary of the shelves
    // pre-assumptions: looking from small x to big x, and within 1e10 scale
    float RenderingBorderXLeft = 1e10f, RenderingBorderXRight = -1e10f,
        RenderingBorderYLeft = 1e10f, RenderingBorderYRight = -1e10f,
        RenderingBorderZLeft = 1e10f, RenderingBorderZRight = -1e10f;
    for (auto ShelfIt = ShelvesWhitelist->CreateIterator(); ShelfIt; ++ShelfIt)
    {
        FVector ShelfOrigin, ShelfExtent;
        ShelfIt->GetObjectActor()->GetActorBounds(false, ShelfOrigin, ShelfExtent);
        RenderingBorderXLeft = FMath::Min(RenderingBorderXLeft, ShelfOrigin.X - ShelfExtent.X);
        RenderingBorderXRight = FMath::Max(RenderingBorderXRight, ShelfOrigin.X + ShelfExtent.X);
        RenderingBorderYLeft = FMath::Min(RenderingBorderYLeft, ShelfOrigin.Y - ShelfExtent.Y);
        RenderingBorderYRight = FMath::Max(RenderingBorderYRight, ShelfOrigin.Y + ShelfExtent.Y);
        RenderingBorderZLeft = FMath::Min(RenderingBorderZLeft, ShelfOrigin.Z - ShelfExtent.Z);
        RenderingBorderZRight = FMath::Max(RenderingBorderZRight, ShelfOrigin.Z + ShelfExtent.Z);
    }
    UE_LOG(LogAutoShuffle, Log, TEXT("Valid Boundary: %f, %f, %f, %f, %f, %f"), RenderingBorderXLeft, RenderingBorderXRight, RenderingBorderYLeft, RenderingBorderYRight, RenderingBorderZLeft, RenderingBorderZRight);
    // clean the occlusion visibility rendering device
    for (int y = 0; y < OCCLUSION_VISIBILITY_RESOLUTION_HEIGHT; ++y)
    {
        for (int x = 0; x < OCCLUSION_VISIBILITY_RESOLUTION_WIDTH; ++x)
        {
            RenderingDevice[y][x].Empty();
        }
    }
    // gather all the valid static mesh actors
    TArray<AStaticMeshActor*> ActorArray;
    TArray<FString> ActorNameArray;
    for (auto GroupIt = ProductsWhitelist->CreateIterator(); GroupIt; ++GroupIt)
    {
        for (auto ProductIt = GroupIt->GetMembers()->CreateIterator(); ProductIt; ++ProductIt)
        {
            // see if the product is within the border by testing the product center
            FVector ProductOrigin, ProductExtent;
            if (!ProductIt->GetObjectActor())
            {
                continue;
            }
            AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(ProductIt->GetObjectActor());
            ProductIt->GetObjectActor()->GetActorBounds(false, ProductOrigin, ProductExtent);
            if (ProductOrigin.X > RenderingBorderXRight || ProductOrigin.X < RenderingBorderXLeft ||
                ProductOrigin.Y > RenderingBorderYRight || ProductOrigin.Y < RenderingBorderYLeft ||
                /** ProductOrigin.Z > RenderingBorderZRight || */ ProductOrigin.Z < RenderingBorderZLeft)
            {
                continue;
            }
            if (StaticMeshActor)
            {
                ActorArray.Add(StaticMeshActor);
                ActorNameArray.Add(ProductIt->GetName());
            }
        }
    }
    // Get all the meshes and draw them on the rendering device
    // Reference: https://forums.unrealengine.com/showthread.php?8856-Accessing-Vertex-Positions-of-static-mesh
    // Reference: https://answers.unrealengine.com/questions/465376/access-to-mesh-data-in-object-via-c.html
    // @todo Parallelable
    UE_LOG(LogAutoShuffle, Log, TEXT("Start rendering %d static meshes"), ActorArray.Num());
    FDateTime DateTime;
    UE_LOG(LogAutoShuffle, Log, TEXT("%d: %d: %d starts"), DateTime.Now().GetMinute(), DateTime.Now().GetSecond(), DateTime.Now().GetMillisecond());
    for (int ActorIdx = 0; ActorIdx < ActorArray.Num(); ++ActorIdx)
    {
        if (!ActorArray[ActorIdx]->GetStaticMeshComponent())
        {
            continue;
        }
        if (!ActorArray[ActorIdx]->GetStaticMeshComponent()->StaticMesh)
        {
            continue;
        }
        if (ActorArray[ActorIdx]->GetStaticMeshComponent()->StaticMesh->SourceModels.Num() == 0)
        {
            continue;
        }
        FRawMesh RawMesh;
        ActorArray[ActorIdx]->GetStaticMeshComponent()->StaticMesh->SourceModels[0].RawMeshBulkData->LoadRawMesh(RawMesh);
        TArray<FVector> Vertices = RawMesh.VertexPositions;
        TArray<uint32> Wedges = RawMesh.WedgeIndices;
        // Assumption: this is a triangle mesh; otherwise, don't know how to do
        for (int WedgeIdx = 0; 3 * WedgeIdx < Wedges.Num(); ++WedgeIdx)
        {
            FVector Vec1 = ActorArray[ActorIdx]->GetActorLocation() + ActorArray[ActorIdx]->GetTransform().TransformVector(Vertices[Wedges[3 * WedgeIdx + 0]]);
            FVector Vec2 = ActorArray[ActorIdx]->GetActorLocation() + ActorArray[ActorIdx]->GetTransform().TransformVector(Vertices[Wedges[3 * WedgeIdx + 1]]);
            FVector Vec3 = ActorArray[ActorIdx]->GetActorLocation() + ActorArray[ActorIdx]->GetTransform().TransformVector(Vertices[Wedges[3 * WedgeIdx + 2]]);
            
            F2DPointf Pointf1(
                (Vec1.Y - RenderingBorderYLeft) / (RenderingBorderYRight - RenderingBorderYLeft) * OCCLUSION_VISIBILITY_RESOLUTION_WIDTH,
                (Vec1.Z - RenderingBorderZLeft) / (RenderingBorderZRight - RenderingBorderZLeft) * OCCLUSION_VISIBILITY_RESOLUTION_HEIGHT,
                Vec1.X
            ), Pointf2(
                (Vec2.Y - RenderingBorderYLeft) / (RenderingBorderYRight - RenderingBorderYLeft) * OCCLUSION_VISIBILITY_RESOLUTION_WIDTH,
                (Vec2.Z - RenderingBorderZLeft) / (RenderingBorderZRight - RenderingBorderZLeft) * OCCLUSION_VISIBILITY_RESOLUTION_HEIGHT,
                Vec2.X
            ), Pointf3(
                (Vec3.Y - RenderingBorderYLeft) / (RenderingBorderYRight - RenderingBorderYLeft) * OCCLUSION_VISIBILITY_RESOLUTION_WIDTH,
                (Vec3.Z - RenderingBorderZLeft) / (RenderingBorderZRight - RenderingBorderZLeft) * OCCLUSION_VISIBILITY_RESOLUTION_HEIGHT,
                Vec3.X
            );
            TArray<F2DPoint> *RenderingPoints = TriangleRasterizer(Pointf1, Pointf2, Pointf3);
            if (RenderingPoints->Num() > 0)
            {
                // Render them to the device
                for (auto PointIt = RenderingPoints->CreateIterator(); PointIt; ++PointIt)
                {
                    if (PointIt->X < 0 || PointIt->X >= OCCLUSION_VISIBILITY_RESOLUTION_WIDTH ||
                        PointIt->Y < 0 || PointIt->Y >= OCCLUSION_VISIBILITY_RESOLUTION_HEIGHT)
                    {
                        continue;
                    }
                    RenderingDevice[PointIt->Y][PointIt->X].Add(FOcclusionPixel(ActorIdx, PointIt->Z));
                }
            }
            delete RenderingPoints;
            RenderingPoints = TriangleRasterizer(Pointf1, Pointf3, Pointf2);
            if (RenderingPoints->Num() > 0)
            {
                // Render them to the device
                for (auto PointIt = RenderingPoints->CreateIterator(); PointIt; ++PointIt)
                {
                    if (PointIt->X < 0 || PointIt->X >= OCCLUSION_VISIBILITY_RESOLUTION_WIDTH ||
                        PointIt->Y < 0 || PointIt->Y >= OCCLUSION_VISIBILITY_RESOLUTION_HEIGHT)
                    {
                        continue;
                    }
                    RenderingDevice[PointIt->Y][PointIt->X].Add(FOcclusionPixel(ActorIdx, PointIt->Z));
                }
            }
            delete RenderingPoints;
        }
    }
    UE_LOG(LogAutoShuffle, Log, TEXT("%d: %d: %d ends"), DateTime.Now().GetMinute(), DateTime.Now().GetSecond(), DateTime.Now().GetMillisecond());
    // Organize the pixels: one product can only have one depth at one pixel
    // the smallest (because we are looking from small to big) product is visible; others are not
    int *VisiblePixelCount = new int[ActorArray.Num()]();
    int *TotalPixelCount = new int[ActorArray.Num()]();
    for (int y = 0; y < OCCLUSION_VISIBILITY_RESOLUTION_HEIGHT; ++y)
    {
        for (int x = 0; x < OCCLUSION_VISIBILITY_RESOLUTION_WIDTH; ++x)
        {
            TArray<FOcclusionPixel> &PixelArray = RenderingDevice[y][x];
            if (PixelArray.Num() == 0)
            {
                continue;
            }
            // Sort the pixel array according to depth (from small to big)
            PixelArray.Sort(FOcclusionPixel::OrganizePixelPredicateLowToHigh);
            VisiblePixelCount[PixelArray[0].ActorIdx] += 1;
            TArray<int> RelatedActorIdxArray;
            for (int PixelIdx = 0; PixelIdx < PixelArray.Num(); ++PixelIdx)
            {
                int RelatedActorIdx = PixelArray[PixelIdx].ActorIdx, Dummy;
                bool ActorIdxFound = RelatedActorIdxArray.Find(RelatedActorIdx, Dummy);
                if (ActorIdxFound == false)
                {
                    RelatedActorIdxArray.Add(RelatedActorIdx);
                }
            }
            for (auto RelatedActorIt = RelatedActorIdxArray.CreateIterator(); RelatedActorIt; ++RelatedActorIt)
            {
                TotalPixelCount[*RelatedActorIt] += 1;
            }
        }
    }
    for (int ActorIdx = 0; ActorIdx < ActorArray.Num(); ++ActorIdx)
    {
        if ((VisiblePixelCount[ActorIdx] + 0.f) / TotalPixelCount[ActorIdx] < OcclusionThreshold)
        {
            ActorArray[ActorIdx]->SetActorHiddenInGame(true);
        }
        else
        {
            ActorArray[ActorIdx]->SetActorHiddenInGame(false);
        }
    }
    delete[] VisiblePixelCount;
    delete[] TotalPixelCount;
}

void FAutoShuffleWindowModule::BatchConvexDecomposition()
{
    // Batch convex decomposition on every product
    // Reference: https://github.com/EpicGames/UnrealEngine/blob/55c9f3ba0010e2e483d49a4cd378f36a46601fad/Engine/Source/Editor/StaticMeshEditor/Private/StaticMeshEditor.cpp#L1625
    UE_LOG(LogAutoShuffle, Log, TEXT("Start Batch Convex Decomposition"));
    ReadWhitelist();
    float InAccuracy = 1.f;
    int32 InMaxHullVerts = 32;
    for (auto GroupIt = ProductsWhitelist->CreateIterator(); GroupIt; ++GroupIt)
    {
        for (auto ProductIt = GroupIt->GetMembers()->CreateIterator(); ProductIt; ++ProductIt)
        {
            AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(ProductIt->GetObjectActor());
            if (!StaticMeshActor)
            {
                continue;
            }
            if (!StaticMeshActor->GetStaticMeshComponent())
            {
                continue;
            }
            if (!StaticMeshActor->GetStaticMeshComponent()->StaticMesh)
            {
                continue;
            }
            if (!StaticMeshActor->GetStaticMeshComponent()->StaticMesh->RenderData)
            {
                continue;
            }
            FStaticMeshLODResources &LODModel = StaticMeshActor->GetStaticMeshComponent()->StaticMesh->RenderData->LODResources[0];
            // Start a busy cursor so the user has feedback while waiting
            const FScopedBusyCursor BusyCursor;
            // make vertex buffer
            int32 NumVerts = LODModel.VertexBuffer.GetNumVertices();
            TArray<FVector> Verts;
            for (int32 VertIdx = 0; VertIdx < NumVerts; ++VertIdx)
            {
                FVector Vert = LODModel.PositionVertexBuffer.VertexPosition(VertIdx);
                Verts.Add(Vert);
            }
            // grab all indices
            TArray<uint32> AllIndices;
            LODModel.IndexBuffer.GetCopy(AllIndices);
            // only copy indices that have collision enabled
            TArray<uint32> CollidingIndices;
            for (const FStaticMeshSection& Section : LODModel.Sections)
            {
                if (Section.bEnableCollision)
                {
                    for (uint32 IndexIdx = Section.FirstIndex; IndexIdx < Section.FirstIndex + (Section.NumTriangles * 3); IndexIdx++)
                    {
                        CollidingIndices.Add(AllIndices[IndexIdx]);
                    }
                }
            }
            // get the bodysetup we are going to put the collision into
            UBodySetup *BodySetup = StaticMeshActor->GetStaticMeshComponent()->StaticMesh->BodySetup;
            if (BodySetup)
            {
                BodySetup->RemoveSimpleCollision();
            }
            else
            {
                // otherwise, create one here.
                StaticMeshActor->GetStaticMeshComponent()->StaticMesh->CreateBodySetup();
                BodySetup = StaticMeshActor->GetStaticMeshComponent()->StaticMesh->BodySetup;
            }
            // run actual util to do the work (if we have some valid input)
            if (Verts.Num() >= 3 && CollidingIndices.Num() > 3)
            {
                DecomposeMeshToHulls(BodySetup, Verts, CollidingIndices, InAccuracy, InMaxHullVerts);
            }
            // refresh collision change back to static mesh components
            RefreshCollisionChange(StaticMeshActor->GetStaticMeshComponent()->StaticMesh);
            // mark mesh as dirty
            StaticMeshActor->GetStaticMeshComponent()->StaticMesh->MarkPackageDirty();
            // mark the static mesh for collision customization
            StaticMeshActor->GetStaticMeshComponent()->StaticMesh->bCustomizedCollision = true;
        }
    }
}

void FAutoShuffleWindowModule::NonProductsVisibilityTogglingImplementation()
{
    auto EditorWorld = GEditor->GetEditorWorldContext().World();
    if (bIsNonProductsVisible == true)
    {
        for (TActorIterator<AActor> ActorIt(EditorWorld); ActorIt; ++ActorIt)
        {
            ActorIt->SetActorHiddenInGame(false);
        }
    }
    else
    {
        ReadWhitelist();
        for (TActorIterator<AActor> ActorIt(EditorWorld); ActorIt; ++ActorIt)
        {
            ActorIt->SetActorHiddenInGame(true);
        }
        for (auto GroupIt = ProductsWhitelist->CreateIterator(); GroupIt; ++GroupIt)
        {
            for (auto ProductIt = GroupIt->GetMembers()->CreateIterator(); ProductIt; ++ProductIt)
            {
                if (ProductIt->GetObjectActor())
                {
                    FVector ProductLocation = ProductIt->GetObjectActor()->GetActorLocation();
                    if (FMath::Abs(ProductLocation.X - DiscardedProductsRegions.X) > 0.1 ||
                        FMath::Abs(ProductLocation.Y - DiscardedProductsRegions.Y) > 0.1 ||
                        FMath::Abs(ProductLocation.Z - DiscardedProductsRegions.Z) > 0.1)
                    {
                        ProductIt->GetObjectActor()->SetActorHiddenInGame(false);
                    }
                }
            }
        }
    }
}

void FAutoShuffleWindowModule::ExportMappingBetweenActorIdAndDisplayName()
{
    FString MappingFileDir = FPaths::Combine(*FPaths::GameDir(), *FString("Data"), *FString("ActorNameMapping.csv"));
    auto EditorWorld = GEditor->GetEditorWorldContext().World();
    FString FileContent = "";
    for (TActorIterator<AActor> ActorIt(EditorWorld); ActorIt; ++ActorIt)
    {
        FString ActorID = ActorIt->GetName(), ActorLabel = ActorIt->GetActorLabel();
        FileContent = FileContent + FString::Printf(TEXT("%s,%s\n"), *ActorID, *ActorLabel);
    }
    FFileHelper::SaveStringToFile(FileContent, *MappingFileDir);
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
            if (ActorIt->GetActorLabel() == NewName)
            {
                NewObjectActor = *ActorIt;
#ifdef VERBOSE_AUTO_SHUFFLE
                UE_LOG(LogAutoShuffle, Log, TEXT("Found %s for %s"), *NewObjectActor->GetActorLabel(), *NewName);
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
        TArray<float>* NewShelfOffset = new TArray<float>();
        TArray<TSharedPtr<FJsonValue>> Shelfoffset = ShelfObjectJson->GetArrayField("Shelfoffset");
        for (auto OffsetValueIt = Shelfoffset.CreateIterator(); OffsetValueIt; ++OffsetValueIt)
        {
            NewShelfOffset->Add((*OffsetValueIt)->AsNumber());
        }
        FVector NewPosition = NewObjectActor->GetActorLocation();
        ShelvesWhitelist->Add(FAutoShuffleShelf());
        ShelvesWhitelist->Top().SetShelfBase(NewShelfBase);
        ShelvesWhitelist->Top().SetShelfOffset(NewShelfOffset);
        ShelvesWhitelist->Top().SetName(NewName);
        ShelvesWhitelist->Top().SetObjectActor(NewObjectActor);
        ShelvesWhitelist->Top().SetPosition(NewPosition);
        ShelvesWhitelist->Top().SetScale(NewScale);
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
        FString NewShelfName = ProductObjectJson->GetStringField("ShelfName");
        bool bProductsDiscarded = ProductObjectJson->GetBoolField("Discard");
        TArray<FAutoShuffleObject>* NewMembers = new TArray<FAutoShuffleObject>();
        TArray<TSharedPtr<FJsonValue>> Members = ProductObjectJson->GetArrayField("Members");
        for (auto MemberValueIt = Members.CreateIterator(); MemberValueIt; ++MemberValueIt)
        {
            TSharedPtr<FJsonObject> ProductMember = (*MemberValueIt)->AsObject();
            FString NewName = ProductMember->GetStringField("Name");
            AActor* NewObjectActor = nullptr;
            for (TActorIterator<AActor> ActorIt(EditorWorld); ActorIt; ++ActorIt)
            {
                if (ActorIt->GetActorLabel() == NewName)
                {
                    NewObjectActor = *ActorIt;
#ifdef VERBOSE_AUTO_SHUFFLE
                    UE_LOG(LogAutoShuffle, Log, TEXT("Found %s for %s"), *ActorIt->GetActorLabel(), *NewName);
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
            NewMembers->Top().SetObjectActor(NewObjectActor);
            NewMembers->Top().SetScale(NewScale);
            if (bProductsDiscarded)
            {
                NewMembers->Top().SetPosition(DiscardedProductsRegions);
            }
        }
        ProductsWhitelist->Add(FAutoShuffleProductGroup());
        ProductsWhitelist->Top().SetName(NewGroupName);
        ProductsWhitelist->Top().SetMembers(NewMembers);
        ProductsWhitelist->Top().SetShelfName(NewShelfName);
        if (bProductsDiscarded)
        {
            ProductsWhitelist->Top().Discard();
        }
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
        // find the Z offset of shelf bases
        TArray<float> ShelfOffsetZ;
        for (auto ShelfOffsetIt = ShelfIt->GetShelfOffset()->CreateIterator(); ShelfOffsetIt; ++ShelfOffsetIt)
        {
            ShelfOffsetZ.Add(*ShelfOffsetIt * BoundingBoxExtent.Z * 2.f);
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
            // check if the name of the shelf that this group belongs to match the current shelf name
            if (ProductGroupIt->GetShelfName() != ShelfIt->GetName())
            {
                continue;
            }
            // check if the whole group of products have been discarded in whitelist
            if (ProductGroupIt->IsDiscarded())
            {
                continue;
            }
            // get a centerilized anchor for placing products
            int ShelfBaseIdx = FMath::RandRange(0, ShelfBaseZ.Num() - 1);
            FVector Anchor;
            Anchor.Z = ShelfBaseZ[ShelfBaseIdx];
            Anchor.Y = FMath::RandRange(float(BoundingBoxOrigin.Y - BoundingBoxExtent.Y + AUTO_SHUFFLE_Y_TWO_END_OFFSET), float(BoundingBoxOrigin.Y + BoundingBoxExtent.Y - AUTO_SHUFFLE_Y_TWO_END_OFFSET));
            Anchor.X = BoundingBoxOrigin.X - BoundingBoxExtent.X;
            // iterate through all the products within the current group
            for (auto ProductIt = ProductGroupIt->GetMembers()->CreateIterator(); ProductIt; ++ProductIt)
            {
                // if rand() <= Density, select
                if (FMath::RandRange(0.f, 1.f) > Density)
                {
#ifdef VERBOSE_AUTO_SHUFFLE
                    UE_LOG(LogAutoShuffle, Log, TEXT("Product %s has been discarded"), *ProductIt->GetName());
#endif
                    ProductIt->SetPosition(DiscardedProductsRegions);
                    ProductIt->Discard();
                    ProductIt->ResetOnShelf();
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
                        ProductIt->SetShelfOffset(ShelfOffsetZ[ProductStartPointShelfBaseIdx]);
                        // deal with the offset of the product center and the bottom
                        FVector ProductOrigin, ProductExtent;
                        ProductIt->GetObjectActor()->GetActorBounds(false, ProductOrigin, ProductExtent);
                        float ProductCurrentBottom = ProductOrigin.Z - ProductExtent.Z;
                        float ProductZLift = ProductStartPoint.Z - ProductCurrentBottom;
                        float ProductCurrentFront = ProductOrigin.X - ProductExtent.X;
                        float ProductXlift = ProductStartPoint.X - ProductCurrentFront;
                        float ProductCurrentOrigin = ProductOrigin.Y;
                        float ProductYLift = ProductStartPoint.Y - ProductCurrentOrigin;
                        ProductStartPoint.X += ProductXlift;
                        ProductStartPoint.Y += ProductYLift;
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
                    // if within the maximum try times the product still didn't find the proper place, discard it
                    if (AlreadyTriedTimes == -1)
                    {
#ifdef VERBOSE_AUTO_SHUFFLE
                        UE_LOG(LogAutoShuffle, Log, TEXT("Product %s has been discarded"), *ProductIt->GetName());
#endif
                        ProductIt->SetPosition(DiscardedProductsRegions);
                        ProductIt->Discard();
                        ProductIt->ResetOnShelf();
                        continue;
                    }
                    ProductIt->SetOnShelf();
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
                    TArray<AActor*> OverlappingActors;
                    int AlreadyTriedTimes = 0;
                    // loop
                    while (AlreadyTriedTimes++ < AUTO_SHUFFLE_MAX_TRY_TIMES)
                    {
                        // get the current object's bounding box
                        ProductIt->SetPosition(Anchor);
                        FVector ProductOrigin, ProductExtent;
                        ProductIt->GetObjectActor()->GetActorBounds(false, ProductOrigin, ProductExtent);
                        float ProductCurrenBottom = ProductOrigin.Z - ProductExtent.Z;
                        float ProductZLift = Anchor.Z - ProductCurrenBottom;
                        FVector ProductStartPoint = Anchor;
                        float ProductCurrentFront = ProductOrigin.X - ProductExtent.X;
                        float ProductXlift = ProductStartPoint.X - ProductCurrentFront;
                        float ProductCurrentOrigin = ProductOrigin.Y;
                        float ProductYLift = ProductStartPoint.Y - ProductCurrentOrigin;
                        ProductStartPoint.X += ProductXlift;
                        ProductStartPoint.Y += ProductYLift;
                        ProductStartPoint.Z += ProductZLift;
                        ProductIt->SetPosition(ProductStartPoint);
                        ProductIt->SetShelfOffset(ShelfOffsetZ[ShelfBaseIdx]);
                        // see if the object could fit the anchor position
                        ProductIt->GetObjectActor()->GetOverlappingActors(OverlappingActors);
                        bool bHasCollision = OverlappingActors.Num() != 0;
                        // see if the product is in the bound of the shelf
                        ProductIt->GetObjectActor()->GetActorBounds(false, ProductOrigin, ProductExtent);
                        bool bIsInBound = ProductOrigin.Y - ProductExtent.Y >= BoundingBoxOrigin.Y - BoundingBoxExtent.Y
                            && ProductOrigin.Y + ProductExtent.Y <= BoundingBoxOrigin.Y + BoundingBoxExtent.Y;
                        if (/** no collision and inbound */ !bHasCollision && bIsInBound)
                        {
                            break;
                        }
                        // else if the product still in bound, get another anchor point that follows the perceptual organization
                        else if (bIsInBound)
                        {
                            float ProductWidth = ProductExtent.Y * 2.f;
                            // place the product to the right
                            if (AlreadyTriedTimes % 2 == 1)
                            {
                                Anchor.Y += AlreadyTriedTimes * (ProductWidth + FMath::RandRange(float(AUTO_SHUFFLE_Y_TWO_END_OFFSET * 0.5f), AUTO_SHUFFLE_Y_TWO_END_OFFSET));
                            }
                            // place the product to the left
                            else
                            {
                                Anchor.Y -= AlreadyTriedTimes * (ProductWidth + FMath::RandRange(float(AUTO_SHUFFLE_Y_TWO_END_OFFSET * 0.5f), AUTO_SHUFFLE_Y_TWO_END_OFFSET));
                            }
                        }
                        // else, randomly find another anchor point
                        else
                        {
                            ShelfBaseIdx = FMath::RandRange(0, ShelfBaseZ.Num() - 1);
                            Anchor.Z = ShelfBaseZ[ShelfBaseIdx];
                            Anchor.Y = FMath::RandRange(float(BoundingBoxOrigin.Y - BoundingBoxExtent.Y + AUTO_SHUFFLE_Y_TWO_END_OFFSET), float(BoundingBoxOrigin.Y + BoundingBoxExtent.Y - AUTO_SHUFFLE_Y_TWO_END_OFFSET));
                            Anchor.X = BoundingBoxOrigin.X - BoundingBoxExtent.X;
                        }
                    }
                    // if collision all the time, discard
                    if (AlreadyTriedTimes >= AUTO_SHUFFLE_MAX_TRY_TIMES)
                    {
#ifdef VERBOSE_AUTO_SHUFFLE
                        UE_LOG(LogAutoShuffle, Log, TEXT("Product %s has been discarded"), *ProductIt->GetName());
#endif
                        ProductIt->SetPosition(DiscardedProductsRegions);
                        ProductIt->Discard();
                        ProductIt->ResetOnShelf();
                        continue;
                    }
                    // else push the product deep inside
                    else
                    {
                        // try to push the item inside, until collided
                        ProductIt->SetOnShelf();
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
                }
            }
            if (bIsOrganizeChecked && bIsPerGroupChecked)
            {
                OrganizeProducts();
            }
        }
    }
}

void FAutoShuffleWindowModule::LowerProducts()
{
    for (auto ProductGroupIt = ProductsWhitelist->CreateIterator(); ProductGroupIt; ++ProductGroupIt)
    {
        for (auto ProductIt = ProductGroupIt->GetMembers()->CreateIterator(); ProductIt; ++ProductIt)
        {
            if (!ProductIt->IsDiscarded())
            {
                FVector Position = ProductIt->GetPosition();
                Position.Z -= ProductIt->GetShelfOffset();
                ProductIt->SetPosition(Position);
            }
        }
    }
}

void FAutoShuffleWindowModule::OrganizeProducts()
{
    // iterate through all the shelves
    for (auto ShelfIt = ShelvesWhitelist->CreateIterator(); ShelfIt; ++ShelfIt)
    {
        FString ShelfName = ShelfIt->GetName();
        FVector ShelfOrigin, ShelfExtent;
        ShelfIt->GetObjectActor()->GetActorBounds(false, ShelfOrigin, ShelfExtent);
        // collect all the products' actors which are on shelf and have not been discarded
        TArray<AActor*> Products;
        Products.Reset();
        for (auto ProductGroupIt = ProductsWhitelist->CreateIterator(); ProductGroupIt; ++ProductGroupIt)
        {
            if (ProductGroupIt->GetShelfName() != ShelfName)
            {
                continue;
            }
            for (auto ProductIt = ProductGroupIt->GetMembers()->CreateIterator(); ProductIt; ++ProductIt)
            {
                if (ProductIt->IsOnShelf() && !ProductIt->IsDiscarded() && ProductIt->GetObjectActor() != nullptr)
                {
                    Products.Add(ProductIt->GetObjectActor());
                }
            }
        }
        // sort them according to bound.Y from low to high for 001 shelf
        if (ShelfIt->GetName() == "BP_ShelfMain_001")
        {
            Products.Sort(OrganizeProductsPredicateLowToHigh);
        }
        // from high to low for 002 shelf
        else
        {
            Products.Sort(OrganizeProductsPredicateHighToLow);
        }

        // iterate through all the sorted actors
        for (auto ProductIt = Products.CreateIterator(); ProductIt; ++ProductIt)
        {
            // loop
            while (true)
            {
                // try to push them to left for 001 shelf
                FVector Position = (*ProductIt)->GetActorLocation();
                if (ShelfIt->GetName() == "BP_ShelfMain_001")
                {
                    Position.Y -= AUTO_SHUFFLE_INC_STEP;
                }
                // to right for 002 shelf
                else
                {
                    Position.Y += AUTO_SHUFFLE_INC_STEP;
                }
                (*ProductIt)->SetActorLocation(Position);
                // check if the object is still in the bound
                FVector ProductOrigin, ProductExtent;
                (*ProductIt)->GetActorBounds(false, ProductOrigin, ProductExtent);
                bool IsInBound;
                if (ShelfIt->GetName() == "BP_ShelfMain_001")
                {
                    IsInBound = ProductOrigin.Y - ProductExtent.Y >= ShelfOrigin.Y - ShelfExtent.Y;
                }
                else
                {
                    IsInBound = ProductOrigin.Y + ProductExtent.Y <= ShelfOrigin.Y + ShelfExtent.Y;
                }
                // check if there's no collision
                TArray<AActor*> OverlappingActors;
                (*ProductIt)->GetOverlappingActors(OverlappingActors);
                bool HasNoCollision = OverlappingActors.Num() == 0;
                // if not in the bound or has collision, restore and proceed to the next product
                if (!IsInBound || !HasNoCollision)
                {
                    if (ShelfIt->GetName() == "BP_ShelfMain_001")
                    {
                        Position.Y += AUTO_SHUFFLE_INC_STEP;
                    }
                    else
                    {
                        Position.Y -= AUTO_SHUFFLE_INC_STEP;
                    }
                    (*ProductIt)->SetActorLocation(Position);
                    break;
                }
            }
        }
    }
    // update the class FAUtoSHuffleObject
    for (auto ProductGroupIt = ProductsWhitelist->CreateIterator(); ProductGroupIt; ++ProductGroupIt)
    {
        for (auto ProductIt = ProductGroupIt->GetMembers()->CreateIterator(); ProductIt; ++ProductIt)
        {
            if (ProductIt->GetObjectActor() != nullptr)
            {
                FVector Position = ProductIt->GetObjectActor()->GetActorLocation();
                ProductIt->SetPosition(Position);
            }
        }
    }
}

bool FAutoShuffleWindowModule::OrganizeProductsPredicateLowToHigh(const AActor &Actor1, const AActor &Actor2)
{
    FVector Actor1Origin, Actor1Extent, Actor2Origin, Actor2Extent;
    Actor1.GetActorBounds(false, Actor1Origin, Actor1Extent);
    Actor2.GetActorBounds(false, Actor2Origin, Actor2Extent);
    return Actor1Origin.Y - Actor1Extent.Y < Actor2Origin.Y - Actor2Extent.Y;
}

bool FAutoShuffleWindowModule::OrganizeProductsPredicateHighToLow(const AActor &Actor1, const AActor &Actor2)
{
    FVector Actor1Origin, Actor1Extent, Actor2Origin, Actor2Extent;
    Actor1.GetActorBounds(false, Actor1Origin, Actor1Extent);
    Actor2.GetActorBounds(false, Actor2Origin, Actor2Extent);
    return Actor1Origin.Y - Actor1Extent.Y > Actor2Origin.Y - Actor2Extent.Y;
}

TArray<class F2DPoint>* FAutoShuffleWindowModule::TriangleRasterizer(const class F2DPointf &V1, const class F2DPointf &V2, const class F2DPointf &V3)
{
    // Reference: http://forum.devmaster.net/t/advanced-rasterization/6145
    // Reference: http://answers.unity3d.com/questions/383804/calculate-uv-coordinates-of-3d-point-on-plane-of-m.html
    // The 3rd implementation
    TArray<class F2DPoint> *PointArray = new TArray<class F2DPoint>;

    float Y1 = V1.Y, Y2 = V2.Y, Y3 = V3.Y;
    float X1 = V1.X, X2 = V2.X, X3 = V3.X;
    
    // Deltas
    float DX12 = X1 - X2, DX23 = X2 - X3, DX31 = X3 - X1;
    float DY12 = Y1 - Y2, DY23 = Y2 - Y3, DY31 = Y3 - Y1;

    // Bounding rectangle
    int MinX = (int)FMath::Min3(X1, X2, X3), MaxX = (int)FMath::Max3(X1, X2, X3);
    int MinY = (int)FMath::Min3(Y1, Y2, Y3), MaxY = (int)FMath::Max3(Y1, Y2, Y3);

    // The area of triangle by cross product a x b = a1b2 - a2b1
    float TriangleArea = FMath::Abs(DX12 * DY31 - DY12 * DX31);

    // An empty triangle
    if (TriangleArea < 1e-20f)
    {
        return PointArray;
    }

    // Constant part of half-edge functions
    float C1 = DY12 * X1 - DX12 * Y1;
    float C2 = DY23 * X2 - DX23 * Y2;
    float C3 = DY31 * X3 - DX31 * Y3;

    float CY1 = C1 + DX12 * MinY - DY12 * MinX;
    float CY2 = C2 + DX23 * MinY - DY23 * MinX;
    float CY3 = C3 + DX31 * MinY - DY31 * MinX;

    // Scan through bounding rectangle
    for (int y = MinY; y <= MaxY; ++y)
    {
        // Start value for horizontal scan
        float CX1 = CY1, CX2 = CY2, CX3 = CY3;

        for (int x = MinX; x <= MaxX; ++x)
        {
            if (CX1 > 0 && CX2 > 0 && CX3 > 0)
            {
                // calculate intepolated z
                float DXP1 = x - V1.X, DXP2 = x - V2.X, DXP3 = x - V3.X;
                float DYP1 = y - V1.Y, DYP2 = y - V2.Y, DYP3 = y - V3.Y;
                // The area of slides by cross product a x b = a1b2 - a2b1
                float A1 = FMath::Abs(DXP2 * DYP3 - DXP3 * DYP2) / TriangleArea;
                float A2 = FMath::Abs(DXP1 * DYP3 - DXP3 * DYP1) / TriangleArea;
                float A3 = FMath::Abs(DXP1 * DYP2 - DXP2 * DYP1) / TriangleArea;
                float z = A1 * V1.Z + A2 * V2.Z + A3 * V3.Z;
                PointArray->Add(F2DPoint(x, y, z));
            }
            CX1 -= DY12;
            CX2 -= DY23;
            CX3 -= DY31;
        }
        CY1 += DX12;
        CY2 += DX23;
        CY3 += DX31;
    }
    return PointArray;
}

FAutoShuffleObject::FAutoShuffleObject()
{
    Scale = 1.f;
    Name = TEXT("Uninitialized Object Name");
    Position = FVector(0.f, 0.f, 0.f);
    Rotation = FVector(0.f, 0.f, 0.f);
    ObjectActor = nullptr;
    bIsDiscarded = false;
    bIsOnShelf = false;
    fShelfOffset = 0.f;
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
    if (ObjectActor != nullptr)
    {
        ObjectActor->SetActorScale3D(FVector(Scale, Scale, Scale));
    }
}

void FAutoShuffleObject::ShrinkScale()
{
    // shrink the scale on z to 1/3 of x and/or y
    if (ObjectActor != nullptr)
    {
        ObjectActor->SetActorScale3D(FVector(Scale, Scale, Scale * 0.3f));
    }
}

void FAutoShuffleObject::ExpandScale()
{
    /** This function restores the object scale up to the scale indicated by private variable Scale.
     *  The goal is to expand the scale while the bottom is kept. So it's like the product growing up
     *  from the shelf 
     *  @note the scaling process does not guarantee the position unchanged
     */
    if (ObjectActor != nullptr)
    {
        // get the bottom and the Origin.XY of the product. These are the variables that the product must keep
        FVector ProductOrigin, ProductExtent;
        ObjectActor->GetActorBounds(false, ProductOrigin, ProductExtent);
        float ConstBottomLine = ProductOrigin.Z - ProductExtent.Z;
        float ProductOriginX = ProductOrigin.X;
        float ProductOriginY = ProductOrigin.Y;
        // change the scale.x and scale.y to scale.z to start the expansion
        float CurrentScale = ObjectActor->GetActorScale3D().Z;
        ObjectActor->SetActorScale3D(FVector(CurrentScale, CurrentScale, CurrentScale));
        // loop
        while (true)
        {
            // Get the overlapping actors
            TArray<AActor*> OverlappingActors;
            ObjectActor->GetOverlappingActors(OverlappingActors);
            // if overlapped, we stop
            if (OverlappingActors.Num() != 0)
            {
                CurrentScale -= 0.1;
                ObjectActor->SetActorScale3D(FVector(CurrentScale, CurrentScale, CurrentScale));
                ObjectActor->GetActorBounds(false, ProductOrigin, ProductExtent);
                float CurrentBottomLine = ProductOrigin.Z - ProductExtent.Z;
                float ProductZLift = ConstBottomLine - CurrentBottomLine;
                Position.Z += ProductZLift;
                Position.X += ProductOriginX - ProductOrigin.X;
                Position.Y += ProductOriginY - ProductOrigin.Y;
                this->SetPosition(Position);
                break;
            }
            // if the currentscale is already the Scale specified in the whitelist, we stop
            if (CurrentScale >= Scale)
            {
                ObjectActor->SetActorScale3D(FVector(Scale, Scale, Scale));
                ObjectActor->GetActorBounds(false, ProductOrigin, ProductExtent);
                float CurrentBottomLine = ProductOrigin.Z - ProductExtent.Z;
                float ProductZLift = ConstBottomLine - CurrentBottomLine;
                Position.Z += ProductZLift;
                Position.X += ProductOriginX - ProductOrigin.X;
                Position.Y += ProductOriginY - ProductOrigin.Y;
                this->SetPosition(Position);
                break;
            }
            // otherwise, we increase the scale wholely, then adjust the bottom to the ConstBottomLine and the origin.XY to the original Origin.XY
            CurrentScale += 0.1;
            ObjectActor->SetActorScale3D(FVector(CurrentScale, CurrentScale, CurrentScale));
            ObjectActor->GetActorBounds(false, ProductOrigin, ProductExtent);
            float CurrentBottomLine = ProductOrigin.Z - ProductExtent.Z;
            float ProductZLift = ConstBottomLine - CurrentBottomLine;
            Position.Z += ProductZLift;
            Position.X += ProductOriginX - ProductOrigin.X;
            Position.Y += ProductOriginY - ProductOrigin.Y;
            this->SetPosition(Position);
        }
    }
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

void FAutoShuffleObject::Discard()
{
    bIsDiscarded = true;
}

bool FAutoShuffleObject::IsDiscarded()
{
    return bIsDiscarded;
}

void FAutoShuffleObject::ResetDiscard()
{
    bIsDiscarded = false;
}

void FAutoShuffleObject::SetShelfOffset(float NewShelfOffset)
{
    fShelfOffset = NewShelfOffset;
}

float FAutoShuffleObject::GetShelfOffset() const
{
    return fShelfOffset;
}

bool FAutoShuffleObject::IsContainedIn(FVector &BoundOrigin, FVector &BoundExtent)
{
    if (ObjectActor != nullptr)
    {
        FVector ObjectOrigin, ObjectExtent;
        ObjectActor->GetActorBounds(false, ObjectOrigin, ObjectExtent);
        if (BoundOrigin.X - BoundExtent.X > ObjectOrigin.X - ObjectExtent.X)
            return false;
        if (BoundOrigin.X + BoundExtent.X < ObjectOrigin.X + ObjectExtent.X)
            return false;
        if (BoundOrigin.Y - BoundExtent.Y > ObjectOrigin.Y - ObjectOrigin.Y)
            return false;
        if (BoundOrigin.Y + BoundExtent.Y < ObjectOrigin.Y + ObjectExtent.Y)
            return false;
        if (BoundOrigin.Z - BoundExtent.Z > ObjectOrigin.Z - ObjectExtent.Z)
            return false;
        if (BoundOrigin.Z + BoundExtent.Z < ObjectOrigin.Z + ObjectExtent.Z)
            return false;
        return true;
    }
    else
    {
        return false;
    }
}

bool FAutoShuffleObject::IsOnShelf() const
{
    return bIsOnShelf;
}

void FAutoShuffleObject::SetOnShelf()
{
    bIsOnShelf = true;
}

void FAutoShuffleObject::ResetOnShelf()
{
    bIsOnShelf = false;
}

FAutoShuffleShelf::FAutoShuffleShelf() : FAutoShuffleObject()
{
    ShelfBase = nullptr;
    ShelfOffset = nullptr;
}

FAutoShuffleShelf::~FAutoShuffleShelf()
{
    if (ShelfBase != nullptr)
    {
        delete ShelfBase;
    }
    if (ShelfOffset != nullptr)
    {
        delete ShelfOffset;
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

void FAutoShuffleShelf::SetShelfOffset(TArray<float> *NewShelfOffset)
{
    if (ShelfOffset != nullptr)
    {
        delete ShelfOffset;
    }
    ShelfOffset = NewShelfOffset;
}

TArray<float>* FAutoShuffleShelf::GetShelfOffset() const
{
    return ShelfOffset;
}

FAutoShuffleProductGroup::FAutoShuffleProductGroup()
{
    Members = nullptr;
    Name = TEXT("Uninitialized Object Name");
    ShelfName = TEXT("Unintialized Object Name");
    bIsDiscarded = false;
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

void FAutoShuffleProductGroup::SetShelfName(FString& NewShelfName)
{
    ShelfName = NewShelfName;
}

FString FAutoShuffleProductGroup::GetShelfName() const
{
    return ShelfName;
}

void FAutoShuffleProductGroup::Discard()
{
    bIsDiscarded = true;
}

void FAutoShuffleProductGroup::ResetDiscard()
{
    bIsDiscarded = false;
}

bool FAutoShuffleProductGroup::IsDiscarded()
{
    return bIsDiscarded;
}

F2DPoint::F2DPoint(int NewX, int NewY, float NewZ)
{
    Y = NewY;
    X = NewX;
    Z = NewZ;
}

F2DPointf::F2DPointf(float NewX, float NewY, float NewZ)
{
    Y = NewY;
    X = NewX;
    Z = NewZ;
}

FOcclusionPixel::FOcclusionPixel(int NewActorIdx, float NewDepth)
{
    ActorIdx = NewActorIdx;
    Depth = NewDepth;
}

FOcclusionPixel::FOcclusionPixel()
{
    ActorIdx = -1;
    Depth = 1e10f;
}

bool FOcclusionPixel::OrganizePixelPredicateLowToHigh(const FOcclusionPixel &Pixel1, const FOcclusionPixel &Pixel2)
{
    return Pixel1.Depth < Pixel2.Depth;
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FAutoShuffleWindowModule, AutoShuffleWindow)