#pragma once

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

class AssetRegistry
{
public:
	using MakeAssetFunc = std::function<std::shared_ptr<Asset>()>;

	enum class AssetError
	{
		None,
		NotFound,
		Ambiguous,
		LoadFailed
	};

	static AssetRegistry& Get();

	AssetRegistry();
	~AssetRegistry();

	template<class T>
		requires std::is_base_of_v<Asset, T>
	std::shared_ptr<T> GetAsset(std::string_view aName)
	{
		myLastError.clear();
		myLastErrorCode = AssetError::None;

		std::string lowerName = GetFullAssetName(aName);

		if (lowerName.empty() || !myAssets.contains(lowerName))
		{
			std::string extension = RegisteredExtension(lowerName);
			lowerName = lowerName.substr(0, lowerName.size() - extension.size());
			if (!myAssetAliases.contains(lowerName) || !myAssets.contains(myAssetAliases[lowerName]))
			{
				SetError("No asset exists with the name: '" + std::string(aName) + "'", AssetError::NotFound);
				return nullptr;
			}

			lowerName = myAssetAliases[lowerName];
		}

		AssetInfo& assetInfo = myAssets.at(lowerName);
		if (assetInfo.Loading)
		{
			SetError("Cyclic asset dependency at '" + assetInfo.Path.string() + "'.");
			return nullptr;
		}

		if (!assetInfo.Asset.expired())
		{
			std::shared_ptr<T> castedAsset = std::dynamic_pointer_cast<T>(assetInfo.Asset.lock());
			if (castedAsset != nullptr)
			{
				return castedAsset;
			}
			else
			{
				SetError("Asset '" + std::string(aName) + "' has a different type.");
				return nullptr;
			}
		}

		const std::string extension = RegisteredExtension(assetInfo.Path);
		if (!myFileExtToAssetType.contains(extension))
		{
			SetError("No loader is registered for '" + assetInfo.Path.string() + "'.");
			return nullptr;
		}

		const std::vector<MakeAssetFunc>& loaders = myFileExtToAssetType.at(extension);
		for (const MakeAssetFunc& loader : loaders)
		{
			assetInfo.Loading = true;

			std::shared_ptr<Asset> asset = loader();
			if (asset->Load(assetInfo.Path, *this))
			{
				asset->myName = assetInfo.Path.stem().string();
				assetInfo.Asset = asset;
				std::shared_ptr<T> castedAsset = std::dynamic_pointer_cast<T>(asset);
				if (castedAsset != nullptr)
				{
					assetInfo.Loading = false;
					return castedAsset;
				}
			}

			assetInfo.Loading = false;
		}

		SetError("No loader works for '" + assetInfo.Path.string() + "' with the given type.");
		return nullptr;
	}

	template<class T>
		requires std::is_base_of_v<Asset, T>
	void RegisterAssetType(const std::vector<std::string>& extensions)
	{
		for (const std::string& extension : extensions)
		{
			std::string lowerExtension = extension;
			std::ranges::transform(lowerExtension, lowerExtension.begin(), tolower);

			if (!myFileExtToAssetType.contains(lowerExtension))
			{
				myFileExtToAssetType.emplace(lowerExtension, std::vector<MakeAssetFunc>());
			}

			myFileExtToAssetType.at(lowerExtension).emplace_back([] { return std::make_shared<T>(); });
		}
	}

	void Initialize(const std::filesystem::path& contentRoot);
	void Clear();

	bool IsInitialized() const { return !myContentRoot.empty(); }
	const std::filesystem::path& GetContentRoot() const { return myContentRoot; }

	const std::string& GetLastError() const { return myLastError; }
	AssetError GetLastErrorCode() const { return myLastErrorCode; }

private:
	struct AssetInfo
	{
		std::filesystem::path Path;
		std::weak_ptr<Asset> Asset;
		bool Loading = false;
	};

	std::string RegisteredExtension(const std::filesystem::path& path) const;
	static std::string NormalizeAssetName(std::string_view value);
	std::string GetFullAssetName(const std::filesystem::path& path) const;
	void SetError(std::string&& aMessage, AssetError aError = AssetError::LoadFailed);

	std::unordered_map<std::string, AssetInfo> myAssets;
	std::unordered_map<std::string, std::vector<MakeAssetFunc>> myFileExtToAssetType;
	std::unordered_map<std::string,std::string> myAssetAliases;

	std::filesystem::path myContentRoot;

	std::string myLastError;
	AssetError myLastErrorCode = AssetError::None;
};
