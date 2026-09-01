#include "GraphicsEngine.pch.h"
#include "Material.h"

#include <cctype>
#include <fstream>
#include <format>
#include <limits>
#include <optional>
#include <ranges>

namespace
{
	std::string ToLower(std::string aValue)
	{
		std::ranges::transform(aValue, aValue.begin(), [](unsigned char aChar)
		{
			return static_cast<char>(std::tolower(aChar));
		});
		return aValue;
	}

	std::string Trim(std::string_view aValue)
	{
		size_t first = 0;
		while (first < aValue.size() && std::isspace(static_cast<unsigned char>(aValue[first])))
		{
			++first;
		}

		size_t last = aValue.size();
		while (last > first && std::isspace(static_cast<unsigned char>(aValue[last - 1])))
		{
			--last;
		}

		return std::string(aValue.substr(first, last - first));
	}

	std::optional<std::string> ExtractStringValue(const std::string& aText, std::string_view aKey)
	{
		const std::string keyToken = std::format("\"{}\"", aKey);
		const size_t keyPos = aText.find(keyToken);
		if (keyPos == std::string::npos)
		{
			return std::nullopt;
		}

		const size_t colonPos = aText.find(':', keyPos + keyToken.size());
		if (colonPos == std::string::npos)
		{
			return std::nullopt;
		}

		size_t valueStart = colonPos + 1;
		while (valueStart < aText.size() && std::isspace(static_cast<unsigned char>(aText[valueStart])))
		{
			++valueStart;
		}

		if (valueStart >= aText.size())
		{
			return std::nullopt;
		}

		if (aText[valueStart] == '"')
		{
			const size_t stringStart = valueStart + 1;
			const size_t stringEnd = aText.find('"', stringStart);
			if (stringEnd == std::string::npos)
			{
				return std::nullopt;
			}

			return aText.substr(stringStart, stringEnd - stringStart);
		}

		const size_t valueEnd = aText.find_first_of(",}\r\n", valueStart);
		return Trim(aText.substr(valueStart, valueEnd - valueStart));
	}

	std::filesystem::path ResolveRelativePath(const std::filesystem::path& aBasePath, const std::string& aValue)
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

	MaterialDomain ParseMaterialDomain(const std::string& aValue)
	{
		const std::string lowerValue = ToLower(aValue);
		if (lowerValue == "surface")
		{
			return MaterialDomain::Surface;
		}

		return MaterialDomain::None;
	}

	ShadingModel ParseShadingModel(const std::string& aValue)
	{
		const std::string lowerValue = ToLower(aValue);
		if (lowerValue == "unlit")
		{
			return ShadingModel::Unlit;
		}
		if (lowerValue == "lit")
		{
			return ShadingModel::Lit;
		}

		return ShadingModel::None;
	}

	BlendMode ParseBlendMode(const std::string& aValue)
	{
		const std::string lowerValue = ToLower(aValue);
		if (lowerValue == "opaque")
		{
			return BlendMode::Opaque;
		}
		else if (lowerValue == "alpha")
		{
			return BlendMode::Alpha;
		}


		return BlendMode::Opaque;
	}
}

bool LoadMaterialDescription(const std::filesystem::path& aPath, MaterialDescription& outDescription)
{
	std::ifstream file(aPath, std::ios::binary);
	if (!file)
	{
		return false;
	}

	std::ostringstream stream;
	stream << file.rdbuf();
	const std::string text = stream.str();
	const std::filesystem::path basePath = aPath.parent_path();

	MaterialDescription description;
	description.Name = ExtractStringValue(text, "name").value_or(aPath.stem().string());
	description.Domain = ParseMaterialDomain(ExtractStringValue(text, "domain").value_or("Surface"));
	description.ShadingModel = ParseShadingModel(ExtractStringValue(text, "shadingModel").value_or("Unlit"));
	description.BlendMode = ParseBlendMode(ExtractStringValue(text, "blendMode").value_or("Opaque"));
	description.MaterialShaderCode = ResolveRelativePath(basePath, ExtractStringValue(text, "materialShaderCode").value_or({}));
	description.AlbedoTexture = ResolveRelativePath(basePath, ExtractStringValue(text, "albedoTexture").value_or({}));
	description.NormalTexture = ResolveRelativePath(basePath, ExtractStringValue(text, "normalTexture").value_or({}));
	description.MaterialTexture = ResolveRelativePath(basePath, ExtractStringValue(text, "materialTexture").value_or({}));

	outDescription = std::move(description);
	return true;
}

bool Material::IsMaterialDataDirty() const
{
    return false;
}

void Material::RefreshMaterialData() const
{
    // Nothing here for Material.
}

const MaterialParameterInfo* Material::GetParameterByIndex(unsigned aIndex) const
{
    if (myParameters.size() <= aIndex)
        return nullptr;

	return &myParameters[aIndex];
}

const MaterialParameterInfo* Material::GetParameterByName(const std::string& aName) const
{
	const auto it = myParameterNameToIndex.find(aName);
	if (it == myParameterNameToIndex.end())
		return nullptr;

	return &myParameters[it->second];
}

unsigned Material::GetTextureSlotByName(const std::string& aName) const
{
	std::string lowerName = ToLower(aName);
	auto it = myTextureSlotNameToIndex.find(lowerName);
	if (it == myTextureSlotNameToIndex.end())
	{
		return (std::numeric_limits<unsigned>::max)();
	}

	return it->second;
}

bool Material::SetTexture(const std::string& aName, const std::shared_ptr<Texture>& aTexture)
{
	const unsigned slot = GetTextureSlotByName(aName);

	return SetTexture(slot, aTexture);
}

bool Material::SetTexture(unsigned aSlot, const std::shared_ptr<Texture>& aTexture)
{
	if (aSlot >= MAX_MATERIAL_TEXTURE_COUNT)
		return false;

	myTextures[aSlot] = aTexture;
	return true;
}

std::shared_ptr<Texture> Material::GetTexture(const std::string& aName) const
{
	const unsigned slot = GetTextureSlotByName(aName);
	return GetTexture(slot);
}

std::shared_ptr<Texture> Material::GetTexture(unsigned aSlot) const
{
	if (aSlot >= MAX_MATERIAL_TEXTURE_COUNT)
		return nullptr;

	return myTextures[aSlot];
}

std::shared_ptr<MaterialInstance> MaterialInstance::Create(std::string_view aName, const std::shared_ptr<MaterialInterface>& aMaterialInterface)
{
	std::shared_ptr<MaterialInstance> instance = std::make_shared<MaterialInstance>();
	instance->myParentMaterial = aMaterialInterface;

	memcpy_s(instance->myData, MATERIAL_BUFFER_SIZE, aMaterialInterface->GetParameterDataBlock(), MATERIAL_BUFFER_SIZE);
	instance->myOverridenParameters.resize(aMaterialInterface->GetParameters().size());
	instance->myIsParameterDataDirty = true;

	instance->myName = aName;

	return instance;
}

bool MaterialInstance::IsMaterialDataDirty() const
{
	return myIsParameterDataDirty || myParentMaterial->IsMaterialDataDirty();
}

void MaterialInstance::RefreshMaterialData() const
{
	if (myParentMaterial->IsMaterialDataDirty())
	{
		std::vector<MaterialParameterInfo> overriddenParams;
		overriddenParams.reserve(myOverridenParameters.size());
		for (size_t i = 0; i < myOverridenParameters.size(); i++)
		{
			if (!myOverridenParameters[i])
				continue;

			overriddenParams.emplace_back(*GetParameterByIndex(static_cast<unsigned>(i)));
		}

		alignas(16) uint8_t settingsCache[MATERIAL_BUFFER_SIZE] = {};
		memcpy_s(settingsCache, MATERIAL_BUFFER_SIZE, myData, MATERIAL_BUFFER_SIZE);

		myParentMaterial->RefreshMaterialData();

		memcpy_s(myData, MATERIAL_BUFFER_SIZE, myParentMaterial->GetParameterDataBlock(), MATERIAL_BUFFER_SIZE);

		for (const auto& overriddenParam : overriddenParams)
		{
			if (const MaterialParameterInfo* info = GetParameterByName(overriddenParam.Name))
			{
				uint8_t* srcPtr = settingsCache + overriddenParam.Offset;
				uint8_t* dstPtr = myData + overriddenParam.Offset;
				memcpy_s(dstPtr, info->Size, srcPtr, overriddenParam.Size);
			}
		}
	}

	myIsParameterDataDirty = false;


}

unsigned MaterialInstance::GetTextureSlotByName(const std::string& aName) const
{
	return myParentMaterial->GetTextureSlotByName(aName);
}

bool MaterialInstance::SetTexture(const std::string& aName, const std::shared_ptr<Texture>& aTexture)
{
	const unsigned slot = GetTextureSlotByName(aName);
	return SetTexture(slot, aTexture);
}

bool MaterialInstance::SetTexture(unsigned aSlot, const std::shared_ptr<Texture>& aTexture)
{
	if (aSlot >= MAX_MATERIAL_TEXTURE_COUNT)
		return false;

	myTextures[aSlot] = aTexture;
	return true;
}

std::shared_ptr<Texture> MaterialInstance::GetTexture(const std::string& aName) const
{
	const unsigned slot = GetTextureSlotByName(aName);
	return GetTexture(slot);
}

std::shared_ptr<Texture> MaterialInstance::GetTexture(unsigned aSlot) const
{
	if (aSlot >= MAX_MATERIAL_TEXTURE_COUNT)
		return nullptr;

	if (!myTextures[aSlot].has_value())
	{
		return myParentMaterial->GetTexture(aSlot);
	}

	return myTextures[aSlot].value();
}

bool MaterialInstance::SetRawParameterValue(const MaterialParameterInfo& aParamInfo, const void* aPtr, size_t aPtrSize)
{
	if (aParamInfo.Offset > MATERIAL_BUFFER_SIZE
		|| aParamInfo.Offset + aParamInfo.Size > MATERIAL_BUFFER_SIZE
		|| aParamInfo.Size > aPtrSize
		)
		return false;

	if (aParamInfo.Index >= myOverridenParameters.size())
	{
		return false;
	}

	memcpy_s(myData + aParamInfo.Offset, aParamInfo.Size, aPtr, aParamInfo.Size);

	myIsParameterDataDirty = true;
	myOverridenParameters[aParamInfo.Index] = true;

	return true;
}

