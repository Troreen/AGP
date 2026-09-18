#include "GameFramework/Scenes/ComponentRegistry.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "GameFramework/Components/StaticMeshComponent.h"
#include <stdexcept>
#include <type_traits>
#include <algorithm>
#include <sstream>

namespace
{
	class PlaceholderComponent final : public SceneComponent
	{
	public:
		explicit PlaceholderComponent(PlaceholderComponentType type, PlaceholderProperties properties)
			: Type(type), Properties(std::move(properties)) {}
		PlaceholderComponentType Type;
		PlaceholderProperties Properties;
	};

	const ComponentData& Common(const ComponentRecord& record)
	{
		return std::visit([](const auto& value) -> const ComponentData& { return value.Common; }, record);
	}

	void ApplyMetadata(Component& component, const ComponentData& data)
	{
		component.SetSourceMetadata(data.Tags, data.SourceParent);
		component.SetEnabled(data.Enabled);
		if (auto* spatial = dynamic_cast<SceneComponent*>(&component); spatial && !spatial->GetTransform().SetData(data.Transform))
			throw std::runtime_error("Invalid component transform");
	}

	void ApplyMesh(MeshComponentBase& component, const StaticMeshData& data, IAssetResolver& assets)
	{
		component.SetSourceAssetIdentity(data.MeshName, data.ContentPath);
		const auto mesh = assets.ResolveMesh(data.Mesh);
		if (!mesh) throw std::runtime_error("Mesh asset is unavailable: " + data.Mesh.Value);
		component.SetMesh(mesh);
		component.SetVisible(data.Visible);
		if (data.Materials.size() != component.GetMaterialCount()) throw std::runtime_error("Material list must match mesh slots");
		for (unsigned i = 0; i < data.Materials.size(); ++i)
		{
			const auto material = CreateMaterialInstance(assets, data.Materials[i]);
			if (!material || !component.SetMaterial(i, material)) throw std::runtime_error("Material asset is unavailable at slot " + std::to_string(i));
		}
	}
}

std::unique_ptr<World> ComponentRegistry::CreateWorld(const SceneData& scene, IAssetResolver& assets, InputSystem* input,
	CommonUtilities::Vector2u size) const
{
	auto world = std::make_unique<World>(input);
	CameraComponent* taggedCamera = nullptr;
	std::vector<std::string> diagnostics;
	for (const auto& actorData : scene.Actors)
	{
		Actor* actor = nullptr;
		try { actor = world->SpawnActor(actorData.Name); }
		catch (const std::exception& error) { diagnostics.push_back(actorData.Name + ": " + error.what()); continue; }
		actor->myArchetype = actorData.Archetype;
		actor->myTags = actorData.Tags;
		actor->SetActive(actorData.Active);
		if (!actor->GetTransform().SetData(actorData.Transform)) diagnostics.push_back(actorData.Name + ": Invalid Actor transform");
		for (const auto& record : actorData.Components)
		{
			const auto& common = Common(record);
			try
			{
				Component* created = std::visit([&](const auto& data) -> Component*
				{
					using T = std::decay_t<decltype(data)>;
					if constexpr (std::is_same_v<T, SceneComponentData>) return actor->AddComponent<SceneComponent>(common.Name);
					else if constexpr (std::is_same_v<T, CameraData>)
						return actor->AddComponent<CameraComponent>(common.Name, data.FieldOfView, data.NearPlane, data.FarPlane, size);
					else if constexpr (std::is_same_v<T, StaticMeshData>) { auto* c = actor->AddComponent<StaticMeshComponent>(common.Name); ApplyMesh(*c, data, assets); return c; }
					else if constexpr (std::is_same_v<T, SkeletalMeshData>)
					{
						auto* c = actor->AddComponent<SkeletalMeshComponent>(common.Name); ApplyMesh(*c, data, assets);
						if (!data.PartialRoot.empty() && !c->ConfigurePartialLayerFromJointName(data.PartialRoot)) throw std::runtime_error("Invalid partial root");
						if (!data.InitialAnimation.empty() && !c->PlayAnimation(data.InitialAnimation, data.Loop)) throw std::runtime_error("Invalid animation");
						return c;
					}
					else if constexpr (std::is_same_v<T, DirectionalLightData>) { auto* c = actor->AddComponent<DirectionalLightComponent>(common.Name); c->SetColor(data.Color); c->SetSourceColorAlpha(data.ColorAlpha); c->SetIntensity(data.Intensity); return c; }
					else if constexpr (std::is_same_v<T, PointLightData>) { auto* c = actor->AddComponent<PointLightComponent>(common.Name); c->SetColor(data.Color); c->SetSourceColorAlpha(data.ColorAlpha); c->SetIntensity(data.Intensity); c->SetRadius(data.Radius); c->SetFalloffExponent(data.FalloffExponent); return c; }
					else if constexpr (std::is_same_v<T, SpotLightData>) { auto* c = actor->AddComponent<SpotLightComponent>(common.Name); c->SetColor(data.Color); c->SetSourceColorAlpha(data.ColorAlpha); c->SetIntensity(data.Intensity); c->SetRadius(data.Radius); c->SetFalloffExponent(data.FalloffExponent); c->SetConeAnglesDegrees(data.InnerConeDegrees, data.OuterConeDegrees); return c; }
					else return actor->AddComponent<PlaceholderComponent>(common.Name, data.Type, data.Properties);
				}, record);
				ApplyMetadata(*created, common);
					const bool componentActiveTag = std::find(common.Tags.begin(), common.Tags.end(), "ActiveCamera") != common.Tags.end();
					const bool actorActiveTag = std::find(actorData.Tags.begin(), actorData.Tags.end(), "ActiveCamera") != actorData.Tags.end();
					auto* camera = dynamic_cast<CameraComponent*>(created);
					if (componentActiveTag && !camera) throw std::runtime_error("ActiveCamera component tag requires a camera");
					if (camera && (componentActiveTag || actorActiveTag))
					{
						if (taggedCamera) throw std::runtime_error("ActiveCamera tag must identify exactly one camera");
					taggedCamera = camera;
				}
			}
			catch (const std::exception& error) { diagnostics.push_back(actorData.Name + "/" + common.Name + ": " + error.what()); }
		}
	}
	if (!diagnostics.empty())
	{
		std::ostringstream message; message << "Scene construction failed:";
		for (const auto& diagnostic : diagnostics) message << "\n - " << diagnostic;
		throw std::runtime_error(message.str());
	}
	if (taggedCamera) world->SetActiveCamera(taggedCamera);
	return world;
}
