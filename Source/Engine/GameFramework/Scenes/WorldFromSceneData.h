#pragma once
#include "GameFramework/Scenes/SceneData.h"
#include "Vector2.hpp"

#include <memory>

class AssetRegistry;
class MaterialAsset;
class MeshAsset;
class World;

struct SceneFallbackAssets
{
	std::shared_ptr<MeshAsset> MissingMesh;
	std::shared_ptr<MaterialAsset> MissingMaterial;
};

std::unique_ptr<World> BuildWorldFromSceneData(const SceneData& aScene, AssetRegistry& anAssetRegistry, CommonUtilities::Vector2u aClientSize, const SceneFallbackAssets& someFallbacks = {});
