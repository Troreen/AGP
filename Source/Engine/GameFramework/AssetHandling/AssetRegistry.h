#pragma once

#include "GameFramework/Scenes/AssetRefs.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class MaterialInterface;
class Mesh;
class Texture;
class Font;

class AssetRegistry
{
public:
	using MeshLoader = std::function<std::shared_ptr<Mesh>(const std::filesystem::path&)>;

	static AssetRegistry& Get();

	void Initialize(const std::filesystem::path& contentRoot);
	void Clear();
	bool IsInitialized() const { return !myContentRoot.empty(); }
	const std::filesystem::path& GetContentRoot() const { return myContentRoot; }

	void SetMeshLoader(MeshLoader loader) { myMeshLoader = std::move(loader); }
	void RegisterMesh(const AssetId& id, std::shared_ptr<Mesh> mesh);
	void RegisterMaterial(const AssetId& id, std::shared_ptr<MaterialInterface> material);
	void RegisterTexture(const AssetId& id, std::shared_ptr<Texture> texture);
	void RegisterFont(const AssetId& id, std::shared_ptr<Font> font);

	MeshAsset ResolveMesh(const AssetId& id);
	MaterialAsset ResolveMaterial(const AssetId& id);
	TextureAsset ResolveTexture(const AssetId& id);
	FontAsset ResolveFont(const AssetId& id);

	AssetId MakeAssetId(const std::filesystem::path& path) const;
	static std::string NormalizeId(std::string_view value);
	const std::string& GetLastError() const { return myLastError; }

private:
	template<class T> struct Entry
	{
		std::filesystem::path Path;
		std::weak_ptr<T> Cached;
		std::shared_ptr<T> Pinned;
	};

	template<class T>
	using EntryMap = std::unordered_map<std::string, Entry<T>>;
	using AliasMap = std::unordered_map<std::string, std::vector<std::string>>;

	void IndexFile(const std::filesystem::path& path);
	template<class T>
	Entry<T>* FindEntry(const AssetId& id, EntryMap<T>& entries, const AliasMap& aliases, std::string_view expectedExtension);
	std::shared_ptr<MaterialInterface> LoadMaterial(const std::filesystem::path& path);
	std::shared_ptr<Texture> LoadTexture(const std::filesystem::path& path);
	std::shared_ptr<Font> LoadFont(const std::filesystem::path& path);
	void SetError(std::string message);

	std::filesystem::path myContentRoot;
	EntryMap<Mesh> myMeshes;
	EntryMap<MaterialInterface> myMaterials;
	EntryMap<Texture> myTextures;
	EntryMap<Font> myFonts;
	AliasMap myMeshAliases;
	AliasMap myMaterialAliases;
	AliasMap myTextureAliases;
	AliasMap myFontAliases;
	MeshLoader myMeshLoader;
	std::string myLastError;
};
