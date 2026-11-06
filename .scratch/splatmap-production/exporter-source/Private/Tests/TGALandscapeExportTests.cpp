#include "TGALandscapeExport.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Editor.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "LandscapeLayerInfoObject.h"
#include "Materials/Material.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TGAExportSettingsComponent.h"
#include "TGAPluginDeveloperSettings.h"
#include "TGAUnrealPlugin.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTGALandscapeAtlasTest, "TGA.LandscapeExport.NamedAtlas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGALandscapeAtlasTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
	ALandscape* Landscape = World->SpawnActor<ALandscape>();
	Landscape->SetActorTransform(FTransform(FQuat::Identity, FVector(-300, 700, 0), FVector(100, 100, 100)));
	TArray<ULandscapeComponent*> Components;
	for (int32 TileIndex = 0; TileIndex < 4; ++TileIndex)
	{
		ULandscapeComponent* Component = NewObject<ULandscapeComponent>(Landscape);
		Landscape->AddInstanceComponent(Component);
		Component->ComponentSizeQuads = 63;
		Component->SubsectionSizeQuads = 63;
		Component->NumSubsections = 1;
		const FIntPoint Base((TileIndex % 2) * 63, (TileIndex / 2) * 63);
		Component->SetSectionBase(Base);
		Component->SetRelativeLocation(FVector(Base.X, Base.Y, 0));
		TArray<FColor> Pixels;
		Pixels.SetNumUninitialized(64 * 64);
		for (int32 Y = 0; Y < 64; ++Y)
		{
			for (int32 X = 0; X < 64; ++X)
			{
				// Dunes uses alpha in tile zero and red elsewhere; Rough uses blue
				// then green. Cliff is absent on tile zero and zero on shared edges.
				const uint8 Dunes = uint8(Base.X + X);
				const uint8 Rough = uint8(Base.Y + Y);
				const uint8 Cliff = Base.X + X > 63 && Base.Y + Y > 63 ? 211 : 0;
				Pixels[Y * 64 + X] = TileIndex == 0 ? FColor(17, 19, Rough, Dunes) : FColor(Dunes, Rough, Cliff, 33);
			}
		}
		UTexture2D* Texture = NewObject<UTexture2D>();
		Texture->Source.Init(64, 64, 1, 1, TSF_BGRA8, reinterpret_cast<const uint8*>(Pixels.GetData()));
		Component->GetWeightmapTextures().Add(Texture);
		for (int32 Layer = 0; Layer < (TileIndex == 0 ? 2 : 3); ++Layer)
		{
			ULandscapeLayerInfoObject* Info = NewObject<ULandscapeLayerInfoObject>();
			Info->SetLayerName(Layer == 0 ? TEXT("Snow_Dunes") : Layer == 1 ? TEXT("Snow_Rough") : TEXT("Snow_Cliff"), false);
			FWeightmapLayerAllocationInfo Allocation;
			Allocation.LayerInfo = Info;
			Allocation.WeightmapTextureIndex = 0;
			Allocation.WeightmapTextureChannel = TileIndex == 0 ? (Layer == 0 ? 3 : 2) : Layer;
			Component->GetWeightmapLayerAllocations().Add(Allocation);
		}
		Components.Add(Component);
	}
	FTGALandscapeAtlas Atlas = TGALandscapeExport::CaptureAtlas(Landscape);
	TestTrue(*Atlas.Error, Atlas.IsValid());
	TestEqual(TEXT("Shared borders stored once"), Atlas.Width, 127);
	TestEqual(TEXT("Four components form 127x127"), Atlas.Height, 127);
	if (Atlas.IsValid())
	{
		for (int32 Y = 0; Y < 127; ++Y)
		{
			for (int32 X = 0; X < 127; ++X)
			{
				const FColor Expected(uint8(X), uint8(Y), X > 63 && Y > 63 ? 211 : 0, 0);
				if (Atlas.Weights[Y * 127 + X] != Expected)
				{
					AddError(TEXT("Named layer remapping changed a weight or misplaced a texel."));
					break;
				}
			}
		}
		TestTrue(TEXT("World position maps to texel centre"), FMath::IsNearlyEqual(-300 * Atlas.MaskU.X + Atlas.MaskU.W, 0.5 / 127, 1e-10));
		TestTrue(TEXT("Y mapping uses actual Landscape translation"), FMath::IsNearlyEqual(700 * Atlas.MaskV.Y + Atlas.MaskV.W, 0.5 / 127, 1e-10));
	}
	// Exercise the production DDS/material writer, including independent mips.
	Landscape->LandscapeMaterial = NewObject<UMaterial>(GetTransientPackage(), TEXT("MM_Landscape"));
	const FString Output = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("LandscapeAtlasTest"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
	IFileManager::Get().MakeDirectory(*Output, true);
	const FTGALandscapeMaterialExport Material = TGALandscapeExport::ExportMaterial(Landscape, Output, 8, 0, 200);
	TestTrue(*Material.Error, Material.IsValid());
	TArray<uint8> Bytes;
	if (Material.IsValid() && TestTrue(TEXT("Atlas DDS written"), FFileHelper::LoadFileToArray(Bytes, *FPaths::Combine(Output, Material.AtlasName + TEXT(".dds")))))
	{
		TestEqual(TEXT("Linear RGBA DXGI format"), Bytes[128], uint8(28));
		TestEqual(TEXT("127 to 1 mip chain has seven levels"), Bytes[28], uint8(7));
		for (int32 Pixel = 0; Pixel < Atlas.Weights.Num(); ++Pixel)
		{
			const FColor& Expected = Atlas.Weights[Pixel];
			const uint8* Actual = Bytes.GetData() + 148 + Pixel * 4;
			if (Actual[0] != Expected.R || Actual[1] != Expected.G || Actual[2] != Expected.B || Actual[3] != Expected.A)
			{
				AddError(TEXT("Writing the atlas altered base-level RGBA weights."));
				break;
			}
		}
		const TArray<TSharedPtr<FJsonValue>>& MaterialParameters = Material.Material->GetArrayField(TEXT("Parameters"));
		TestEqual(TEXT("Complete paint/mapping/surface/noise parameters"), MaterialParameters.Num(), 18);
		TestEqual(TEXT("Material uses authored parent"), Material.Material->GetStringField(TEXT("Parent")), FString(TEXT("MM_Landscape")));
	}
	TestFalse(TEXT("Zero surface repeat rejected"), TGALandscapeExport::ExportMaterial(Landscape, Output, 0, 0, 200).IsValid());
	IFileManager::Get().DeleteDirectory(*Output, false, true);
	TArray<FTGALandscapePaintTile> Tiles;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		FTGALandscapePaintTile Tile;
		Tile.Name = FString::FromInt(Index);
		Tile.SectionBase = FIntPoint((Index % 2) * 63, (Index / 2) * 63);
		Tile.Weights.Init(FColor(41, 77, 88, 123), 64 * 64);
		Tiles.Add(Tile);
	}
	TestTrue(TEXT("Single component supported"), TGALandscapeExport::ComposeAtlas({Tiles[0]}, FTransform::Identity).IsValid());
	TestTrue(TEXT("2x2 preserves independent canonical alpha"), TGALandscapeExport::ComposeAtlas(Tiles, FTransform::Identity).Weights[0].A == 123);
	Tiles.Pop();
	TestTrue(TEXT("Hole rejected"), TGALandscapeExport::ComposeAtlas(Tiles, FTransform::Identity).Error.Contains(TEXT("holes")));
	Tiles[1].Weights[0].R++;
	TestTrue(TEXT("Different shared border rejected"), TGALandscapeExport::ComposeAtlas(Tiles, FTransform::Identity).Error.Contains(TEXT("border")));
	TestFalse(TEXT("Rotated Landscape rejected"), TGALandscapeExport::ComposeAtlas({Tiles[0]}, FTransform(FRotator(0, 10, 0))).IsValid());
	TestFalse(TEXT("Negative scale rejected"), TGALandscapeExport::ComposeAtlas({Tiles[0]}, FTransform(FQuat::Identity, FVector::ZeroVector, FVector(-1, 1, 1))).IsValid());
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTGALandscapeCurrentSceneTest, "TGA.LandscapeExport.CurrentScene",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGALandscapeCurrentSceneTest::RunTest(const FString& Parameters)
{
	FString Map;
	FString MeshActor;
	FString Output;
	if (!FParse::Value(FCommandLine::Get(), TEXT("AGPLandscapeMap="), Map) ||
		!FParse::Value(FCommandLine::Get(), TEXT("AGPLandscapeMeshActor="), MeshActor) ||
		!FParse::Value(FCommandLine::Get(), TEXT("AGPLandscapeOutput="), Output))
	{
		AddWarning(TEXT("Opt-in current-scene export skipped: supply AGPLandscapeMap, AGPLandscapeMeshActor and a fresh AGPLandscapeOutput directory."));
		return true;
	}
	if (!TestFalse(TEXT("Use a fresh output directory"), FPaths::DirectoryExists(Output)) ||
		!TestTrue(TEXT("Requested map loads"), FEditorFileUtils::LoadMap(Map, false, true)))
	{
		return false;
	}
	UWorld* World = GEditor->GetEditorWorldContext().World();
	ALandscapeProxy* Source = nullptr;
	AActor* Target = nullptr;
	int32 SourceCount = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (ALandscapeProxy* Landscape = Cast<ALandscapeProxy>(*It))
		{
			Source = Landscape;
			++SourceCount;
		}
		if (It->GetActorLabel() == MeshActor || It->GetName() == MeshActor)
		{
			Target = *It;
		}
	}
	if (!TestEqual(TEXT("Exactly one native Landscape"), SourceCount, 1) || !TestNotNull(TEXT("Named mesh actor exists"), Target))
	{
		return false;
	}
	UTGAExportSettingsComponent* Link = Target->GetComponentByClass<UTGAExportSettingsComponent>();
	if (!Link)
	{
		Link = NewObject<UTGAExportSettingsComponent>(Target);
		Target->AddInstanceComponent(Link);
	}
	Link->SourceLandscape = Source;
	UTGAPluginDeveloperSettings* Settings = GetMutableDefault<UTGAPluginDeveloperSettings>();
	TGuardValue<bool> EnableMasks(Settings->ExportLandscapeWeightMaps, true);
	TGuardValue<bool> EnableMaterials(Settings->ExportLandscapeMaterialList, true);
	IFileManager::Get().MakeDirectory(*Output, true);
	FTGAUnrealPluginModule& Exporter = FModuleManager::LoadModuleChecked<FTGAUnrealPluginModule>(TEXT("TGAUnrealPlugin"));
	// Only the export is saved. The loaded map and transient source link are not.
	if (!TestTrue(TEXT("Normal scene export succeeds"), Exporter.ExportLevelToJSON(World, Output)))
	{
		return false;
	}
	TMap<FString, TArray<uint8>> WorkingFiles;
	TArray<FString> Filenames;
	IFileManager::Get().FindFiles(Filenames, *(Output / TEXT("*")), true, false);
	for (const FString& Filename : Filenames)
	{
		if (Filename.EndsWith(TEXT("_Level.json")) || Filename.StartsWith(TEXT("T_AGP_Landscape_")))
		{
			TArray<uint8> Bytes;
			FFileHelper::LoadFileToArray(Bytes, *(Output / Filename));
			WorkingFiles.Add(Filename, MoveTemp(Bytes));
		}
	}
	TArray<ULandscapeComponent*> Components;
	Source->GetComponents(Components);
	const FWeightmapLayerAllocationInfo* ChangedAllocation = Components[0]->GetWeightmapLayerAllocations().FindByPredicate(
		[](const FWeightmapLayerAllocationInfo& Allocation) { return Allocation.IsAllocated(); });
	if (!TestNotNull(TEXT("A painted allocation exists"), ChangedAllocation))
	{
		return false;
	}
	UTexture2D* ChangedTexture = Components[0]->GetWeightmapTextures()[ChangedAllocation->WeightmapTextureIndex];
	TArray64<uint8> Original;
	ChangedTexture->Source.GetMipData(Original, 0);
	TArray64<uint8> Changed = Original;
	// An interior pixel changes the candidate atlas without invalidating borders.
	const int32 SourceByte[] = {2, 1, 0, 3};
	Changed[(ChangedTexture->Source.GetSizeX() + 1) * 4 + SourceByte[ChangedAllocation->WeightmapTextureChannel]] ^= 1;
	const int32 Width = ChangedTexture->Source.GetSizeX();
	const int32 Height = ChangedTexture->Source.GetSizeY();
	ChangedTexture->Source.Init(Width, Height, 1, 1, TSF_BGRA8, Changed.GetData());
	AStaticMeshActor* Conflict = World->SpawnActor<AStaticMeshActor>();
	UTGAExportSettingsComponent* ConflictSettings = NewObject<UTGAExportSettingsComponent>(Conflict);
	Conflict->AddInstanceComponent(ConflictSettings);
	ConflictSettings->SourceLandscape = Source;
	ConflictSettings->SurfaceRepeatMeters = Link->SurfaceRepeatMeters * 2;
	AddExpectedError(TEXT("must use identical surface-repeat/noise controls"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Later conflicting floater fails export"), Exporter.ExportLevelToJSON(World, Output));
	for (const auto& Entry : WorkingFiles)
	{
		TArray<uint8> Bytes;
		FFileHelper::LoadFileToArray(Bytes, *(Output / Entry.Key));
		TestTrue(TEXT("Failed export preserves prior runtime JSON and atlas bytes"), Bytes == Entry.Value);
	}
	ChangedTexture->Source.Init(Width, Height, 1, 1, TSF_BGRA8, Original.GetData());
	World->DestroyActor(Conflict);
	TestTrue(TEXT("Successful export recovers after rejected update"), Exporter.ExportLevelToJSON(World, Output));
	return true;
}

#endif
