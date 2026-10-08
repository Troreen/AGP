#pragma once
#include "GameFramework/Components/MeshComponentBase.h"
#include "GameFramework/Scenes/SceneData.h"
#include "Vector2.hpp"

#include <filesystem>
#include <memory>


class AssetRegistry;
class MaterialAsset;
class MeshAsset;
class World;

struct SceneFallbackAssets
{
	std::shared_ptr<MeshAsset> MissingMesh;
	std::shared_ptr<MaterialAsset> MissingMaterial;
	std::filesystem::path MissingShader;
};

class WorldFromSceneConverter
{
public:
	WorldFromSceneConverter() = default;
	~WorldFromSceneConverter() = default;

	bool Initialize(const std::filesystem::path& aNameConversions);
	std::unique_ptr<World> BuildWorldFromSceneData(const SceneData& aScene, AssetRegistry& anAssetRegistry, const std::filesystem::path& aShaderPath, CommonUtilities::Vector2u aClientSize, const SceneFallbackAssets& someFallbacks = {});

private:
	class PlaceholderComponent final : public SceneComponent
	{
	public:
		explicit PlaceholderComponent(PlaceholderComponentType aType, PlaceholderProperties someProperties)
			: Type(aType), Properties(std::move(someProperties))
		{
		}
		PlaceholderComponentType Type;
		PlaceholderProperties Properties;
	};

	const ComponentData& Common(const ComponentRecord& aRecord);
	void ApplyComponentData(Component& aComponent, const ComponentData& someData);
	bool ApplyMesh(Actor& anActor, MeshComponentBase& aComponent, const StaticMeshData& someData, AssetRegistry& anAssetRegistry, const SceneFallbackAssets& someFallbacks);
	Component* CreateComponent(Actor& anActor, const ComponentRecord& aRecord, AssetRegistry& anAssetRegistry, CommonUtilities::Vector2u aClientSize, const SceneFallbackAssets& someFallbacks);

	std::unordered_map<std::string, std::string> myUnrealDataNameToMaterialParameterName;
	std::unordered_map<std::string, unsigned> myUnrealParameterNameToTextureSlot;
};
