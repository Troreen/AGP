#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "GameFramework/Components/StaticMeshComponent.h"
#include "GameFramework/GameFrameworkLog.h"
#include "GameFramework/Scenes/ComponentRegistry.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <type_traits>

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
		SceneComponent* sceneComponent = dynamic_cast<SceneComponent*>(&component);
		if (sceneComponent && !sceneComponent->GetTransform().SetData(data.Transform))
		{
			throw std::runtime_error("Invalid component transform");
		}
	}

	bool ApplyMesh(MeshComponentBase& component, const StaticMeshData& data, AssetRegistry& assets)
	{
		const std::shared_ptr<MeshAsset> mesh = assets.GetAsset<MeshAsset>(data.MeshName);
		if (!mesh)
		{
			GFLOG(Warning, "Skipping mesh component '{}': {}", data.Common.Name, assets.GetLastError());
			return false;
		}
		component.SetMesh(mesh);
		component.SetVisible(data.Visible);

		if (data.Materials.size() != component.GetMaterialCount())
		{
			throw std::runtime_error("Material list must match mesh slots");
		}

		for (size_t materialSlot = 0; materialSlot < data.Materials.size(); ++materialSlot)
		{
			const MaterialInstanceData& materialData = data.Materials[materialSlot];
			const std::shared_ptr<MaterialAsset> material = assets.GetAsset<MaterialAsset>(materialData.Name);

			if (!material)
			{
				GFLOG(Warning, "Skipping mesh component '{}': material '{}' is unavailable at slot {}.",
				      data.Common.Name, materialData.Name, materialSlot);
				return false;
			}

			if (!component.SetMaterial(static_cast<unsigned>(materialSlot), material))
			{
				throw std::runtime_error("Material slot is invalid: " + std::to_string(materialSlot));
			}
		}
		return true;
	}

	Component* CreateComponent(Actor& actor, const ComponentRecord& record, AssetRegistry& assets,
	                           CommonUtilities::Vector2u clientSize)
	{
		const ComponentData& common = Common(record);
		return std::visit([&](const auto& componentData) -> Component*
		{
			using ComponentType = std::decay_t<decltype(componentData)>;
			if constexpr (std::is_same_v<ComponentType, SceneComponentData>)
			{
				return actor.AddComponent<SceneComponent>(common.Name);
			}
			else if constexpr (std::is_same_v<ComponentType, CameraData>)
			{
				return actor.AddComponent<CameraComponent>(common.Name, componentData.FieldOfView,
				                                                  componentData.NearPlane, componentData.FarPlane, clientSize);
			}
			else if constexpr (std::is_same_v<ComponentType, StaticMeshData>)
			{
				StaticMeshComponent* meshComponent = actor.AddComponent<StaticMeshComponent>(common.Name);
				if (!ApplyMesh(*meshComponent, componentData, assets))
				{
					meshComponent->Destroy();
					return nullptr;
				}
				return meshComponent;
			}
			else if constexpr (std::is_same_v<ComponentType, SkeletalMeshData>)
			{
				SkeletalMeshComponent* meshComponent = actor.AddComponent<SkeletalMeshComponent>(common.Name);
				if (!ApplyMesh(*meshComponent, componentData, assets))
				{
					meshComponent->Destroy();
					return nullptr;
				}
				if (!componentData.PartialRoot.empty() &&
				    !meshComponent->ConfigurePartialLayerFromJointName(componentData.PartialRoot))
				{
					throw std::runtime_error("Invalid partial root");
				}
				if (!componentData.InitialAnimation.empty() &&
				    !meshComponent->PlayAnimation(componentData.InitialAnimation, componentData.Loop))
				{
					throw std::runtime_error("Invalid animation");
				}
				return meshComponent;
			}
			else if constexpr (std::is_same_v<ComponentType, DirectionalLightData>)
			{
				DirectionalLightComponent* light = actor.AddComponent<DirectionalLightComponent>(common.Name);
				light->SetColor(componentData.Color);
				light->SetSourceColorAlpha(componentData.ColorAlpha);
				light->SetIntensity(componentData.Intensity);
				return light;
			}
			else if constexpr (std::is_same_v<ComponentType, PointLightData>)
			{
				PointLightComponent* light = actor.AddComponent<PointLightComponent>(common.Name);
				light->SetColor(componentData.Color);
				light->SetSourceColorAlpha(componentData.ColorAlpha);
				light->SetIntensity(componentData.Intensity);
				light->SetRadius(componentData.Radius);
				light->SetFalloffExponent(componentData.FalloffExponent);
				return light;
			}
			else if constexpr (std::is_same_v<ComponentType, SpotLightData>)
			{
				SpotLightComponent* light = actor.AddComponent<SpotLightComponent>(common.Name);
				light->SetColor(componentData.Color);
				light->SetSourceColorAlpha(componentData.ColorAlpha);
				light->SetIntensity(componentData.Intensity);
				light->SetRadius(componentData.Radius);
				light->SetFalloffExponent(componentData.FalloffExponent);
				light->SetConeAnglesDegrees(componentData.InnerConeDegrees, componentData.OuterConeDegrees);
				return light;
			}
			else
			{
				return actor.AddComponent<PlaceholderComponent>(common.Name, componentData.Type, componentData.Properties);
			}
		}, record);
	}
}

std::unique_ptr<World> ComponentRegistry::CreateWorld(const SceneData& scene, AssetRegistry& assets,
	CommonUtilities::Vector2u size) const
{
	std::unique_ptr<World> world = std::make_unique<World>();
	CameraComponent* taggedCamera = nullptr;
	std::vector<std::string> diagnostics;

	for (const ActorRecord& actorData : scene.Actors)
	{
		Actor* actor = nullptr;
		try
		{
			actor = world->SpawnActor(actorData.Name);
		}
		catch (const std::exception& error)
		{
			diagnostics.push_back(actorData.Name + ": " + error.what());
			continue;
		}
		actor->myArchetype = actorData.Archetype;
		actor->myTags = actorData.Tags;
		actor->SetActive(actorData.Active);
		if (!actor->GetTransform().SetData(actorData.Transform))
		{
			diagnostics.push_back(actorData.Name + ": Invalid Actor transform");
		}

		for (const ComponentRecord& record : actorData.Components)
		{
			const ComponentData& common = Common(record);
			try
			{
				Component* created = CreateComponent(*actor, record, assets, size);
				if (created == nullptr)
				{
					continue;
				}

				ApplyMetadata(*created, common);

				const bool componentActiveTag = std::find(common.Tags.begin(), common.Tags.end(), "ActiveCamera") != common.Tags.end();
				const bool actorActiveTag = std::find(actorData.Tags.begin(), actorData.Tags.end(), "ActiveCamera") != actorData.Tags.end();
				CameraComponent* camera = dynamic_cast<CameraComponent*>(created);

				if (componentActiveTag && !camera)
				{
					throw std::runtime_error("ActiveCamera component tag requires a camera");
				}
				if (camera && (componentActiveTag || actorActiveTag))
				{
					if (taggedCamera)
					{
						throw std::runtime_error("ActiveCamera tag must identify exactly one camera");
					}
					taggedCamera = camera;
				}
			}
			catch (const std::exception& error)
			{
				diagnostics.push_back(actorData.Name + "/" + common.Name + ": " + error.what());
			}
		}
	}
	if (!diagnostics.empty())
	{
		std::ostringstream message;
		message << "Scene construction failed:";
		for (const std::string& diagnostic : diagnostics)
		{
			message << "\n - " << diagnostic;
		}
		throw std::runtime_error(message.str());
	}
	if (taggedCamera)
	{
		world->SetActiveCamera(taggedCamera);
	}
	return world;
}
