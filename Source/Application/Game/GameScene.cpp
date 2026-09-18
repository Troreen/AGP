#include "GameScene.h"
#include "GameLog.h"
#include "GameFramework/UnrealSceneImporter/UnrealSceneAdapter.h"
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
	const std::filesystem::path GameSceneFile = "ExportedScenes/lvl_blockout/Lvl_Blockout_Level.json";

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

	const auto imported = UnrealSceneImporter{}.ImportScene(myContentRoot / GameSceneFile);
	if (!imported)
	{
		throw std::runtime_error(FormatDiagnostics("Scene import failed:", imported.Diagnostics));
	}

	auto converted = UnrealSceneAdapter{}.Convert(*imported.Data);
	if (!converted)
	{
		throw std::runtime_error(FormatDiagnostics("Scene conversion failed:", converted.Diagnostics));
	}

	SceneData scene = std::move(*converted.Scene);
	PrepareAssets(scene, context);
	GAMELOG(Log, "Loaded scene '{}' from '{}' ({} actors).", name, GameSceneFile.string(), scene.Actors.size());
	return scene;
}

void GameScene::PrepareAssets(SceneData& scene, SceneLoadContext& context)
{
	const auto material = GetMaterial(myContentRoot / "Shaders/CubeMaterial.mat");
	if (!material)
	{
		throw std::runtime_error("Could not create the fallback material used by imported scene assets");
	}
	const AssetId materialId{"ImportedSceneMaterial"};
	context.Assets.BindMaterial(materialId, material);

	for (auto& actor : scene.Actors)
	{
		actor.Components.erase(std::remove_if(actor.Components.begin(), actor.Components.end(), [&](ComponentRecord& record)
		{
			return std::visit([&](auto& data)
			{
				using T = std::decay_t<decltype(data)>;
				if constexpr (!std::is_same_v<T, StaticMeshData> && !std::is_same_v<T, SkeletalMeshData>)
				{
					return false;
				}
				else
				{
					auto mesh = myMeshLibrary.LoadSceneMesh(data.MeshName, data.ContentPath);
					if (!mesh)
					{
						GAMELOG(Warning, "Skipping unavailable imported mesh '{}' ({}) on actor '{}'.",
						        data.MeshName, data.ContentPath, actor.Name);
						return true;
					}
					context.Assets.BindMesh(data.Mesh, mesh);
					data.Materials.assign(mesh->GetNumMaterialSlots(), MaterialInstanceData{});
					for (size_t slot = 0; slot < data.Materials.size(); ++slot)
					{
						data.Materials[slot].Name = data.MeshName + " Material " + std::to_string(slot);
						data.Materials[slot].Parent = materialId;
					}
					return false;
				}
			}, record);
		}), actor.Components.end());
	}
}

std::shared_ptr<MaterialInterface> GameScene::GetMaterial(const std::filesystem::path& file)
{
	const std::string key = file.lexically_normal().string();
	if (const auto it = myMaterialCache.find(key); it != myMaterialCache.end())
	{
		return it->second;
	}
	MaterialDescription description;
	if (!LoadMaterialDescription(file, description))
	{
		GAMELOG(Warning, "Could not load material description '{}'.", file.string());
		return nullptr;
	}
	auto material = std::make_shared<Material>();
	if (!GraphicsEngine::Get().CreateMaterial(description, *material))
	{
		return nullptr;
	}
	return myMaterialCache.emplace(key, material).first->second;
}
