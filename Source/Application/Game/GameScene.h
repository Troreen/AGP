#pragma once

#include "MeshLibrary.h"
#include "GameFramework/Scenes/SceneData.h"
#include <filesystem>

// Game-owned scene authoring and synchronous asset-loader registration. Worlds
// are owned by the host; this helper can safely survive scene reloads.
class GameScene final
{
public:
	SceneData Load(const SceneType& name, SceneLoadContext& context);

private:
	void PrepareAssets(SceneData& scene, SceneLoadContext& context);
	MeshLibrary myMeshLibrary;
	std::filesystem::path myContentRoot;
	bool myInitialized = false;
};
