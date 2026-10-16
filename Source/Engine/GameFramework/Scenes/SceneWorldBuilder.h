#pragma once
#include "GameFramework/Components/MeshComponentBase.h"
#include "GameFramework/Scenes/SceneData.h"
#include "Vector2.hpp"

#include <filesystem>
#include <memory>
#include <functional>

class AssetRegistry;
class MaterialAsset;
class MeshAsset;
class World;
class CustomComponent;

struct SceneFallbackAssets
{
	std::shared_ptr<MeshAsset> MissingMesh;
	std::shared_ptr<MaterialAsset> MissingMaterial;
	std::filesystem::path MissingShader;
};

using CustomComponentCreator = std::function<CustomComponent*(Actor&, const CustomComponentData&)>;

class SceneWorldBuilder
{
public:
	SceneWorldBuilder() = default;
	~SceneWorldBuilder() = default;

	bool Initialize(const std::filesystem::path& aNameConversions);
	std::unique_ptr<World> BuildWorld(const SceneData& aScene, AssetRegistry& anAssetRegistry,
	    const std::filesystem::path& aShaderPath, CommonUtilities::Vector2u aClientSize,
	    const SceneFallbackAssets& someFallbacks = {},
	    const CustomComponentCreator& aCustomCreator = {});

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

	void CreateSceneMaterials(const std::vector<MasterMaterialRecord>& someMaterials, AssetRegistry& anAssetRegistry, const std::filesystem::path& aShaderPath, const SceneFallbackAssets& someFallbacks);
	void CreateActorsAndComponents(World& aWorld, const std::vector<ActorRecord>& someActors, AssetRegistry& anAssetRegistry, CommonUtilities::Vector2u aClientSize, const SceneFallbackAssets& someFallbacks, const CustomComponentCreator& aCustomCreator);
	const ComponentData& GetCommonComponentData(const ComponentRecord& aRecord);
	void ApplyCommonComponentData(Component& aComponent, const ComponentData& someComponentData);
	std::shared_ptr<MaterialAsset> LoadMaterial(const MaterialInstanceData& someMaterialData, AssetRegistry& anAssetRegistry, const std::string& aComponentName);
	bool ApplyMesh(Actor& anActor, MeshComponentBase& aComponent, const StaticMeshData& someMeshData, AssetRegistry& anAssetRegistry, const SceneFallbackAssets& someFallbacks);
	Component* CreateComponent(Actor& anActor, const ComponentRecord& aRecord, AssetRegistry& anAssetRegistry, const SceneFallbackAssets& someFallbacks);

	std::unordered_map<std::string, std::string> myUnrealDataNameToMaterialParameterName;
};
