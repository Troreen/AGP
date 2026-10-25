// Copyright Epic Games, Inc. All Rights Reserved.

#include "TGAUnrealPlugin.h"

#include <FBX/2020.2/include/fbxsdk/core/fbxmanager.h>

#include "Editor.h"
#include "JsonBlueprintFunctionLibrary.h"
#include "JsonObjectWrapper.h"
#include "FBX/2020.2/include/fbxsdk.h"
#include "FBX/2020.2/include/fbxsdk/scene/geometry/fbxnode.h"
#include "FBX/2020.2/include/fbxsdk/scene/geometry/fbxgeometrybase.h"
#include "FBX/2020.2/include/fbxsdk/scene/geometry/fbxmesh.h"
#include "FBX/2020.2/include/fbxsdk/scene/fbxscene.h"
#include "FBX/2020.2/include/fbxsdk/core/fbxmanager.h"
#include "NavigationSystem.h"
#include "TGAUnrealPluginStyle.h"
#include "TGAUnrealPluginCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Public/DesktopPlatformModule.h"
#include "Public/IDesktopPlatform.h"
#include "Dom/JsonObject.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/GameplayStatics.h"
#include "NavMesh/RecastNavMesh.h"
#include "Public/Detour/DetourNavMesh.h"
#include "Subsystems/UnrealEditorSubsystem.h"
#include "LandscapeProxy.h"
#include "LandscapeComponent.h"
#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ImageUtils.h"
#include "JsonObjectConverter.h"
#include "LandscapeComponent.h"
#include "LandscapeProxy.h"
#include "LevelEditor.h"
#include "Misc/FileHelper.h"

#include "TGAExportSettingsComponent.h"
#include "TGAPluginDeveloperSettings.h"
#include "TGASerializableInterface.h"
#include "Detour/DetourNavMesh.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Styling/SlateStyleMacros.h"

static const FName TGAUnrealPluginTabName("TGAUnrealPlugin");

#define LOCTEXT_NAMESPACE "FTGAUnrealPluginModule"

DEFINE_LOG_CATEGORY(LogTGA);

#define TGA_LOG(Message,...) UE_LOG(LogTGA, Log, TEXT("[  LOG  ] " Message), ##__VA_ARGS__)
#define TGA_WARN(Message,...) UE_LOG(LogTGA, Warning, TEXT("[WARNING] " Message), ##__VA_ARGS__)
#define TGA_ERR(Message,...) UE_LOG(LogTGA, Error, TEXT("[ ERROR ] " Message), ##__VA_ARGS__)

void FTGAUnrealPluginModule::StartupModule()
{
	const UTGAPluginDeveloperSettings* Settings = GetDefault<UTGAPluginDeveloperSettings>();
	if (!Settings->RelativeExportPath.IsEmpty())
	{
		LastLevelExportFolder = UKismetSystemLibrary::GetProjectDirectory() / Settings->RelativeExportPath;
		DefaultExportFolder = LastLevelExportFolder;
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		PlatformFile.CreateDirectory(*LastLevelExportFolder);
	}
	else
	{
		LastLevelExportFolder = UKismetSystemLibrary::GetProjectDirectory();
		DefaultExportFolder = LastLevelExportFolder;
	}
	
	FTGAUnrealPluginStyle::Initialize();
	FTGAUnrealPluginStyle::ReloadTextures();

	FTGAUnrealPluginCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FTGAUnrealPluginCommands::Get().TGAExportLevel,
		FExecuteAction::CreateRaw(this, &FTGAUnrealPluginModule::Command_ExportLevel),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTGAUnrealPluginModule::RegisterMenus));

	LevelSerializer.Initialize();
}

void FTGAUnrealPluginModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FTGAUnrealPluginStyle::Shutdown();

	FTGAUnrealPluginCommands::Unregister();
}

void FTGAUnrealPluginModule::Command_ExportLevel()
{
	// Make sure the Editor World actually exists...
	UUnrealEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();
	const UWorld* World = EditorSubsystem->GetEditorWorld();
	if(!World)
	{
		return;
	}

	// And that we have a valid level loaded
	const ULevel* Level = World->GetCurrentLevel();
	if(!Level)
	{
		return;
	}
	
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	
	const UTGAPluginDeveloperSettings* Settings = GetDefault<UTGAPluginDeveloperSettings>();
	if (!Settings->RememberExportPath)
		LastLevelExportFolder = DefaultExportFolder;

	// Figure out where the user wants us to save the files...
	// We'll create a directory with the same name as the level if it doesn't exist.
	FString SavePath;	
	const bool bOpened = DesktopPlatform->OpenDirectoryDialog(
		nullptr,
		FString("Export Level (TGA)"),
		LastLevelExportFolder,
		SavePath
		);

	if(!bOpened || SavePath.IsEmpty())
	{
		return;
	}

	LastLevelExportFolder = SavePath;

	const FString LevelName = World->GetMapName();

	SavePath /= LevelName;	
	SavePath.ToLowerInline();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if(!PlatformFile.CreateDirectory(*SavePath))
	{
		const FText DialogText = FText::FromString("Could not create the path " + SavePath);
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);
		return;
	}
	
	// Export Navmesh
	GAllowActorScriptExecutionInEditor = true;
	ExportNavMeshToFBX(World, SavePath);
	const bool bExported = ExportLevelToJSON(World, SavePath);
	GAllowActorScriptExecutionInEditor = false;
	if (!bExported)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Level export failed. See the Output Log for details."));
	}
}

PRAGMA_DISABLE_OPTIMIZATION_ACTUAL
bool FTGAUnrealPluginModule::ExportLevelToJSON(const UWorld* World, const FString& SavePath)
{
	if(!World)
	{
		UE_LOG(LogTGA, Error, TEXT("Failed to export scene because no valid World was found!"));
		return false;
	}
	if(!World->GetCurrentLevel())
	{
		UE_LOG(LogTGA, Error, TEXT("Failed to export scene because no level has been loaded!"));
		return false;
	}

	const FString LevelName = World->GetMapName();
	
	UE_LOG(LogTGA, Log, TEXT("Exporting %s..."), *LevelName);

	TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject());
	TArray<AActor*> WorldActors;
	UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), WorldActors);

	UE_LOG(LogTGA, Log, TEXT("Going through %d actors."), WorldActors.Num());

	const UTGAPluginDeveloperSettings* Settings = GetDefault<UTGAPluginDeveloperSettings>();
	
	// Prepass to get all materials and exportable actors.
	TSet<UMaterialInterface*> WorldMaterials;
	TArray<AActor*> ExportActors;
	ExportActors.Reserve(WorldActors.Num());
	for (AActor* Actor : WorldActors)
	{
		if (IsActorAllowedToExport(Actor))
		{
			ExportActors.Add(Actor);
			
			TArray<UMeshComponent*> ActorComponents;
			Actor->GetComponents<UMeshComponent>(ActorComponents);
			for (UMeshComponent* Component : ActorComponents)
			{
				for (UMaterialInterface* mat : Component->GetMaterials())
				{
					if (mat)
					{
						WorldMaterials.Add(mat->GetBaseMaterial());
					}
				}
			}

			TArray<ULandscapeComponent*> LandscapeComponents;
			Actor->GetComponents<ULandscapeComponent>(LandscapeComponents);
			for (ULandscapeComponent* Component : LandscapeComponents)
			{
				UMaterialInterface* Material = Component->GetLandscapeMaterial(0);
				if (Material)
				{
					WorldMaterials.Add(Material);
				}

				for (UMaterialInstanceConstant* matInstance : Component->MaterialInstances)
				{
					if (matInstance)
					{
						WorldMaterials.Add(matInstance->GetBaseMaterial());
					}
				}

			}
			if (ALandscapeProxy* Landscape = Cast<ALandscapeProxy>(Actor))
			{
				if (!ExportLandscapeWeightmaps(LevelName, Landscape, SavePath))
				{
					TGA_ERR("Landscape weightmap export failed for %s", *Actor->GetPathName());
					return false;
				}
			}
		}
	}

	TArray<TSharedPtr<FJsonValue>> JsonMaterials;
	for (UMaterialInterface* Material : WorldMaterials)
	{
		TSharedPtr<FJsonObject> MaterialJson = LevelSerializer.SerializeMaterial(Material, false);
		if (MaterialJson.IsValid())
		{
			JsonMaterials.Add(MakeShareable(new FJsonValueObject(MaterialJson)));
		}
	}	
	
	Json->SetArrayField("MasterMaterials", JsonMaterials);
	
	TArray<TSharedPtr<FJsonValue>> JsonActors;
	for(AActor* Actor : ExportActors)
	{
		TGA_LOG(TEXT("Processing %s..."), *GetNameSafe(Actor));

		TSharedPtr<FJsonObject> ActorJson = MakeShareable(new FJsonObject());
		FJsonObjectConverter Converter;
		
		ActorJson->SetStringField("Name", *Actor->GetActorLabel());
		const UClass* ActorClass = Actor->GetClass();
		ActorJson->SetStringField("Archetype", ActorClass->GetName());

		const FTransform ActorTransform = Actor->GetActorTransform();

		const FMatrix TransformMatrix = ActorTransform.ToMatrixWithScale();
		TArray<TSharedPtr<FJsonValue>> JsonTransformMatrix;
		JsonTransformMatrix.Reserve(16);
		for(int r = 0; r < 4; ++r)
			for(int c = 0; c < 4; ++c)
			{
				JsonTransformMatrix.Add(MakeShared<FJsonValueNumber>(TransformMatrix.M[r][c]));
			}

		ActorJson->SetArrayField("Transform", JsonTransformMatrix);

		TArray<TSharedPtr<FJsonValue>> JsonProperties;

		// If this is a Blueprint Actor we should export any Variables that were/
		// created in the graph.
		if(!ActorClass->IsNative())
		{
			const FField* ClassField = ActorClass->ChildProperties;
			while(ClassField)
			{
				FProperty* Prop = (FProperty*)ClassField;
				if(Prop)
				{
					FString FieldName = Prop->GetName();
					FString VariableName = Prop->GetAuthoredName();
					FString CPPType = Prop->GetCPPType();
					if(Prop->GetOwner<UObject>() == Actor)
					{
						const void* Value = Prop->ContainerPtrToValuePtr<uint8>(Actor);

						TSharedPtr<FJsonValue> JsonValue = Converter.UPropertyToJsonValue(Prop, Value);
						if (JsonValue.IsValid())
						{
							const TSharedPtr<FJsonObject> PropertyJson = MakeShareable(new FJsonObject());
							PropertyJson->SetStringField("Name", VariableName);
							PropertyJson->SetStringField("Type", CPPType);
							PropertyJson->SetField("Value", JsonValue);
							//PropertyJson->SetStringField()
							JsonProperties.Add(MakeShareable(new FJsonValueObject(PropertyJson)));
						}
					}
				}				
				
				ClassField = ClassField->Next;
			}
		}
		if(!JsonProperties.IsEmpty())
		{
			ActorJson->SetArrayField("Properties", JsonProperties);	
		}

		TArray<TSharedPtr<FJsonValue>> JsonTags;
		if(!Actor->Tags.IsEmpty())
		{
			for(const FName& Tag : Actor->Tags)
			{
				JsonTags.Add(MakeShareable(new FJsonValueString(Tag.ToString())));
			}
		}

		ActorJson->SetArrayField("Tags", JsonTags);

		TArray<UActorComponent*> ActorComponents;
		Actor->GetComponents<UActorComponent>(ActorComponents);
		TArray<TSharedPtr<FJsonValue>> JsonComponents;
	
		for(UActorComponent* Component : ActorComponents)
		{
			TSharedPtr<FJsonObject> ComponentJson = MakeShareable(new FJsonObject());
			
			if(LevelSerializer.SerializeComponent(Actor, Component, ComponentJson))
			{
				JsonComponents.Add(MakeShareable(new FJsonValueObject(ComponentJson)));	
			}
		}

		ActorJson->SetArrayField("Components", JsonComponents);
		// Finally allow custom serialization if we support that
		if(Actor->Implements<UTGASerializableInterface>())
		{
			// This doesn't work since the interface function doesn't run.
			FJsonObjectWrapper Object{};
			ITGASerializableInterface::Execute_TGA_Serialize(Actor, Object);
			if(!Object.JsonObject->Values.IsEmpty())
				ActorJson->SetObjectField("Custom", Object.JsonObject);
		}
		
		JsonActors.Add(MakeShareable(new FJsonValueObject(ActorJson)));
	}
	
	Json->SetArrayField("Actors", JsonActors);
	
	FString JsonString;
	const auto Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

	const FString OutputFile = SavePath / (LevelName + "_Level.json");
	
	if (!FFileHelper::SaveStringToFile(JsonString, *OutputFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		TGA_ERR("Cannot write level JSON: %s", *OutputFile);
		return false;
	}

	UE_LOG(LogTGA, Log, TEXT("Exported level with %d actors!"), JsonActors.Num());
	UE_LOG(LogTGA, Log, TEXT("File: %s"), *OutputFile);



	const FText DialogText = FText::FromString("Level exported to " + SavePath);
	const FText DialogTitle = FText::FromString("Export Level to JSON");
	FMessageDialog::Open(EAppMsgCategory::Success, EAppMsgType::Ok, DialogText, DialogTitle);
	
	return true;
}

bool FTGAUnrealPluginModule::ExportLandscapeWeightmaps(FString LevelName, ALandscapeProxy* LandscapeProxy, const FString& SavePath)
{
    if (!LandscapeProxy)
    {
        return false;
    }

    // Ensure target directory exists
    if (!IFileManager::Get().MakeDirectory(*SavePath, true))
    {
        return false;
    }

    TArray<ULandscapeComponent*> LandscapeComponents;
    LandscapeProxy->GetComponents(LandscapeComponents);

    for (ULandscapeComponent* LandscapeComp : LandscapeComponents)
    {
        // Each component can have multiple weightmap textures (packing up to 4 layers per texture)
	TArray<TObjectPtr<UTexture2D>>& Textures = LandscapeComp->GetWeightmapTextures();
        for (const FWeightmapLayerAllocationInfo& Allocation : LandscapeComp->GetWeightmapLayerAllocations())
        {
            if (Allocation.IsAllocated() && (!Allocation.LayerInfo ||
                Allocation.WeightmapTextureIndex >= Textures.Num() || Allocation.WeightmapTextureChannel >= 4))
            {
                TGA_ERR("Invalid weightmap layer allocation: %s", *LandscapeComp->GetPathName());
                return false;
            }
        }
        for (int32 TexIdx = 0; TexIdx < Textures.Num(); ++TexIdx)
        {
            UTexture2D* WeightmapTex = Textures[TexIdx];
            if (!WeightmapTex)
            {
                return false;
            }

            // Landscape editing updates Source; platform bulk data can retain older pixels.
            FTextureSource& Source = WeightmapTex->Source;
            if (!Source.IsValid() || Source.GetFormat() != TSF_BGRA8 || Source.GetNumMips() == 0)
            {
                TGA_ERR("Unsupported weightmap source: %s", *WeightmapTex->GetPathName());
                return false;
            }

            const int32 Width = Source.GetSizeX();
            const int32 Height = Source.GetSizeY();
            TArray64<uint8> SourceData;

            // GetMipData returns a copy, so no texture source lock survives this read.
            const int64 PixelCount = int64(Width) * Height;
            if (Width <= 0 || Height <= 0 || PixelCount > MAX_int32 ||
                !Source.GetMipData(SourceData, 0) || SourceData.Num() != PixelCount * sizeof(FColor))
            {
                TGA_ERR("Invalid weightmap data: %s", *WeightmapTex->GetPathName());
                return false;
            }

            // Preserve linear weight bytes, including alpha, without color conversion.
            TArray<FColor> Pixels;
            Pixels.SetNumUninitialized(Width * Height);
            FMemory::Memcpy(Pixels.GetData(), SourceData.GetData(), SourceData.Num());

            // CompressImageArray uses thumbnail JPEG compression in UE 5.7.
            TArray64<uint8> CompressedPngData;
            FImageUtils::PNGCompressImageArray(Width, Height,
                TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), CompressedPngData);

            // Construct file path (e.g., Saved/LandscapeExports/Comp_0_Weightmap_0.png)
            FString FileName = FString::Printf(TEXT("%s_%s_Weightmap_%d.png"), *LandscapeProxy->GetName(), *LandscapeComp->GetName(), TexIdx);
            FString FullPath = FPaths::Combine(SavePath, FileName);

            // Write the file to disk
            if (!CompressedPngData.IsEmpty() && FFileHelper::SaveArrayToFile(CompressedPngData, *FullPath))
            {
                UE_LOG(LogTemp, Log, TEXT("Exported weightmap to: %s"), *FullPath);
            }
            else
            {
                TGA_ERR("Could not write weightmap: %s", *FullPath);
                return false;
            }
        }
    }
    return true;
}

bool FTGAUnrealPluginModule::ExportNavMeshToFBX(const UWorld* World, const FString& SavePath)
{
	const ARecastNavMesh* NavMesh = TryGetNavMesh(World);
	const FString LevelName = World->GetMapName();
	
	if(IsValid(NavMesh))
	{
		// Number of tiles we have.
		unsigned int NumTiles = 0;

		// Vertices, just positions.
		TArray<FVector> Vertices;

		// Keep track of duplicate vertices between tiles.
		TMap<uint32, unsigned int> VertexDuplicateMap;

		struct Poly
		{
			TArray<unsigned int> Indices;
		};

		TArray<Poly> Polygons;
		uint32 vxX = GetTypeHash(1.0f);
		
		const dtNavMesh* Mesh = NavMesh->GetRecastMesh();
		for(int t = 0; t < Mesh->getMaxTiles(); t++)
		{
			const dtMeshTile* Tile = Mesh->getTile(t);
			if(!Tile || !Tile->header || !Tile->dataSize)
			{
				continue;
			}

			NumTiles++;
			const unsigned int NumTilePolys = Tile->header->polyCount;
			const unsigned int NumTileVx = Tile->header->vertCount;

			for(unsigned int p = 0; p < NumTilePolys; p++)
			{
				Poly Polygon;
				// Polys have any number of vertices as generated.
				// There will be at least 3 but may be 4, 5, 6 etc.
				const unsigned int NumVerts = Tile->polys[p].vertCount;
				
				const dtPoly poly = Tile->polys[p];
				
				if (poly.vertCount < 3)
					continue;
				
				for(int i = 0; i < poly.vertCount; i++)
				{
					// Vert saved at poly.verts[i] + 0, 1, 2
					const dtReal* ptr = &Tile->verts[poly.verts[i]*3]; 
					const float X = ptr[0];
					const float Y = ptr[1];
					const float Z = ptr[2];

					FVector Vert = FVector(X, Y, Z);
					
					const uint32 VertHash = GetTypeHash(Vert);
					const unsigned int* existingIdx = VertexDuplicateMap.Find(VertHash);
					if(existingIdx)
					{
						// use existing
						Polygon.Indices.Add(*existingIdx);
					}
					else
					{
						// add
						unsigned int Idx = Vertices.Add(Vert);
						VertexDuplicateMap.Add(VertHash, Idx);
						Polygon.Indices.Add(Idx);
					}
				}

				Polygons.Add(Polygon);
			}
		}

		// Now we have all polygons.
		// We can dump them to FBX.

		FbxManager* FbxMgr = FbxManager::Create();
		FbxScene* FbxScene = FbxScene::Create(FbxMgr, "TGAScene");
		
		FbxGlobalSettings& GlobalSettings = FbxScene->GetGlobalSettings();
		GlobalSettings.SetSystemUnit(FbxSystemUnit::cm);
		FbxAxisSystem UEAxisSystem(FbxAxisSystem::eZAxis, FbxAxisSystem::eParityEven, FbxAxisSystem::eLeftHanded);
		GlobalSettings.SetAxisSystem(UEAxisSystem);
		
		FbxNode* FbxNode = FbxNode::Create(FbxMgr, "MeshNode");
		FbxScene->GetRootNode()->AddChild(FbxNode);
		
		FbxMesh* FbxMesh = FbxMesh::Create(FbxMgr, "NavMesh");
		FbxNode->SetNodeAttribute(FbxMesh);
		
		FbxMesh->InitControlPoints(Vertices.Num());
		
		for(int v = 0; v < Vertices.Num(); v++)
		{
			FbxVector4* ControlPoints = FbxMesh->GetControlPoints();
			const FVector& Vertex = Vertices[v]; 
			ControlPoints[v] = FbxVector4(Vertex.X, Vertex.Y, Vertex.Z);
		}
		
		for(const Poly& Polygon : Polygons)
		{
			FbxMesh->BeginPolygon();
			for(int Idx = 0; Idx < Polygon.Indices.Num(); Idx++)
			{				
				FbxMesh->AddPolygon(Polygon.Indices[Idx]);
			}
			FbxMesh->EndPolygon();
		}

		FbxAxisSystem::MayaYUp.DeepConvertScene(FbxScene);
		
		const FString OutputFile = SavePath / (LevelName + "_NavMesh.fbx");
		
		FbxIOSettings* FbxIOSettings = FbxIOSettings::Create(FbxMgr, "TGA");
		FbxMgr->SetIOSettings(FbxIOSettings);
		
		FbxExporter* FbxExporter = FbxExporter::Create(FbxMgr, "Exporter");
		FbxExporter->Initialize(TCHAR_TO_ANSI(*OutputFile), -1, FbxIOSettings);
		FbxStatus& Status = FbxExporter->GetStatus();
		if(!Status.Error())
		{
			FbxExporter->Export(FbxScene);
			Status = FbxExporter->GetStatus();
			if(!Status.Error())
			{
				UE_LOG(LogTGA, Log, TEXT("Exported Navmesh!"));
				UE_LOG(LogTGA, Log, TEXT("File: %s"), *OutputFile);
				FbxExporter->Destroy();
				return true;
			}
		}
		FbxExporter->Destroy();

		const FString StatusMessage = FString(ANSI_TO_TCHAR(Status.GetErrorString()));
		UE_LOG(LogTGA, Log, TEXT("FBX Export reported error: %s"), *StatusMessage);
	}

	return false;
}

const ARecastNavMesh* FTGAUnrealPluginModule::TryGetNavMesh(const UWorld* World) const
{
	if(const UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		if(const ANavigationData* NavData = NavigationSystem->GetDefaultNavDataInstance())
		{
			if(const ARecastNavMesh* NavMesh = Cast<ARecastNavMesh>(NavData))
			{
				return NavMesh;
			}			
		}
	}

	// FText DialogText = FText::FromString("No navigation has been built for this level, NavMesh will not be exported!");
	// FMessageDialog::Open(EAppMsgType::Ok, DialogText);
	UE_LOG(LogTGA, Warning, TEXT("No navigation has been built for this level, NavMesh will not be exported!"));
	return nullptr;
}

PRAGMA_ENABLE_OPTIMIZATION_ACTUAL

void FTGAUnrealPluginModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	// {
	// 	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	// 	{
	// 		FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
	// 		Section.AddMenuEntryWithCommandList(FTGAUnrealPluginCommands::Get().TGAExportLevel, PluginCommands);
	// 	}
	// }

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				// This is the boilerplate plugin button addition code.
				// FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FTGAUnrealPluginCommands::Get().TGAExportLevel));
				// Entry.SetCommandList(PluginCommands);
			}
			{
				FUIAction TGAMenuShownDelegate;
				TGAMenuShownDelegate.IsActionVisibleDelegate = FIsActionButtonVisible::CreateLambda([]()
					{
						#if WITH_EDITOR
						return GEditor->PlayWorld == nullptr;
						#else
						return false;
						#endif						
					}
				);
				
				FToolMenuEntry Entry = FToolMenuEntry::InitComboButton(
					"TGAMenu",
					TGAMenuShownDelegate,
					FOnGetContent::CreateLambda([this]
					{
						static const FName MenuName("TGAPlugin.Editor.TgaPluginMenu");
						const bool bShouldCloseWindowAfterMenuSelection = true;
						UToolMenu* Menu = UToolMenus::Get()->FindMenu(MenuName);
						if(Menu == nullptr)
						{
							Menu = UToolMenus::Get()->RegisterMenu(MenuName);

							FToolMenuSection& ExportSection = Menu->AddSection("ExportSection", LOCTEXT("TgaPlugin.Menu.ExportSection", "Export"));
							FToolMenuEntry& LevelToJsonEntry = ExportSection.AddMenuEntry(
								"LevelToJSON",
								LOCTEXT("ExportToJson", "Export Level to JSON"),
								LOCTEXT("ExportToJsonTooltip", "Exports all supported data in the level to a JSON file and optional support files."),
								FSlateIcon(FTGAUnrealPluginStyle::GetStyleSetName(), TEXT("Icons.TGAExport")),
								FUIAction(FExecuteAction::CreateRaw(this, &FTGAUnrealPluginModule::Command_ExportLevel))
							);
						}
						
						return UToolMenus::Get()->GenerateWidget(Menu);
					}),
					LOCTEXT("TgaMenu", "TGA"),
					LOCTEXT("TgaMenuTooltip", "Opens the TGA Plugin Menu"),
					FSlateIcon(FTGAUnrealPluginStyle::GetStyleSetName(), TEXT("Icons.TGA"))
					);

				Section.AddEntry(Entry);
			}
		}
	}
}

bool FTGAUnrealPluginModule::IsActorAllowedToExport(const AActor* Actor) const
{
	if (!Actor)
		return false;
	
	const UTGAPluginDeveloperSettings* Settings = GetDefault<UTGAPluginDeveloperSettings>();
	UClass* ActorClass = Actor->GetClass();
	for (UClass* ExcludedClass : Settings->TypeExclusionList)
	{
		if (ActorClass->IsChildOf(ExcludedClass))
		{
			TGA_LOG(TEXT("%s is on the Type Exclusion List and will not be exported."), *GetNameSafe(Actor));
			return false;
		}			
	}

	// May have more settings here in the future
	// but this component should never be a requirement to export an Actor.
	if (UTGAExportSettingsComponent* ESC = Actor->GetComponentByClass<UTGAExportSettingsComponent>())
	{
		if(ESC->bDoNotExport)
		{
			TGA_LOG(TEXT("%s is marked Do Not Export and will not be exported."), *GetNameSafe(Actor));
			return false;
		}
	}

	// Check if we have components that the serializer understands.
	TArray<UActorComponent*> ActorComponents;
	Actor->GetComponents<UActorComponent>(ActorComponents);

	// If we have no components at all, don't serialize the actor.
	if(ActorComponents.IsEmpty())
	{
		TGA_LOG(TEXT("%s has no Components and will not be exported."), *GetNameSafe(Actor));
		return false;
	}

	// Make sure the actor has at least one component that we can serialize.
	bool ShouldSerializeActor = false;
	for(UActorComponent* Component : ActorComponents)
	{
		if(LevelSerializer.CanSerializeComponent(Actor, Component))
		{
			ShouldSerializeActor = true;
			break;
		}				
	}

	if (!ShouldSerializeActor)
	{
		TGA_LOG(TEXT("%s has no Components which can be Serialized and will not be exported."), *GetNameSafe(Actor));
	}
	
	return ShouldSerializeActor;
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTGAUnrealPluginModule, TGAUnrealPlugin)
