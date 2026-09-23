#pragma once

#include "MeshLibrary.h"
#include "GameFramework/Scenes/SceneData.h"
#include <filesystem>

// Loads exported game scenes and prepares their assets for runtime use.
class GameScene final
{
public:
	void InitializeScene(SceneLoadContext& aSceneLoadContext);
	SceneData Load(SceneType aSceneType, SceneLoadContext& aSceneLoadContext);

private:
	void PrepareAssets(SceneData& aSceneData, SceneLoadContext& aSceneLoadContext);
	void PrepareMaterial(MaterialInstanceData& aMaterialData, const AssetId& aFallbackMaterial, AssetRegistry& aAssetRegistry);
	MeshLibrary myMeshLibrary;
	std::filesystem::path myContentRoot;
	bool myIsInitialized = false;
};
