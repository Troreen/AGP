#include "GameScene.h"
#include "GameLog.h"
#include "GameFramework/UnrealSceneImporter/UnrealSceneImporter.h"
#include "GraphicsEngine/GraphicsEngine.h"
#include "GraphicsEngine/Materials/Material.h"
#include "GraphicsEngine/Objects/Mesh.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace
{
	constexpr const char* GameSceneName = "Game";
	const std::filesystem::path GameSceneFile = "ExportedScenes/TestExportMap_Level.json";

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
	if (name != GameSceneName)
	{
		throw std::runtime_error("Unknown Game scene: " + name);
	}
	if (!myInitialized)
	{
		myContentRoot = context.ContentRoot;
		myMeshLibrary.Initialize(myContentRoot);
		myInitialized = true;
	}

	const UnrealImportResult imported = UnrealSceneImporter{}.ImportScene(myContentRoot / GameSceneFile);
	if (!imported)
	{
		throw std::runtime_error(FormatDiagnostics("Scene import failed:", imported.Diagnostics));
	}

	SceneData scene = std::move(*imported.Data);
	PrepareAssets(scene, context);
	GAMELOG(Log, "Loaded scene '{}' from '{}' ({} actors).", name, GameSceneFile.string(), scene.Actors.size());
	return scene;
}

void GameScene::PrepareAssets(SceneData& scene, SceneLoadContext& context)
{
	const std::shared_ptr<MaterialInterface> material = GetMaterial(myContentRoot / "Shaders/CubeMaterial.mat");
	if (!material)
	{
		throw std::runtime_error("Could not create the fallback material used by imported scene assets");
	}
	const AssetId materialId{"ImportedSceneMaterial"};
	context.Assets.BindMaterial(materialId, material);

	for (ActorRecord& actor : scene.Actors)
	{
		std::erase_if(actor.Components, [&](ComponentRecord& componentRecord)
		{
			return std::visit([&](auto& componentData)
			{
				using ComponentType = std::decay_t<decltype(componentData)>;
				if constexpr (!std::is_same_v<ComponentType, StaticMeshData> && !std::is_same_v<ComponentType, SkeletalMeshData>)
				{
					return false;
				}
				else
				{
					std::shared_ptr<Mesh> mesh = myMeshLibrary.LoadSceneMesh(componentData.MeshName, componentData.ContentPath);
					if (!mesh)
					{
						GAMELOG(Warning, "Skipping unavailable imported mesh '{}' ({}) on actor '{}'.",
						        componentData.MeshName, componentData.ContentPath, actor.Name);
						return true;
					}
					context.Assets.BindMesh(componentData.Mesh, mesh);
					componentData.Materials.assign(mesh->GetNumMaterialSlots(), MaterialInstanceData{});
					for (size_t materialSlot = 0; materialSlot < componentData.Materials.size(); ++materialSlot)
					{
						MaterialInstanceData& materialData = componentData.Materials[materialSlot];
						materialData.Name = componentData.MeshName + " Material " + std::to_string(materialSlot);
						materialData.Parent = materialId;
					}
					return false;
				}
			}, componentRecord);
		});
	}
}

std::shared_ptr<MaterialInterface> GameScene::GetMaterial(const std::filesystem::path& file)
{
	const std::string key = file.lexically_normal().string();
	if (const auto materialIt = myMaterialCache.find(key); materialIt != myMaterialCache.end())
	{
		return materialIt->second;
	}
	MaterialDescription description;
	if (!LoadMaterialDescription(file, description))
	{
		GAMELOG(Warning, "Could not load material description '{}'.", file.string());
		return nullptr;
	}
	std::shared_ptr<Material> material = std::make_shared<Material>();
	if (!GraphicsEngine::Get().CreateMaterial(description, *material))
	{
		GAMELOG(Warning, "Could not initialize material '{}' from '{}' using shader include '{}'.",
		        description.Name, file.string(), description.MaterialShaderCode.string());
		return nullptr;
	}
	return myMaterialCache.emplace(key, material).first->second;
}
