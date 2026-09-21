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
	std::string Lower(std::string value)
	{
		std::ranges::transform(value, value.begin(), [](unsigned char character)
		{
			return static_cast<char>(std::tolower(character));
		});
		return value;
	}

	void AddAlias(std::unordered_map<std::string, std::vector<std::string>>& aliases,
	              const std::string& alias, const std::string& id)
	{
		auto& matches = aliases[alias];
		if (std::ranges::find(matches, id) == matches.end())
		{
			matches.push_back(id);
		}
	}

	std::string ReadString(const simdjson::dom::object& object, std::string_view key, std::string fallback = {})
	{
		auto value = object.at_key(key);
		if (value.error() || !value.value().is_string())
		{
			return fallback;
		}
		return std::string(value.value().get_string().value());
	}

	std::filesystem::path ResolveRelative(const std::filesystem::path& parent, const std::string& value)
	{
		if (value.empty()) return {};
		const std::filesystem::path path(value);
		return path.is_absolute() ? path.lexically_normal() : (parent / path).lexically_normal();
	}

	bool ReadNumber(const simdjson::dom::object& object, std::string_view key, float& value)
	{
		double parsed = 0.0;
		if (object.at_key(key).get(parsed)) return false;
		value = static_cast<float>(parsed);
		return std::isfinite(value);
	}

	bool ReadRectangle(const simdjson::dom::object& object, CommonUtilities::Rectanglef& rectangle)
	{
		return ReadNumber(object, "top", rectangle.Top) && ReadNumber(object, "left", rectangle.Left) &&
		       ReadNumber(object, "bottom", rectangle.Bottom) && ReadNumber(object, "right", rectangle.Right);
	}
}

AssetRegistry& AssetRegistry::Get()
{
	static AssetRegistry registry;
	return registry;
}

AssetRegistry::AssetRegistry()
{
	RegisterAssetType<MeshAsset>({".fbx"});
	RegisterAssetType<MaterialAsset>({".mat"});
	RegisterAssetType<TextureAsset>({".dds", ".png", ".jpg", ".jpeg"});
	RegisterAssetType<FontAsset>({".font.json"});
}

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

	for (std::filesystem::recursive_directory_iterator iterator(myContentRoot, error), end; iterator != end && !error; iterator.increment(error))
	{
		if (iterator->is_regular_file(error) && !error)
		{
			IndexFile(iterator->path());
		}
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
	myMeshLoader = {};
	myLastError.clear();
	myLastErrorCode = AssetError::None;
}

std::string AssetRegistry::NormalizeId(std::string_view value)
{
	if (value.empty()) return {};
	std::string result(value);
	std::ranges::replace(result, '\\', '/');
	while (result.starts_with("./")) result.erase(0, 2);
	while (result.starts_with('/')) result.erase(0, 1);
	return Lower(std::filesystem::path(result).lexically_normal().generic_string());
}

AssetId AssetRegistry::MakeAssetId(const std::filesystem::path& path) const
{
	std::error_code error;
	const std::filesystem::path absolute = path.is_absolute() ? path : myContentRoot / path;
	const std::filesystem::path relative = std::filesystem::relative(absolute, myContentRoot, error);
	return AssetId{NormalizeId(error ? path.generic_string() : relative.generic_string())};
}

void AssetRegistry::IndexFile(const std::filesystem::path& path)
{
	const std::string extension = RegisteredExtension(path);
	if (extension.empty()) return;
	const std::string id = MakeAssetId(path).Value;
	const std::string fileName = Lower(path.filename().string());
	const std::string alias = fileName.substr(0, fileName.size() - extension.size());
	myAssets.try_emplace(id, AssetInfo{path});
	AddAlias(myAssetAliases, alias, id);
}

std::string AssetRegistry::RegisteredExtension(const std::filesystem::path& path) const
{
	const std::string fileName = Lower(path.filename().string());
	std::string longest;
	for (const auto& [extension, loaders] : myFileExtToAssetType)
		if (fileName.ends_with(extension) && extension.size() > longest.size()) longest = extension;
	return longest;
}

AssetRegistry::AssetInfo* AssetRegistry::FindAssetInfo(std::string_view name, const std::vector<std::string>& extensions,
                                                       const std::function<bool(const Asset&)>& matchesType)
{
	std::string key = NormalizeId(name);
	if (auto found = myAssets.find(key); found != myAssets.end()) return &found->second;
	const bool unrealPath = key.starts_with("game/");
	if (unrealPath)
	{
		key.erase(0, 5);
		const size_t slash = key.find_last_of('/');
		const size_t objectSeparator = key.find('.', slash == std::string::npos ? 0 : slash);
		if (objectSeparator != std::string::npos) key.resize(objectSeparator);
	}
	std::vector<std::string> matches;
	for (const auto& [id, info] : myAssets)
	{
		const std::string extension = RegisteredExtension(id);
		if ((!extension.empty() && std::ranges::find(extensions, extension) != extensions.end() ||
		     info.Pinned && matchesType(*info.Pinned)) &&
		    id.substr(0, id.size() - extension.size()) == key) matches.push_back(id);
	}
	if (matches.size() == 1) return &myAssets.at(matches.front());
	if (matches.size() > 1)
	{
		myLastErrorCode = AssetError::Ambiguous;
		myLastError = std::format("Asset path '{}' has multiple file types; include an extension.", name);
		return nullptr;
	}
	if (!unrealPath && key.find('/') != std::string::npos)
	{
		myLastErrorCode = AssetError::NotFound;
		myLastError = std::format("Asset path '{}' was not indexed.", name);
		return nullptr;
	}
	const std::string alias = Lower(std::filesystem::path(key).stem().string());
	const auto foundAlias = myAssetAliases.find(alias);
	if (foundAlias == myAssetAliases.end())
	{
		myLastErrorCode = AssetError::NotFound;
		myLastError = std::format("Asset '{}' was not indexed.", name);
		return nullptr;
	}
	matches.clear();
	for (const std::string& id : foundAlias->second)
	{
		const auto& info = myAssets.at(id);
		if (info.Pinned ? matchesType(*info.Pinned) :
		    std::ranges::find(extensions, RegisteredExtension(id)) != extensions.end()) matches.push_back(id);
	}
	if (matches.empty())
	{
		myLastErrorCode = AssetError::NotFound;
		myLastError = std::format("Asset '{}' of the requested type was not indexed.", name);
		return nullptr;
	}
	if (matches.size() != 1)
	{
		myLastErrorCode = AssetError::Ambiguous;
		myLastError = std::format("Asset name '{}' is ambiguous; use a Content-relative path.", name);
		return nullptr;
	}
	return &myAssets.at(matches.front());
}

void AssetRegistry::PinAsset(const AssetId& id, std::shared_ptr<Asset> asset)
{
	if (!asset) return;
	const std::string key = NormalizeId(id.Value);
	if (key.empty()) return;
	auto& info = myAssets[key];
	info.Pinned = std::move(asset);
	info.Cached = info.Pinned;
	const std::string extension = RegisteredExtension(key);
	info.Pinned->myName = extension.empty() ? Lower(std::filesystem::path(key).stem().string()) :
	                       Lower(std::filesystem::path(key).filename().string().substr(0,
	                           std::filesystem::path(key).filename().string().size() - extension.size()));
	AddAlias(myAssetAliases, info.Pinned->myName, key);
}

void AssetRegistry::RegisterMesh(const AssetId& id, std::shared_ptr<Mesh> mesh)
{
	if (mesh) PinAsset(id, std::make_shared<MeshAsset>(std::move(mesh)));
}

void AssetRegistry::RegisterMaterial(const AssetId& id, std::shared_ptr<MaterialInterface> material)
{
	if (material) PinAsset(id, std::make_shared<MaterialAsset>(std::move(material)));
}

void AssetRegistry::RegisterTexture(const AssetId& id, std::shared_ptr<Texture> texture)
{
	if (texture) PinAsset(id, std::make_shared<TextureAsset>(std::move(texture)));
}

void AssetRegistry::RegisterFont(const AssetId& id, std::shared_ptr<Font> font)
{
	if (font) PinAsset(id, std::make_shared<FontAsset>(std::move(font)));
}

std::shared_ptr<Mesh> AssetRegistry::LoadMesh(const std::filesystem::path& path)
{
	if (!myMeshLoader)
	{
		SetError(std::format("No mesh loader is registered for '{}'.", path.string()));
		return {};
	}
	auto mesh = myMeshLoader(path);
	if (!mesh) SetError(std::format("Mesh could not be loaded from '{}'.", path.string()));
	return mesh;
}

MeshHandle AssetRegistry::ResolveMesh(const AssetId& id)
{
	MeshHandle result;
	result.myAsset = GetAsset<MeshAsset>(id.Value);
	if (const auto asset = std::static_pointer_cast<MeshAsset>(result.myAsset)) result.myResource = asset->GetMesh();
	return result;
}

MaterialHandle AssetRegistry::ResolveMaterial(const AssetId& id)
{
	MaterialHandle result;
	result.myAsset = GetAsset<MaterialAsset>(id.Value);
	if (const auto asset = std::static_pointer_cast<MaterialAsset>(result.myAsset)) result.myResource = asset->GetMaterial();
	return result;
}

TextureHandle AssetRegistry::ResolveTexture(const AssetId& id)
{
	TextureHandle result;
	result.myAsset = GetAsset<TextureAsset>(id.Value);
	if (const auto asset = std::static_pointer_cast<TextureAsset>(result.myAsset)) result.myResource = asset->GetTextureShared();
	return result;
}

FontHandle AssetRegistry::ResolveFont(const AssetId& id)
{
	FontHandle result;
	result.myAsset = GetAsset<FontAsset>(id.Value);
	if (const auto asset = std::static_pointer_cast<FontAsset>(result.myAsset)) result.myResource = asset->GetFont();
	return result;
}

std::shared_ptr<Font> AssetRegistry::LoadFont(const std::filesystem::path& path, std::shared_ptr<TextureAsset>& atlasAsset)
{
	simdjson::dom::parser parser;
	simdjson::padded_string json;
	if (simdjson::padded_string::load(path.string()).get(json))
	{
		SetError(std::format("Font metadata '{}' could not be read.", path.string()));
		return nullptr;
	}
	simdjson::dom::object root;
	if (parser.parse(json).get(root))
	{
		SetError(std::format("Font metadata '{}' contains malformed JSON.", path.string()));
		return nullptr;
	}
	auto atlasResult = root.at_key("atlas");
	auto metricsResult = root.at_key("metrics");
	auto glyphsResult = root.at_key("glyphs");
	const std::string atlasFile = ReadString(root, "atlasFile");
	if (atlasResult.error() || !atlasResult.value().is_object() || metricsResult.error() || !metricsResult.value().is_object() ||
	    glyphsResult.error() || !glyphsResult.value().is_array() || atlasFile.empty())
	{
		SetError(std::format("Font metadata '{}' is missing atlas, metrics, glyphs, or atlasFile.", path.string()));
		return nullptr;
	}
	const auto atlas = atlasResult.value().get_object().value();
	const auto metrics = metricsResult.value().get_object().value();
	auto font = std::make_shared<Font>();
	float width = 0.0f, height = 0.0f;
	if (!ReadNumber(metrics, "lineHeight", font->myLineHeight) || !ReadNumber(metrics, "ascender", font->myAscender) ||
	    !ReadNumber(metrics, "descender", font->myDescender) || !ReadNumber(atlas, "distanceRange", font->myDistanceRange) ||
	    !ReadNumber(atlas, "width", width) || !ReadNumber(atlas, "height", height) || font->myLineHeight <= 0.0f ||
	    width <= 0.0f || height <= 0.0f)
	{
		SetError(std::format("Font metadata '{}' has missing or invalid line/atlas metrics.", path.string()));
		return nullptr;
	}
	font->myAtlasWidth = static_cast<unsigned>(width);
	font->myAtlasHeight = static_cast<unsigned>(height);
	atlasAsset = GetAsset<TextureAsset>(MakeAssetId(ResolveRelative(path.parent_path(), atlasFile)).Value);
	if (!atlasAsset)
	{
		const std::string atlasError = myLastError;
		SetError(std::format("Font metadata '{}' requires atlas '{}': {}", path.string(), atlasFile, atlasError));
		return nullptr;
	}
	font->myAtlas = atlasAsset->GetTextureShared();
	for (const simdjson::dom::element element : glyphsResult.value().get_array().value())
	{
		if (!element.is_object()) continue;
		const auto glyphObject = element.get_object().value();
		uint64_t unicode = 0;
		Font::Glyph glyph;
		if (glyphObject.at_key("unicode").get(unicode) || !ReadNumber(glyphObject, "advance", glyph.Advance)) continue;
		glyph.Unicode = static_cast<uint32_t>(unicode);
		auto planeResult = glyphObject.at_key("planeBounds");
		auto boundsResult = glyphObject.at_key("atlasBounds");
		if (!planeResult.error() && planeResult.value().is_object() && !boundsResult.error() && boundsResult.value().is_object())
		{
			glyph.HasGeometry = ReadRectangle(planeResult.value().get_object().value(), glyph.PlaneBounds) &&
			                    ReadRectangle(boundsResult.value().get_object().value(), glyph.AtlasBounds);
			if (!glyph.HasGeometry)
			{
				SetError(std::format("Font metadata '{}' has malformed bounds for codepoint {}.", path.string(), unicode));
				return nullptr;
			}
		}
		font->myGlyphs[glyph.Unicode] = glyph;
	}
	if (font->myGlyphs.empty() || !font->FindGlyph('?'))
	{
		SetError(std::format("Font metadata '{}' contains no usable glyphs or '?' fallback.", path.string()));
		return nullptr;
	}
	return font;
}

void AssetRegistry::SetError(std::string message)
{
	myLastError = std::move(message);
	myLastErrorCode = AssetError::LoadFailed;
}
