#pragma once
#include <filesystem>
#include <unordered_map>
#include "MeshLibrary.h"
#include "GameFramework/Scenes/SceneData.h"
class MaterialInterface;

// Game-owned scene authoring and the temporary synchronous asset adapter. Worlds
// are owned by the host; this helper can safely survive any number of scene reloads.
class GameScene final
{
public:
	SceneData Load(const std::string& name, SceneLoadContext& context);

private:
	std::shared_ptr<MaterialInterface> GetMaterial(const std::filesystem::path& file);
	void PrepareAssets(SceneData& scene, SceneLoadContext& context);
	MeshLibrary myMeshLibrary;

	std::filesystem::path myContentRoot;
	bool myInitialized = false;
	std::unordered_map<std::string, std::shared_ptr<MaterialInterface>> myMaterialCache;
};
