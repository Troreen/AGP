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

void GameScene::InitializeScene(SceneLoadContext& aSceneLoadContext)
{
	myContentRoot = aSceneLoadContext.ContentRoot;

    myMeshLibrary.Initialize(myContentRoot);

    /*aSceneLoadContext.Assets.SetMeshLoader(
        [this](const std::filesystem::path& path)
        {
            return myMeshLibrary.LoadMesh(path);
        });

    aSceneLoadContext.Assets.RegisterMesh(
        AssetId{"/Engine/BasicShapes/Plane.Plane"},
        myMeshLibrary.GetMesh("Floor"));

    aSceneLoadContext.Assets.RegisterMesh(
        AssetId{"/Engine/BasicShapes/Cube.Cube"},
        myMeshLibrary.GetMesh("Cube"));*/

    myIsInitialized = true;
}

SceneData GameScene::Load(SceneType aSceneType , SceneLoadContext& aSceneLoadContext)
{
	
	const std::filesystem::path sceneFile = GetSceneFile(aSceneType);
	if (sceneFile.empty())
	{
		throw std::runtime_error("Unknown Game scene: " + GetSceneName(aSceneType));
	}

	if (!myIsInitialized)
	{
		InitializeScene(aSceneLoadContext);
	}
	
	UnrealImportResult importResult = UnrealSceneImporter{}.ImportScene(myContentRoot / sceneFile);
	if (!importResult)
	{
		throw std::runtime_error(FormatDiagnostics("Scene import failed:", importResult.Diagnostics));
	}

	SceneData sceneData = std::move(*importResult.Data);
	PrepareAssets(sceneData, aSceneLoadContext);
	GAMELOG(Log, "Loaded scene '{}' from '{}' ({} actors).", GetSceneName(aSceneType), sceneFile.string(), sceneData.Actors.size());
	return sceneData;
}

void GameScene::PrepareAssets(SceneData& aSceneData, SceneLoadContext& aSceneLoadContext)
{
	const std::string fallbackMaterial{"Shaders/_DefaultMaterial.mat"}; // TODO: change this into purple black missing texture material

	for (ActorRecord& actor : aSceneData.Actors)
	{
		for (ComponentRecord& component : actor.Components)
		{
			std::visit([&](auto& componentData)
			{
				using ComponentType = std::decay_t<decltype(componentData)>;
				if constexpr (std::is_same_v<ComponentType, StaticMeshData> || std::is_same_v<ComponentType, SkeletalMeshData>)
				{
					for (MaterialInstanceData& materialData : componentData.Materials)
					{
						PrepareMaterial(materialData, fallbackMaterial, aSceneLoadContext.Assets);
					}
				}
			}, component);
		}
	}
}

void GameScene::PrepareMaterial(MaterialInstanceData& aMaterialData, const std::string& aFallbackMaterial, AssetRegistry& aAssetRegistry)
{
	if (aAssetRegistry.GetAsset<MaterialAsset>(aMaterialData.Name))
	{
		return;
	}

	if (aAssetRegistry.GetLastErrorCode() != AssetRegistry::AssetError::NotFound)
    {
        throw std::runtime_error(
            "Could not load authored material '" +
            aMaterialData.Name +
            "': " +
            aAssetRegistry.GetLastError());
    }

    GAMELOG(Warning, "No authored .mat for imported material '{}'; using fallback.", aMaterialData.Name);

    aMaterialData.Name = aFallbackMaterial;
    aMaterialData.Parameters.clear();
}
