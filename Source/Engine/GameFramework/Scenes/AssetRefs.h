#pragma once

#include <memory>
#include <string>
#include <vector>

class AssetRegistry;
class Asset;
class MaterialInterface;
class Mesh;
class MeshComponentBase;
class Texture;
class Font;
class GameApplication;
struct MaterialInstanceData;

struct AssetId
{
	std::string Value;
};

// Opaque framework handles keep renderer resource types out of gameplay APIs.
class MeshHandle
{
public:
	explicit operator bool() const { return bool(myResource); }

private:
	std::shared_ptr<Mesh> myResource;
	std::shared_ptr<Asset> myAsset;
	friend class AssetRegistry;
	friend class MeshComponentBase;
};

class MaterialHandle
{
public:
	explicit operator bool() const { return bool(myResource); }

private:
	std::shared_ptr<MaterialInterface> myResource;
	std::shared_ptr<Asset> myAsset;
	std::vector<std::shared_ptr<Asset>> myTextureAssets;
	friend class AssetRegistry;
	friend class MeshComponentBase;
	friend MaterialHandle CreateMaterialInstance(AssetRegistry&, const MaterialInstanceData&);
};

class TextureHandle
{
public:
	explicit operator bool() const { return bool(myResource); }

private:
	std::shared_ptr<Texture> myResource;
	std::shared_ptr<Asset> myAsset;
	friend class AssetRegistry;
	friend MaterialHandle CreateMaterialInstance(AssetRegistry&, const MaterialInstanceData&);
};

class FontHandle
{
public:
	explicit operator bool() const { return bool(myResource); }

private:
	std::shared_ptr<Font> myResource;
	std::shared_ptr<Asset> myAsset;
	friend class AssetRegistry;
	friend class GameApplication;
};

MaterialHandle CreateMaterialInstance(AssetRegistry& assets, const MaterialInstanceData& data);
