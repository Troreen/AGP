#pragma once

#include "GameFramework/Scenes/AssetRefs.h"
#include "Asset.h"

#include <filesystem>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <type_traits>

class MaterialInterface;
class Mesh;
class Texture;
class Font;
class MeshAsset;
class FontAsset;
class TextureAsset;

class AssetRegistry
{
public:
	enum class AssetError { None, NotFound, Ambiguous, LoadFailed };
	using MakeAssetFunc = std::function<std::shared_ptr<Asset>()>;
	using MeshLoader = std::function<std::shared_ptr<Mesh>(const std::filesystem::path&)>;

	static AssetRegistry& Get();
	AssetRegistry();

	template<class T> requires std::is_base_of_v<Asset, T>
	std::shared_ptr<T> GetAsset(std::string_view name)
	{
		myLastError.clear();
		myLastErrorCode = AssetError::None;
		std::vector<std::string> extensions;
		for (const auto& [extension, loaders] : myFileExtToAssetType)
			for (const MakeAssetFunc& loader : loaders)
				if (std::dynamic_pointer_cast<T>(loader())) { extensions.push_back(extension); break; }
		AssetInfo* info = FindAssetInfo(name, extensions, [](const Asset& asset) { return dynamic_cast<const T*>(&asset) != nullptr; });
		if (!info) return {};
		if (info->Pinned)
		{
			if (auto typed = std::dynamic_pointer_cast<T>(info->Pinned)) return typed;
			SetError("Asset '" + std::string(name) + "' has a different type.");
			return {};
		}
		if (info->Loading)
		{
			SetError("Cyclic asset dependency at '" + info->Path.string() + "'.");
			return {};
		}
		if (auto cached = info->Cached.lock())
		{
			if (auto typed = std::dynamic_pointer_cast<T>(cached)) return typed;
			SetError("Asset '" + std::string(name) + "' has a different type.");
			return {};
		}
		const std::string extension = RegisteredExtension(info->Path);
		const auto loaders = myFileExtToAssetType.find(extension);
		if (loaders == myFileExtToAssetType.end())
		{
			SetError("No loader is registered for '" + info->Path.string() + "'.");
			return {};
		}
		info->Loading = true;
		struct LoadingGuard { AssetInfo& Entry; ~LoadingGuard() { Entry.Loading = false; } } loadingGuard{*info};
		for (const MakeAssetFunc& loader : loaders->second)
		{
			std::shared_ptr<Asset> asset = loader();
			if (!asset || !std::dynamic_pointer_cast<T>(asset)) continue;
			bool loaded = false;
			try { loaded = asset->Load(info->Path, *this); }
			catch (const std::bad_alloc&) { throw; }
			catch (const std::exception& error) { SetError(error.what()); }
			if (loaded)
			{
				const std::string fileName = info->Path.filename().string();
				asset->myName = fileName.substr(0, fileName.size() - extension.size());
				info->Cached = asset;
				myLastError.clear();
				myLastErrorCode = AssetError::None;
				return std::static_pointer_cast<T>(asset);
			}
		}
		const std::string cause = myLastError;
		SetError("Could not load asset '" + info->Path.string() + "'" + (cause.empty() ? "." : ": " + cause));
		return {};
	}

	template<class T> requires std::is_base_of_v<Asset, T>
	void RegisterAssetType(const std::vector<std::string>& extensions)
	{
		for (const std::string& extension : extensions)
			myFileExtToAssetType[NormalizeId(extension)].push_back([] { return std::make_shared<T>(); });
	}

	void Initialize(const std::filesystem::path& contentRoot);
	void Clear();
	bool IsInitialized() const { return !myContentRoot.empty(); }
	const std::filesystem::path& GetContentRoot() const { return myContentRoot; }

	void SetMeshLoader(MeshLoader loader) { myMeshLoader = std::move(loader); }
	void RegisterMesh(const AssetId& id, std::shared_ptr<Mesh> mesh);
	void RegisterMaterial(const AssetId& id, std::shared_ptr<MaterialInterface> material);
	void RegisterTexture(const AssetId& id, std::shared_ptr<Texture> texture);
	void RegisterFont(const AssetId& id, std::shared_ptr<Font> font);

	MeshHandle ResolveMesh(const AssetId& id);
	MaterialHandle ResolveMaterial(const AssetId& id);
	TextureHandle ResolveTexture(const AssetId& id);
	FontHandle ResolveFont(const AssetId& id);

	AssetId MakeAssetId(const std::filesystem::path& path) const;
	static std::string NormalizeId(std::string_view value);
	const std::string& GetLastError() const { return myLastError; }
	AssetError GetLastErrorCode() const { return myLastErrorCode; }

private:
	struct AssetInfo
	{
		std::filesystem::path Path;
		std::weak_ptr<Asset> Cached;
		std::shared_ptr<Asset> Pinned;
		bool Loading = false;
	};
	AssetInfo* FindAssetInfo(std::string_view name, const std::vector<std::string>& extensions,
	                         const std::function<bool(const Asset&)>& matchesType);
	std::string RegisteredExtension(const std::filesystem::path& path) const;
	void PinAsset(const AssetId& id, std::shared_ptr<Asset> asset);
	std::shared_ptr<Mesh> LoadMesh(const std::filesystem::path& path);
	std::shared_ptr<Font> LoadFont(const std::filesystem::path& path, std::shared_ptr<TextureAsset>& atlasAsset);
	friend class MeshAsset;
	friend class FontAsset;
	std::unordered_map<std::string, AssetInfo> myAssets;
	std::unordered_map<std::string, std::vector<MakeAssetFunc>> myFileExtToAssetType;
	std::unordered_map<std::string, std::vector<std::string>> myAssetAliases;

	void IndexFile(const std::filesystem::path& path);
	void SetError(std::string message);

	std::filesystem::path myContentRoot;
	MeshLoader myMeshLoader;
	std::string myLastError;
	AssetError myLastErrorCode = AssetError::None;
};
