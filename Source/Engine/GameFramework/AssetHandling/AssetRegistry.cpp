#include "GameFramework/AssetHandling/AssetRegistry.h"

#include "GameFramework/SimdJson/simdjson.h"
#include "GraphicsEngine/GraphicsEngine.h"
#include "GraphicsEngine/Materials/Material.h"
#include "GraphicsEngine/Objects/Texture.h"

#include <array>
#include <algorithm>
#include <cctype>
#include <format>
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

	MaterialDomain ParseDomain(std::string value)
	{
		return Lower(std::move(value)) == "surface" ? MaterialDomain::Surface : MaterialDomain::None;
	}

	ShadingModel ParseShadingModel(std::string value)
	{
		value = Lower(std::move(value));
		if (value == "lit") return ShadingModel::Lit;
		if (value == "unlit") return ShadingModel::Unlit;
		return ShadingModel::None;
	}

	BlendMode ParseBlendMode(std::string value)
	{
		return Lower(std::move(value)) == "alpha" ? BlendMode::Alpha : BlendMode::Opaque;
	}

	std::filesystem::path ResolveRelative(const std::filesystem::path& parent, const std::string& value)
	{
		if (value.empty()) return {};
		const std::filesystem::path path(value);
		return path.is_absolute() ? path.lexically_normal() : (parent / path).lexically_normal();
	}
}

AssetRegistry& AssetRegistry::Get()
{
	static AssetRegistry registry;
	return registry;
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
	myMeshes.clear();
	myMaterials.clear();
	myTextures.clear();
	myMeshAliases.clear();
	myMaterialAliases.clear();
	myTextureAliases.clear();
	myMeshLoader = {};
	myLastError.clear();
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
	const std::string extension = Lower(path.extension().string());
	const std::string id = MakeAssetId(path).Value;
	const std::string alias = Lower(path.stem().string());
	if (extension == ".fbx")
	{
		myMeshes.try_emplace(id, Entry<Mesh>{path});
		AddAlias(myMeshAliases, alias, id);
	}
	else if (extension == ".mat")
	{
		myMaterials.try_emplace(id, Entry<MaterialInterface>{path});
		AddAlias(myMaterialAliases, alias, id);
	}
	else if (extension == ".dds" || extension == ".png" || extension == ".jpg" || extension == ".jpeg")
	{
		myTextures.try_emplace(id, Entry<Texture>{path});
		AddAlias(myTextureAliases, alias, id);
	}
}

template<class T>
AssetRegistry::Entry<T>* AssetRegistry::FindEntry(
	const AssetId& id, EntryMap<T>& entries, const AliasMap& aliases, std::string_view expectedExtension)
{
	std::string key = NormalizeId(id.Value);
	if (auto found = entries.find(key); found != entries.end())
	{
		return &found->second;
	}

	if (key.starts_with("game/"))
	{
		key.erase(0, 5);
		const size_t slash = key.find_last_of('/');
		const size_t objectSeparator = key.find('.', slash == std::string::npos ? 0 : slash);
		if (objectSeparator != std::string::npos) key.resize(objectSeparator);
		key += expectedExtension;
		if (auto found = entries.find(key); found != entries.end())
		{
			return &found->second;
		}
	}

	const std::string alias = Lower(std::filesystem::path(key).stem().string());
	const auto aliasIt = aliases.find(alias);
	if (aliasIt == aliases.end())
	{
		SetError(std::format("Asset '{}' was not indexed.", id.Value));
		return nullptr;
	}
	if (aliasIt->second.size() != 1)
	{
		SetError(std::format("Asset name '{}' is ambiguous; use a Content-relative path.", id.Value));
		return nullptr;
	}
	return &entries.at(aliasIt->second.front());
}

void AssetRegistry::RegisterMesh(const AssetId& id, std::shared_ptr<Mesh> mesh)
{
	if (!mesh) return;
	const std::string key = NormalizeId(id.Value);
	if (key.empty()) return;
	auto& entry = myMeshes[key];
	entry.Pinned = std::move(mesh);
	entry.Cached = entry.Pinned;
	AddAlias(myMeshAliases, Lower(std::filesystem::path(key).stem().string()), key);
}

void AssetRegistry::RegisterMaterial(const AssetId& id, std::shared_ptr<MaterialInterface> material)
{
	if (!material) return;
	const std::string key = NormalizeId(id.Value);
	if (key.empty()) return;
	auto& entry = myMaterials[key];
	entry.Pinned = std::move(material);
	entry.Cached = entry.Pinned;
	AddAlias(myMaterialAliases, Lower(std::filesystem::path(key).stem().string()), key);
}

void AssetRegistry::RegisterTexture(const AssetId& id, std::shared_ptr<Texture> texture)
{
	if (!texture) return;
	const std::string key = NormalizeId(id.Value);
	if (key.empty()) return;
	auto& entry = myTextures[key];
	entry.Pinned = std::move(texture);
	entry.Cached = entry.Pinned;
	AddAlias(myTextureAliases, Lower(std::filesystem::path(key).stem().string()), key);
}

MeshAsset AssetRegistry::ResolveMesh(const AssetId& id)
{
	myLastError.clear();
	MeshAsset result;
	Entry<Mesh>* entry = FindEntry(id, myMeshes, myMeshAliases, ".fbx");
	if (!entry) return result;
	result.myResource = entry->Pinned ? entry->Pinned : entry->Cached.lock();
	if (!result.myResource && entry->Path.empty())
	{
		SetError(std::format("Mesh '{}' has no source file.", id.Value));
		return {};
	}
	if (!result.myResource && !myMeshLoader)
	{
		SetError(std::format("No mesh loader is registered for '{}'.", id.Value));
		return {};
	}
	if (!result.myResource)
	{
		result.myResource = myMeshLoader(entry->Path);
		entry->Cached = result.myResource;
	}
	if (!result.myResource) SetError(std::format("Mesh '{}' could not be loaded from '{}'.", id.Value, entry->Path.string()));
	return result;
}

MaterialAsset AssetRegistry::ResolveMaterial(const AssetId& id)
{
	myLastError.clear();
	MaterialAsset result;
	Entry<MaterialInterface>* entry = FindEntry(id, myMaterials, myMaterialAliases, ".mat");
	if (!entry) return result;
	result.myResource = entry->Pinned ? entry->Pinned : entry->Cached.lock();
	if (!result.myResource && !entry->Path.empty())
	{
		result.myResource = LoadMaterial(entry->Path);
		entry->Cached = result.myResource;
	}
	if (!result.myResource && myLastError.empty()) SetError(std::format("Material '{}' could not be loaded.", id.Value));
	return result;
}

TextureAsset AssetRegistry::ResolveTexture(const AssetId& id)
{
	myLastError.clear();
	TextureAsset result;
	Entry<Texture>* entry = FindEntry(id, myTextures, myTextureAliases, ".dds");
	if (!entry) return result;
	result.myResource = entry->Pinned ? entry->Pinned : entry->Cached.lock();
	if (!result.myResource && !entry->Path.empty())
	{
		result.myResource = LoadTexture(entry->Path);
		entry->Cached = result.myResource;
	}
	if (!result.myResource && myLastError.empty()) SetError(std::format("Texture '{}' could not be loaded.", id.Value));
	return result;
}

std::shared_ptr<Texture> AssetRegistry::LoadTexture(const std::filesystem::path& path)
{
	std::shared_ptr<Texture> texture = std::make_shared<Texture>();
	if (!GraphicsEngine::Get().LoadTexture(path, *texture))
	{
		SetError(std::format("Texture '{}' failed renderer initialization.", path.string()));
		return nullptr;
	}
	return texture;
}

std::shared_ptr<MaterialInterface> AssetRegistry::LoadMaterial(const std::filesystem::path& path)
{
	simdjson::dom::parser parser;
	simdjson::padded_string json;
	if (simdjson::padded_string::load(path.string()).get(json))
	{
		SetError(std::format("Material '{}' could not be read.", path.string()));
		return nullptr;
	}
	simdjson::dom::object root;
	if (parser.parse(json).get(root))
	{
		SetError(std::format("Material '{}' contains malformed JSON.", path.string()));
		return nullptr;
	}

	const std::string name = ReadString(root, "name", path.stem().string());
	const std::string master = ReadString(root, "masterMaterial");
	if (!master.empty())
	{
		const MaterialAsset parent = ResolveMaterial(AssetId{master});
		if (!parent)
		{
			SetError(std::format("Material '{}' requires unavailable parent '{}'.", name, master));
			return nullptr;
		}
		std::shared_ptr<MaterialInstance> instance = MaterialInstance::Create(name, parent.myResource);
		if (!instance) return nullptr;

		auto texturesResult = root.at_key("textures");
		if (!texturesResult.error() && texturesResult.value().is_object())
		{
			const auto textures = texturesResult.value().get_object().value();
			const std::array<std::pair<std::string_view, unsigned>, 3> slots{{
				{"albedo", MaterialInterface::ALBEDO_TEXTURE_SLOT},
				{"normal", MaterialInterface::NORMAL_TEXTURE_SLOT},
				{"material", MaterialInterface::MATERIAL_TEXTURE_SLOT}}};
			for (const auto& [key, slot] : slots)
			{
				const std::string textureName = ReadString(textures, key);
				if (textureName.empty()) continue;
				const TextureAsset texture = ResolveTexture(MakeAssetId(ResolveRelative(path.parent_path(), textureName)));
				if (!texture || !instance->SetTexture(slot, texture.myResource))
				{
					SetError(std::format("Material '{}' could not resolve texture '{}'.", name, textureName));
					return nullptr;
				}
			}
		}

		auto parametersResult = root.at_key("parameters");
		if (!parametersResult.error() && parametersResult.value().is_object())
		{
			for (const auto [key, value] : parametersResult.value().get_object().value())
			{
				const std::string parameterName(key);
				bool applied = false;
				if (value.is_double()) applied = instance->SetValue(parameterName, static_cast<float>(value.get_double().value()));
				else if (value.is_int64()) applied = instance->SetValue(parameterName, static_cast<int>(value.get_int64().value()));
				else if (value.is_uint64()) applied = instance->SetValue(parameterName, static_cast<unsigned>(value.get_uint64().value()));
				else if (value.is_bool()) applied = instance->SetValue(parameterName, value.get_bool().value());
				else if (value.is_array())
				{
					auto array = value.get_array().value();
					if (array.size() == 2) applied = instance->SetValue(parameterName, CU::Vector2f{
						static_cast<float>(array.at(0).get_double().value()), static_cast<float>(array.at(1).get_double().value())});
					else if (array.size() == 3) applied = instance->SetValue(parameterName, CU::Vector3f{
						static_cast<float>(array.at(0).get_double().value()), static_cast<float>(array.at(1).get_double().value()),
						static_cast<float>(array.at(2).get_double().value())});
					else if (array.size() == 4) applied = instance->SetValue(parameterName, CU::Vector4f{
						static_cast<float>(array.at(0).get_double().value()), static_cast<float>(array.at(1).get_double().value()),
						static_cast<float>(array.at(2).get_double().value()), static_cast<float>(array.at(3).get_double().value())});
				}
				if (!applied)
				{
					SetError(std::format("Material '{}' has an invalid or incompatible parameter '{}'.", name, parameterName));
					return nullptr;
				}
			}
		}
		return instance;
	}

	MaterialDescription description;
	description.Name = name;
	description.Domain = ParseDomain(ReadString(root, "domain", "Surface"));
	description.ShadingModel = ParseShadingModel(ReadString(root, "shadingModel", "Unlit"));
	description.BlendMode = ParseBlendMode(ReadString(root, "blendMode", "Opaque"));
	description.MaterialShaderCode = ResolveRelative(path.parent_path(), ReadString(root, "materialShaderCode"));
	description.AlbedoTexture = ResolveRelative(path.parent_path(), ReadString(root, "albedoTexture"));
	description.NormalTexture = ResolveRelative(path.parent_path(), ReadString(root, "normalTexture"));
	description.MaterialTexture = ResolveRelative(path.parent_path(), ReadString(root, "materialTexture"));

	std::shared_ptr<Material> material = std::make_shared<Material>();
	if (!GraphicsEngine::Get().CreateMaterial(description, *material))
	{
		SetError(std::format("Material '{}' failed renderer initialization.", path.string()));
		return nullptr;
	}
	return material;
}

void AssetRegistry::SetError(std::string message)
{
	myLastError = std::move(message);
}

template AssetRegistry::Entry<Mesh>* AssetRegistry::FindEntry(
	const AssetId&, EntryMap<Mesh>&, const AliasMap&, std::string_view);
template AssetRegistry::Entry<MaterialInterface>* AssetRegistry::FindEntry(
	const AssetId&, EntryMap<MaterialInterface>&, const AliasMap&, std::string_view);
template AssetRegistry::Entry<Texture>* AssetRegistry::FindEntry(
	const AssetId&, EntryMap<Texture>&, const AliasMap&, std::string_view);
