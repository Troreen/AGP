#include "GameScene.h"
#include "GameLog.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AssetHandling/MaterialAsset.h"
#include "GameFramework/UnrealSceneImporter/UnrealSceneImporter.h"

#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace
{
	std::filesystem::path GetSceneFile(const std::string& name)
	{
		if (name == "Game") return "ExportedScenes/TestExportMap_Level.json";
		if (name == "ChestMaterials") return "ExportedScenes/ChestMaterials_Level.json";
		if (name == "Lvl_Blockout_Level") return "ExportedScenes/Lvl_Blockout_Level.json";
		return {};
	}

	std::string FormatDiagnostics(const char* heading, const std::vector<ImportDiagnostic>& diagnostics)
	{
		std::ostringstream message;
		message << heading;
		for (const auto& diagnostic : diagnostics)
		{
			message << "\n - " << diagnostic.Context << ": " << diagnostic.Message;
		}
		return message.str();
	}
}

SceneData GameScene::Load(const std::string& name, SceneLoadContext& context)
{
	const std::filesystem::path sceneFile = GetSceneFile(name);
	if (sceneFile.empty())
	{
		throw std::runtime_error("Unknown Game scene: " + name);
	}
	if (!myInitialized)
	{
		myContentRoot = context.ContentRoot;
		myMeshLibrary.Initialize(myContentRoot);
		context.Assets.SetMeshLoader([this](const std::filesystem::path& path)
		{
			return myMeshLibrary.LoadMesh(path);
		});
		context.Assets.RegisterMesh(AssetId{"/Engine/BasicShapes/Plane.Plane"}, myMeshLibrary.GetMesh("Floor"));
		context.Assets.RegisterMesh(AssetId{"/Engine/BasicShapes/Cube.Cube"}, myMeshLibrary.GetMesh("Cube"));
		myInitialized = true;
	}

	const UnrealImportResult imported = UnrealSceneImporter{}.ImportScene(myContentRoot / sceneFile);
	if (!imported)
	{
		throw std::runtime_error(FormatDiagnostics("Scene import failed:", imported.Diagnostics));
	}

	SceneData scene = std::move(*imported.Data);
	PrepareAssets(scene, context);
	GAMELOG(Log, "Loaded scene '{}' from '{}' ({} actors).", name, sceneFile.string(), scene.Actors.size());
	return scene;
}

void GameScene::PrepareAssets(SceneData& scene, SceneLoadContext& context)
{
	const AssetId fallbackMaterial{"Shaders/CubeMaterial.mat"}; // TODO: change this into purple black missing texture material 
	if (!context.Assets.ResolveMaterial(fallbackMaterial))
	{
		throw std::runtime_error("Could not create the fallback material used by imported scene assets: " +
		                         context.Assets.GetLastError());
	}

	for (ActorRecord& actor : scene.Actors)
	{
		for (ComponentRecord& componentRecord : actor.Components)
		{
			std::visit([&](auto& componentData)
			{
				using ComponentType = std::decay_t<decltype(componentData)>;
				if constexpr (std::is_same_v<ComponentType, StaticMeshData> || std::is_same_v<ComponentType, SkeletalMeshData>)
				{
					// The imported name selects an authored .mat; Unreal Parent is metadata.
					for (size_t materialSlot = 0; materialSlot < componentData.Materials.size(); ++materialSlot)
					{
						MaterialInstanceData& materialData = componentData.Materials[materialSlot];
						if (context.Assets.GetAsset<MaterialAsset>(materialData.Name))
						{
							materialData.Parent = AssetId{materialData.Name};
							continue;
						}
						if (context.Assets.GetLastErrorCode() != AssetRegistry::AssetError::NotFound)
						{
							throw std::runtime_error("Could not load authored material '" + materialData.Name + "': " + context.Assets.GetLastError());
						}
						GAMELOG(Warning, "No authored .mat for imported material '{}'; using fallback.", materialData.Name);
						materialData.Parent = fallbackMaterial;
						materialData.Parameters.clear();
					}
				}
			}, componentRecord);
		}
	}
}
