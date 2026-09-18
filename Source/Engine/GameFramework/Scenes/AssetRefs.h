#pragma once

#include <memory>
#include <string>

class AssetRegistry;
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
class MeshAsset
{
public:
	explicit operator bool() const { return bool(myResource); }

private:
	std::shared_ptr<Mesh> myResource;
	friend class AssetRegistry;
	friend class MeshComponentBase;
};

class MaterialAsset
{
public:
	explicit operator bool() const { return bool(myResource); }

private:
	std::shared_ptr<MaterialInterface> myResource;
	friend class AssetRegistry;
	friend class MeshComponentBase;
	friend MaterialAsset CreateMaterialInstance(AssetRegistry&, const MaterialInstanceData&);
};

class TextureAsset
{
public:
	explicit operator bool() const { return bool(myResource); }

private:
	std::shared_ptr<Texture> myResource;
	friend class AssetRegistry;
	friend MaterialAsset CreateMaterialInstance(AssetRegistry&, const MaterialInstanceData&);
};

class FontAsset
{
public:
	explicit operator bool() const { return bool(myResource); }

private:
	std::shared_ptr<Font> myResource;
	friend class AssetRegistry;
	friend class GameApplication;
};

MaterialAsset CreateMaterialInstance(AssetRegistry& assets, const MaterialInstanceData& data);
