#include "MaterialAsset.h"

#include "AssetRegistry.h"
#include "TextureAsset.h"

#include <GraphicsEngine/GraphicsEngine.h>
#include <Logger/Logger.h>

DECLARE_LOG_CATEGORY_WITH_NAME(MaterialAssetLog, ASSET, Verbose);

DEFINE_LOG_CATEGORY(MaterialAssetLog);

MaterialAsset::MaterialAsset() = default;
MaterialAsset::MaterialAsset(std::shared_ptr<MaterialInterface> material) : myMaterial(std::move(material)) {}
MaterialAsset::~MaterialAsset() = default;

namespace
{
    MaterialDomain GetMaterialDomain(std::string_view aDomainName)
    {
        return aDomainName == "Surface" ? MaterialDomain::Surface : MaterialDomain::None;
    }

    ShadingModel GetShadingModel(std::string_view aModelName)
    {
        if (aModelName == "Unlit")
        {
            return ShadingModel::Unlit;
        }
        else if (aModelName == "Lit")
        {
            return ShadingModel::Lit;
        }

        return ShadingModel::None;
    }

    BlendMode GetBlendMode(std::string_view aBlendModeName)
    {
        return aBlendModeName == "Alpha" ? BlendMode::Alpha : BlendMode::Opaque;
    }

    std::filesystem::path ResolveRelativePath(const std::filesystem::path& aBasePath, std::string_view aValue)
    {
        if (aValue.empty())
        {
            return {};
        }

        std::filesystem::path path = aValue;
        if (path.is_absolute())
        {
            return path;
        }

        return aBasePath / path;
    }
}

const std::shared_ptr<MaterialInterface>& MaterialAsset::GetMaterial() const
{
    return myMaterial;
}

using JsonElement = simdjson::simdjson_result<simdjson::dom::element>;

#define JSON_EXISTS(jsonElement) (jsonElement.error() != simdjson::NO_SUCH_FIELD)

bool MaterialAsset::Load(const std::filesystem::path& aPath, AssetRegistry& aRegistry)
{
    simdjson::padded_string json = simdjson::padded_string::load(aPath.c_str());
    simdjson::dom::parser parser;
    simdjson::dom::object root;

    simdjson::error_code error = parser.parse(json).get(root);
    if (error != simdjson::error_code::SUCCESS)
    {
        LOG(MaterialAssetLog, Warning, "Could not parse material at '{}'.", aPath.string());
        return false;
    }

    JsonElement masterMat = root.at_key("masterMaterial");
    if (JSON_EXISTS(masterMat) && masterMat.get_string_length().value() > 0)
    {
        return CreateMaterialInstance(aPath, root, aRegistry);
    }

    return CreateMaterial(aPath, root);
}

bool MaterialAsset::CreateMaterial(const std::filesystem::path& aPath, const simdjson::dom::object& aRoot)
{
    MaterialDescription description;
    
    {
        JsonElement nameElement = aRoot.at_key("name");
        description.Name = JSON_EXISTS(nameElement) ? std::string(nameElement) : aPath.filename().string();
    }

    {
        JsonElement general = aRoot.at_key("general");
        if (JSON_EXISTS(general))
        {
            JsonElement domain = general.at_key("domain");
            JsonElement shadingModel = general.at_key("shadingModel");
            JsonElement blendMode = general.at_key("blendMode");

            description.Domain = !JSON_EXISTS(domain) ? MaterialDomain::Surface : GetMaterialDomain(domain);
            description.ShadingModel = !JSON_EXISTS(shadingModel) ? ShadingModel::Lit : GetShadingModel(shadingModel);
            description.BlendMode = !JSON_EXISTS(blendMode) ? BlendMode::Opaque : GetBlendMode(blendMode);
        }
        else
        {
            description.Domain = MaterialDomain::Surface;
            description.ShadingModel = ShadingModel::Lit;
            description.BlendMode = BlendMode::Opaque;
        }
    }

    const std::filesystem::path basePath = aPath.parent_path();
    {
        JsonElement textures = aRoot.at_key("textures");
        if (JSON_EXISTS(textures))
        {
            JsonElement albedoTexture = textures.at_key("albedo");
            JsonElement normalTexture = textures.at_key("normal");
            JsonElement materialTexture = textures.at_key("material");

            description.AlbedoTexture = !JSON_EXISTS(albedoTexture) ? "" : ResolveRelativePath(basePath, albedoTexture);
            description.NormalTexture = !JSON_EXISTS(normalTexture) ? "" : ResolveRelativePath(basePath, normalTexture);
            description.MaterialTexture = !JSON_EXISTS(materialTexture) ? "" : ResolveRelativePath(basePath, materialTexture);
        }
    }

    {
        JsonElement shaderCode = aRoot.at_key("shaderCode");
        description.MaterialShaderCode = !JSON_EXISTS(shaderCode) ? "" : ResolveRelativePath(basePath, shaderCode);
    }

    std::unique_ptr<Material> material = std::make_unique<Material>();
    if (!GraphicsEngine::Get().CreateMaterial(description, *material))
    {
        LOG(MaterialAssetLog, Warning, "Could not create material '{}'.", description.Name);
        return false;
    }

    myMaterial = std::move(material);

    return true;
}

bool MaterialAsset::CreateMaterialInstance(const std::filesystem::path& aPath, const simdjson::dom::object& aRoot, AssetRegistry& aRegistry)
{
    JsonElement nameElement = aRoot.at_key("name");
    std::string name = JSON_EXISTS(nameElement) ? std::string(nameElement) : aPath.filename().string();

    std::string masterMaterialName = std::string(aRoot["masterMaterial"]);
    std::shared_ptr<MaterialAsset> masterMaterialAsset = aRegistry.GetAsset<MaterialAsset>(masterMaterialName);
    if (!masterMaterialAsset)
    {
        LOG(MaterialAssetLog, Warning, "Material '{}' has unavailable master '{}': {}", name, masterMaterialName, aRegistry.GetLastError());
        return false;
    }
	myParentAsset = masterMaterialAsset;

    std::shared_ptr<MaterialInstance> material = MaterialInstance::Create(name, masterMaterialAsset->GetMaterial());
    if (material == nullptr)
    {
        LOG(MaterialAssetLog, Warning, "Could not create material instance {}", aPath.string());
        return false;
    }

    const std::filesystem::path basePath = aPath.parent_path();
    {
        JsonElement textures = aRoot.at_key("textures");
        if (JSON_EXISTS(textures))
        {
            JsonElement albedoTexture = textures.at_key("albedo");
            JsonElement normalTexture = textures.at_key("normal");
            JsonElement materialTexture = textures.at_key("material");

            std::filesystem::path albedoPath = !JSON_EXISTS(albedoTexture) ? "" : ResolveRelativePath(basePath, albedoTexture);
            std::filesystem::path normalPath = !JSON_EXISTS(normalTexture) ? "" : ResolveRelativePath(basePath, normalTexture);
            std::filesystem::path materialPath = !JSON_EXISTS(materialTexture) ? "" : ResolveRelativePath(basePath, materialTexture);

            const auto applyTexture = [&](const std::filesystem::path& path, unsigned slot)
            {
                if (path.empty())
				{
					return true;
				}

				const std::shared_ptr<TextureAsset> texture = aRegistry.GetAsset<TextureAsset>(path.generic_string());
                if (!texture || !material->SetTexture(slot, texture->GetTextureShared()))
				{
					return false;
				}
                myTextureAssets.push_back(texture);
                return true;
            };
            if (!applyTexture(albedoPath, Material::ALBEDO_TEXTURE_SLOT) ||
                !applyTexture(normalPath, Material::NORMAL_TEXTURE_SLOT) ||
                !applyTexture(materialPath, Material::MATERIAL_TEXTURE_SLOT))
            {
                LOG(MaterialAssetLog, Warning, "Material '{}' has an unavailable texture: {}", name, aRegistry.GetLastError());
                return false;
            }
        }
    }

    if (!SetParameters(aRoot, material))
    {
        LOG(MaterialAssetLog, Warning, "Material '{}' has an invalid or incompatible parameter.", name);
        return false;
    }

    myMaterial = std::move(material);
    return true;
}

bool MaterialAsset::SetParameters(const simdjson::dom::object& aRoot, std::shared_ptr<MaterialInstance>& aMaterial)
{
    auto parameterResult = aRoot.at_key("parameters");
    if (parameterResult.error() == simdjson::NO_SUCH_FIELD)
	{
		return true;
	}
    if (parameterResult.error() || !parameterResult.value().is_object())
	{
		return false;
	}
    simdjson::dom::object parameters = parameterResult.value().get_object().value();
    bool applied = true;
    for (auto it = parameters.begin(); it != parameters.end(); ++it)
    {
        bool handled = false;
        simdjson::dom::key_value_pair itValue = *it;
        std::string parameterName = std::string(itValue.key);
        simdjson::dom::element parameterValue = itValue.value;

        if (parameterValue.is_array() && parameterValue.get_array().size() == 1)
        {
            parameterValue = parameterValue.get_array().at(0);
        }

        if (parameterValue.is_uint64())
        {
            handled = true;
			applied &= aMaterial->SetValue(parameterName, static_cast<uint32_t>(parameterValue.get_uint64().value()));
        }
        else if (parameterValue.is_int64())
        {
            handled = true;
			applied &= aMaterial->SetValue(parameterName, static_cast<int>(parameterValue.get_int64().value()));
        }
        else if (parameterValue.is_double())
        {
            handled = true;
			applied &= aMaterial->SetValue(parameterName, static_cast<float>(parameterValue.get_double().value()));
        }
        else if (parameterValue.is_bool())
        {
            handled = true;
			applied &= aMaterial->SetValue(parameterName, parameterValue.get_bool().value());
        }
        else if (parameterValue.is_array())
        {
            simdjson::dom::array parameterArray = parameterValue.get_array().value();
            simdjson::dom::element arrayElement = parameterArray.at(0);

            switch (parameterArray.size())
            {
                case 2:
                {
                    if (arrayElement.is_uint64())
                    {
                        CU::Vector2u value(static_cast<uint32_t>(parameterArray.at(0).get_uint64().value()),
                            static_cast<uint32_t>(parameterArray.at(1).get_uint64().value()));
                        handled = true;
						applied &= aMaterial->SetValue(parameterName, value);
                    }
                    else if (arrayElement.is_int64())
                    {
                        CU::Vector2i value(static_cast<int>(parameterArray.at(0).get_int64().value()),
                            static_cast<int>(parameterArray.at(1).get_int64().value()));
                        handled = true;
						applied &= aMaterial->SetValue(parameterName, value);
                    }
                    else if (arrayElement.is_double())
                    {
                        CU::Vector2f value(static_cast<float>(parameterArray.at(0).get_double().value()),
                            static_cast<float>(parameterArray.at(1).get_double().value()));
                        handled = true;
						applied &= aMaterial->SetValue(parameterName, value);
                    }
                    break;
                }

                case 3:
                {
                    if (arrayElement.is_uint64())
                    {
                        CU::Vector3<unsigned> value(static_cast<uint32_t>(parameterArray.at(0).get_uint64().value()),
                            static_cast<uint32_t>(parameterArray.at(1).get_uint64().value()),
                            static_cast<uint32_t>(parameterArray.at(2).get_uint64().value()));
                        handled = true;
						applied &= aMaterial->SetValue(parameterName, value);
                    }
                    else if (arrayElement.is_int64())
                    {
                        CU::Vector3<int> value(static_cast<int>(parameterArray.at(0).get_int64().value()),
                            static_cast<int>(parameterArray.at(1).get_int64().value()),
                            static_cast<int>(parameterArray.at(2).get_int64().value()));
                        handled = true;
						applied &= aMaterial->SetValue(parameterName, value);
                    }
                    else if (arrayElement.is_double())
                    {
                        CU::Vector3f value(static_cast<float>(parameterArray.at(0).get_double().value()),
                            static_cast<float>(parameterArray.at(1).get_double().value()),
                            static_cast<float>(parameterArray.at(2).get_double().value()));
                        handled = true;
						applied &= aMaterial->SetValue(parameterName, value);
                    }
                    break;
                }

                case 4:
                {
                    if (arrayElement.is_uint64())
                    {
                        CU::Vector4u value(static_cast<uint32_t>(parameterArray.at(0).get_uint64().value()),
                            static_cast<uint32_t>(parameterArray.at(1).get_uint64().value()),
                            static_cast<uint32_t>(parameterArray.at(2).get_uint64().value()),
                            static_cast<uint32_t>(parameterArray.at(3).get_uint64().value()));
                        handled = true;
						applied &= aMaterial->SetValue(parameterName, value);
                    }
                    else if (arrayElement.is_int64())
                    {
                        CU::Vector4i value(static_cast<int>(parameterArray.at(0).get_int64().value()),
                            static_cast<int>(parameterArray.at(1).get_int64().value()),
                            static_cast<int>(parameterArray.at(2).get_int64().value()),
                            static_cast<int>(parameterArray.at(3).get_int64().value()));
                        handled = true;
						applied &= aMaterial->SetValue(parameterName, value);
                    }
                    else if (arrayElement.is_double())
                    {
                        CU::Vector4f value(static_cast<float>(parameterArray.at(0).get_double().value()),
                            static_cast<float>(parameterArray.at(1).get_double().value()),
                            static_cast<float>(parameterArray.at(2).get_double().value()),
                            static_cast<float>(parameterArray.at(3).get_double().value()));
                        handled = true;
						applied &= aMaterial->SetValue(parameterName, value);
                    }
                    break;
                }
            }
        }

        if (!handled)
		{
			return false;
		}
    }
    return applied;
}
