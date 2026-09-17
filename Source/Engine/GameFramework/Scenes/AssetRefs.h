#pragma once
#include <memory>
#include <string>
#include <unordered_map>

class Mesh;
class MaterialInterface;
class MeshComponentBase;

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
};

class AssetLibrary
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

private:
	std::unordered_map<std::string, MeshAsset> myMeshes;
	std::unordered_map<std::string, MaterialAsset> myMaterials;
};
