#include "GraphicsEngine.pch.h"
#include "Material.h"

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
		for (size_t i = 0; i < overriddenParams.size(); i++)
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

bool MaterialInstance::SetRawParameterValue(const MaterialParameterInfo& aParamInfo, const void* aPtr, size_t aPtrSize)
{
	if (aParamInfo.Offset > MATERIAL_BUFFER_SIZE
		|| aParamInfo.Offset + aParamInfo.Size > MATERIAL_BUFFER_SIZE
		|| aParamInfo.Size > aPtrSize
		)
		return false;

	memcpy_s(myData + aParamInfo.Offset, aParamInfo.Size, aPtr, aParamInfo.Size);

	myIsParameterDataDirty = true;
	myOverridenParameters[aParamInfo.Index] = true;

	return true;
}

