#include "GameFramework/Scenes/AssetRefs.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/Scenes/SceneData.h"
#include "GameFramework/GameFrameworkLog.h"
#include "GraphicsEngine/Materials/Material.h"
#include <type_traits>

MaterialAsset CreateMaterialInstance(AssetRegistry& assets, const MaterialInstanceData& data)
{
	const AssetId parentId = data.Parent.Value.empty() ? AssetId{data.Name} : data.Parent;
	const MaterialAsset parentMaterial = assets.ResolveMaterial(parentId);
	if (!parentMaterial)
	{
		GFLOG(Warning, "Could not initialize material instance '{}': parent material '{}' is unavailable ({}).",
		      data.Name, parentId.Value, assets.GetLastError());
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
				return materialInstance->SetValue(parameter.Name, parameterValue);
			}
			else
			{
				const TextureAsset texture = assets.ResolveTexture(parameterValue);
				return texture && materialInstance->SetTexture(parameter.Name, texture.myResource);
			}
		}, parameter.Value);

		if (!applied)
		{
			GFLOG(Warning, "Could not apply parameter '{}' to material instance '{}'.", parameter.Name, data.Name);
			return {};
		}
	}

	MaterialAsset material;
	material.myResource = std::move(materialInstance);
	return material;
}
