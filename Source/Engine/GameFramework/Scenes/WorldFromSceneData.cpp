#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AssetHandling/MaterialAsset.h"
#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "GameFramework/Components/StaticMeshComponent.h"
#include "GameFramework/GameFrameworkLog.h"
#include "GameFramework/Scenes/WorldFromSceneData.h"
#include "GameFramework/World/World.h"

#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace
{
	class PlaceholderComponent final : public SceneComponent
	{
	public:
		explicit PlaceholderComponent(PlaceholderComponentType aType, PlaceholderProperties someProperties)
			: Type(aType), Properties(std::move(someProperties)) {}
		PlaceholderComponentType Type;
		PlaceholderProperties Properties;
	};

	const ComponentData& Common(const ComponentRecord& aRecord)
	{
		return std::visit(
			[](const auto& aValue) 
			-> const ComponentData& 
			{ 
				return aValue.Common; 
			}, 
			aRecord);
	}

	void ApplyComponentData(Component& aComponent, const ComponentData& someData)
	{
		aComponent.SetSourceMetadata(someData.Tags, someData.SourceParent);
		aComponent.SetEnabled(someData.Enabled);

		SceneComponent* sceneComponent = dynamic_cast<SceneComponent*>(&aComponent);
		if (sceneComponent && !sceneComponent->GetTransform().SetData(someData.Transform))
		{
			throw std::runtime_error("Invalid component transform");
		}
	}

	bool ApplyMesh(MeshComponentBase& aComponent, const StaticMeshData& someData, AssetRegistry& anAssetRegistry, const SceneFallbackAssets& someFallbacks)
	{
		std::shared_ptr<MeshAsset> mesh = anAssetRegistry.GetAsset<MeshAsset>(someData.MeshName);
		if (!mesh)
		{
			GFLOG(Warning, "Using fallback cube for mesh component '{}': {}", someData.Common.Name, anAssetRegistry.GetLastError());
			mesh = someFallbacks.MissingMesh;
			if (!mesh)
			{
				return false;
			}
		}
		const bool usingFallbackMesh = mesh == someFallbacks.MissingMesh;

		aComponent.SetMesh(mesh);
		aComponent.SetVisible(someData.Visible);

		if (!usingFallbackMesh && someData.Materials.size() > aComponent.GetMaterialCount())
		{
			throw std::runtime_error("Material list exceeds mesh slots");
		}

		for (unsigned materialSlot = 0; materialSlot < aComponent.GetMaterialCount(); ++materialSlot)
		{
			std::shared_ptr<MaterialAsset> material;
			if (!usingFallbackMesh && materialSlot < someData.Materials.size())
			{
				const MaterialInstanceData& materialData = someData.Materials[materialSlot];
				material = anAssetRegistry.GetAsset<MaterialAsset>(materialData.Name);
				if (!material)
				{
					GFLOG(Warning, "Cannot load material '{}' for mesh component '{}': {}", materialData.Name, someData.Common.Name, anAssetRegistry.GetLastError());
				}
			}

			if (!material)
			{
				// TODO: Bind a dedicated error texture/material when those assets are available.
				GFLOG(Warning, "Using fallback material for mesh component '{}' at slot {}.", someData.Common.Name, materialSlot);
				material = someFallbacks.MissingMaterial;
				if (!material)
				{
					return false;
				}
			}

			if (!aComponent.SetMaterial(materialSlot, material))
			{
				throw std::runtime_error("Material slot is invalid: " + std::to_string(materialSlot));
			}
		}
		return true;
	}

	Component* CreateComponent(Actor& anActor, const ComponentRecord& aRecord, AssetRegistry& anAssetRegistry, CommonUtilities::Vector2u aClientSize, const SceneFallbackAssets& someFallbacks)
	{
		const ComponentData& common = Common(aRecord);
		// Create the component that matches the data stored in this record.
		return std::visit([&](const auto& someComponentData) -> Component*
		{
			using ComponentType = std::decay_t<decltype(someComponentData)>;
			if constexpr (std::is_same_v<ComponentType, SceneComponentData>)
			{
				return anActor.AddComponent<SceneComponent>(common.Name);
			}
			else if constexpr (std::is_same_v<ComponentType, CameraData>)
			{
				return anActor.AddComponent<CameraComponent>(common.Name, someComponentData.FieldOfView,
				                                                  someComponentData.NearPlane, someComponentData.FarPlane, aClientSize);
			}
			else if constexpr (std::is_same_v<ComponentType, StaticMeshData>)
			{
				StaticMeshComponent* meshComponent = anActor.AddComponent<StaticMeshComponent>(common.Name);
				if (!ApplyMesh(*meshComponent, someComponentData, anAssetRegistry, someFallbacks))
				{
					// No usable mesh or material was available, so discard this component.
					meshComponent->Destroy();
					return nullptr;
				}
				return meshComponent;
			}
			else if constexpr (std::is_same_v<ComponentType, SkeletalMeshData>)
			{
				SkeletalMeshComponent* meshComponent = anActor.AddComponent<SkeletalMeshComponent>(common.Name);
				if (!ApplyMesh(*meshComponent, someComponentData, anAssetRegistry, someFallbacks))
				{
					meshComponent->Destroy();
					return nullptr;
				}
				if (meshComponent->GetMesh() == someFallbacks.MissingMesh)
				{
					// The fallback cube has no skeleton to animate.
					return meshComponent;
				}
				if (!someComponentData.PartialRoot.empty() &&
				    !meshComponent->ConfigurePartialLayerFromJointName(someComponentData.PartialRoot))
				{
					throw std::runtime_error("Invalid partial root");
				}
				if (!someComponentData.InitialAnimation.empty() &&
				    !meshComponent->PlayAnimation(someComponentData.InitialAnimation, someComponentData.Loop))
				{
					throw std::runtime_error("Invalid animation");
				}
				return meshComponent;
			}
			else if constexpr (std::is_same_v<ComponentType, DirectionalLightData>)
			{
				DirectionalLightComponent* light = anActor.AddComponent<DirectionalLightComponent>(common.Name);
				light->SetColor(someComponentData.Color);
				light->SetSourceColorAlpha(someComponentData.ColorAlpha);
				light->SetIntensity(someComponentData.Intensity);
				return light;
			}
			else if constexpr (std::is_same_v<ComponentType, PointLightData>)
			{
				PointLightComponent* light = anActor.AddComponent<PointLightComponent>(common.Name);
				light->SetColor(someComponentData.Color);
				light->SetSourceColorAlpha(someComponentData.ColorAlpha);
				light->SetIntensity(someComponentData.Intensity);
				light->SetRadius(someComponentData.Radius);
				light->SetFalloffExponent(someComponentData.FalloffExponent);
				return light;
			}
			else if constexpr (std::is_same_v<ComponentType, SpotLightData>)
			{
				SpotLightComponent* light = anActor.AddComponent<SpotLightComponent>(common.Name);
				light->SetColor(someComponentData.Color);
				light->SetSourceColorAlpha(someComponentData.ColorAlpha);
				light->SetIntensity(someComponentData.Intensity);
				light->SetRadius(someComponentData.Radius);
				light->SetFalloffExponent(someComponentData.FalloffExponent);
				light->SetConeAnglesDegrees(someComponentData.InnerConeDegrees, someComponentData.OuterConeDegrees);
				return light;
			}
			else
			{
				// Preserve component types that do not have a runtime implementation yet.
				return anActor.AddComponent<PlaceholderComponent>(common.Name, someComponentData.Type, someComponentData.Properties);
			}
		}, aRecord);
	}
}

std::unique_ptr<World> BuildWorldFromSceneData(const SceneData& aScene, AssetRegistry& anAssetRegistry, CommonUtilities::Vector2u aClientSize, const SceneFallbackAssets& someFallbacks)
{
	std::unique_ptr<World> world = std::make_unique<World>();
	CameraComponent* selectedCamera = nullptr;
	std::vector<std::string> diagnostics;

	for (const ActorRecord& actorData : aScene.Actors)
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

		actor->SetSourceMetadata(actorData.Archetype, actorData.Tags);
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
				Component* created = CreateComponent(*actor, record, anAssetRegistry, aClientSize, someFallbacks);
				if (created == nullptr)
				{
					continue;
				}

				ApplyComponentData(*created, common);

				// If this component is a camera, remember it so we can set it as the active camera for the world.
				if (std::holds_alternative<CameraData>(record))
				{
					selectedCamera = static_cast<CameraComponent*>(created);
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
	if (selectedCamera)
	{
		world->SetActiveCamera(selectedCamera);
	}
	return world;
}
