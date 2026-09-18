#include "GameFramework/Scenes/SceneData.h"
#include "GraphicsEngine/Materials/Material.h"
#include <type_traits>

MaterialAsset CreateMaterialInstance(IAssetResolver& assets, const MaterialInstanceData& data)
{
	const AssetId parentId = data.Parent.Value.empty() ? AssetId{data.Name} : data.Parent;
	const auto parent = assets.ResolveParentMaterial(parentId);
	if (!parent) return {};
	auto instance = MaterialInstance::Create(data.Name, parent.myResource);
	if (!instance) return {};
	for (const auto& parameter : data.Parameters)
	{
		const bool applied = std::visit([&](const auto& value)
		{
			using T = std::decay_t<decltype(value)>;
			if constexpr (std::is_same_v<T, float> || std::is_same_v<T, CommonUtilities::Vector4f>) return instance->SetValue(parameter.Name, value);
			else { auto texture = assets.ResolveTexture(value); return texture && instance->SetTexture(parameter.Name, texture); }
		}, parameter.Value);
		if (!applied) return {};
	}
	MaterialAsset result;
	result.myResource = std::move(instance);
	return result;
}
