#include "GameFramework/Scenes/SceneData.h"
#include "GameFramework/GameFrameworkLog.h"
#include "GraphicsEngine/Materials/Material.h"
#include <type_traits>

MaterialAsset CreateMaterialInstance(IAssetResolver& assets, const MaterialInstanceData& data)
{
	const AssetId parentId = data.Parent.Value.empty() ? AssetId{data.Name} : data.Parent;
	const MaterialAsset parentMaterial = assets.ResolveParentMaterial(parentId);
	if (!parentMaterial)
	{
		GFLOG(Warning, "Could not initialize material instance '{}': parent material '{}' is not bound.",
		      data.Name, parentId.Value);
		return {};
	}

	std::shared_ptr<MaterialInstance> materialInstance = MaterialInstance::Create(data.Name, parentMaterial.myResource);
	if (!materialInstance)
	{
		GFLOG(Warning, "Could not initialize material instance '{}' from parent '{}'.", data.Name, parentId.Value);
		return {};
	}
	for (const MaterialParameterData& parameter : data.Parameters)
	{
		const bool applied = std::visit([&](const auto& parameterValue)
		{
			using ParameterType = std::decay_t<decltype(parameterValue)>;
			if constexpr (std::is_same_v<ParameterType, float> || std::is_same_v<ParameterType, CommonUtilities::Vector4f>)
			{
				const bool valueWasSet = materialInstance->SetValue(parameter.Name, parameterValue);
				if (!valueWasSet)
				{
					GFLOG(Warning, "Could not initialize material instance '{}': value parameter '{}' is missing or incompatible.",
					      data.Name, parameter.Name);
				}
				return valueWasSet;
			}
			else
			{
				const std::shared_ptr<Texture> texture = assets.ResolveTexture(parameterValue);
				if (!texture)
				{
					GFLOG(Warning, "Could not initialize material instance '{}': texture '{}' for parameter '{}' is not bound.",
					      data.Name, parameterValue.Value, parameter.Name);
					return false;
				}
				const bool textureWasSet = materialInstance->SetTexture(parameter.Name, texture);
				if (!textureWasSet)
				{
					GFLOG(Warning, "Could not initialize material instance '{}': texture parameter '{}' does not exist on parent '{}'.",
					      data.Name, parameter.Name, parentId.Value);
				}
				return textureWasSet;
			}
		}, parameter.Value);
		if (!applied)
		{
			return {};
		}
	}
	MaterialAsset material;
	material.myResource = std::move(materialInstance);
	return material;
}
