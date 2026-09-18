#pragma once
#include <memory>
#include <string>
#include <unordered_map>

class Mesh;
class MaterialInterface;
class MeshComponentBase;
class Texture;
struct MaterialInstanceData;
class IAssetResolver;

struct AssetId
{
	std::string Value;
};

// Ready resources from the existing renderer. Gameplay binds them without exposing backend APIs.
class MeshAsset
{
public:
	explicit operator bool() const
	{
		return bool(myResource);
	}

private:
	std::shared_ptr<Mesh> myResource;
	friend class AssetLibrary;
	friend class MeshComponentBase;
};

class MaterialAsset
{
public:
	explicit operator bool() const
	{
		return bool(myResource);
	}

private:
	std::shared_ptr<MaterialInterface> myResource;
	friend class AssetLibrary;
	friend class MeshComponentBase;
	friend MaterialAsset CreateMaterialInstance(IAssetResolver&, const MaterialInstanceData&);
};

class IAssetResolver
{
public:
	virtual ~IAssetResolver() = default;
	virtual MeshAsset ResolveMesh(const AssetId& id) = 0;
	virtual MaterialAsset ResolveParentMaterial(const AssetId& id) = 0;
	virtual std::shared_ptr<Texture> ResolveTexture(const AssetId& id) = 0;
};

class AssetLibrary final : public IAssetResolver
{
public:
	void BindMesh(const AssetId& id, std::shared_ptr<Mesh> mesh)
	{
		MeshAsset asset;
		asset.myResource = std::move(mesh);
		myMeshes[id.Value] = std::move(asset);
	}

	void BindMaterial(const AssetId& id, std::shared_ptr<MaterialInterface> material)
	{
		MaterialAsset asset;
		asset.myResource = std::move(material);
		myMaterials[id.Value] = std::move(asset);
	}

	MeshAsset FindMesh(const AssetId& id) const
	{
		const auto it = myMeshes.find(id.Value);
		return it == myMeshes.end() ? MeshAsset{} : it->second;
	}

	MaterialAsset FindMaterial(const AssetId& id) const
	{
		const auto it = myMaterials.find(id.Value);
		return it == myMaterials.end() ? MaterialAsset{} : it->second;
	}
	void BindTexture(const AssetId& id, std::shared_ptr<Texture> texture) { myTextures[id.Value] = std::move(texture); }
	MeshAsset ResolveMesh(const AssetId& id) override { return FindMesh(id); }
	MaterialAsset ResolveParentMaterial(const AssetId& id) override { return FindMaterial(id); }
	std::shared_ptr<Texture> ResolveTexture(const AssetId& id) override
	{
		const auto it = myTextures.find(id.Value); return it == myTextures.end() ? nullptr : it->second;
	}

private:
	std::unordered_map<std::string, MeshAsset> myMeshes;
	std::unordered_map<std::string, MaterialAsset> myMaterials;
	std::unordered_map<std::string, std::shared_ptr<Texture>> myTextures;
};

MaterialAsset CreateMaterialInstance(IAssetResolver& assets, const MaterialInstanceData& data);
