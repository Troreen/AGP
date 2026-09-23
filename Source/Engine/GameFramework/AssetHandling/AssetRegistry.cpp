#include "GameFramework/AssetHandling/AssetRegistry.h"

#include "GameFramework/SimdJson/simdjson.h"
#include "GraphicsEngine/Objects/Font.h"
#include "GameFramework/AssetHandling/MaterialAsset.h"
#include "GameFramework/AssetHandling/TextureAsset.h"
#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GameFramework/AssetHandling/FontAsset.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <cmath>
#include <ranges>

namespace
{
	std::string Lower(std::string&& value)
	{
		std::ranges::transform(value, value.begin(), tolower);
		return value;
	}
}

AssetRegistry::AssetRegistry()
{
	RegisterAssetType<MeshAsset>({ ".fbx" });
	RegisterAssetType<MaterialAsset>({ ".mat" });
	RegisterAssetType<TextureAsset>({ ".dds", ".png", ".jpg", ".jpeg" });
	RegisterAssetType<FontAsset>({ ".font.json" });
}

AssetRegistry::~AssetRegistry() = default;

void AssetRegistry::Initialize(const std::filesystem::path& contentRoot)
{
	Clear();

	std::error_code error;
	myContentRoot = std::filesystem::weakly_canonical(contentRoot, error);
	if (error || !std::filesystem::is_directory(myContentRoot, error))
	{
		SetError(std::format("Content root '{}' is unavailable.", contentRoot.string()));
		myContentRoot.clear();
		return;
	}

	for (std::filesystem::recursive_directory_iterator it(myContentRoot, error), end; it != end && !error; it.increment(error))
	{
		if (!it->is_regular_file(error) || !it->path().has_extension())
		{
			continue;
		}

		const std::filesystem::path& path = it->path();

		std::string lowerExtension = Lower(RegisteredExtension(path));
		if (!myFileExtToAssetType.contains(lowerExtension))
		{
			continue;
		}

		const std::string assetName = GetFullAssetName(path);
		std::string alias = Lower(path.filename().generic_string());
		alias = alias.substr(0, alias.size() - lowerExtension.size());

		myAssets.try_emplace(assetName, AssetInfo{ .Path = path });

		myAssetAliases[alias] = assetName;
	}

	if (error)
	{
		SetError(std::format("Could not finish indexing '{}': {}", myContentRoot.string(), error.message()));
	}
}

void AssetRegistry::Clear()
{
	myContentRoot.clear();
	myAssets.clear();
	myAssetAliases.clear();
	myLastError.clear();
	myLastErrorCode = AssetError::None;
}

std::string AssetRegistry::RegisteredExtension(const std::filesystem::path& path) const
{
	const std::string fileName = Lower(path.filename().string());
	std::string longest;
	for (const auto& [extension, loaders] : myFileExtToAssetType)
	{
		if (fileName.ends_with(extension) && extension.size() > longest.size())
		{
			longest = extension;
		}
	}
	return longest;
}

std::string AssetRegistry::NormalizeAssetName(std::string_view value)
{
	if (value.empty())
	{
		return {};
	}

	std::string result(value);
	std::ranges::replace(result, '\\', '/');
	while (result.starts_with("./"))
	{
		result.erase(0, 2);
	}
	while (result.starts_with('/'))
	{
		result.erase(0, 1);
	}

	return Lower(std::filesystem::path(result).lexically_normal().generic_string());
}

std::string AssetRegistry::GetFullAssetName(const std::filesystem::path& path) const
{
	std::error_code error;
	const std::filesystem::path absolute = path.is_absolute() ? path : myContentRoot / path;
	const std::filesystem::path relative = std::filesystem::relative(absolute, myContentRoot, error);
	return NormalizeAssetName(error ? path.generic_string() : relative.generic_string());
}

void AssetRegistry::SetError(std::string&& aMessage, AssetError aError)
{
	myLastError = std::move(aMessage);
	myLastErrorCode = aError;
}
