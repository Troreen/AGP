#include "GameFramework/Scenes/ComponentRegistry.h"
#include "GameFramework/Components/CameraComponent.h"

std::unique_ptr<World> ComponentRegistry::CreateWorld(const SceneData& scene, const AssetLibrary& assets, const GameInput* input,
                                                      CommonUtilities::Vector2u size) const
{
	auto world = std::make_unique<World>(input);
	for (const auto& record : scene.Actors)
	{
		auto* actor = world->SpawnActor(record.Name);
		actor->SetActive(record.Active);
		if (!actor->GetTransform().SetLocalPose(record.Pose))
		{
			throw std::runtime_error(record.Name + ": Invalid Actor transform");
		}
		for (const auto& description : record.Components)
		{
			try
			{
				const auto factory = myFactories.find(description.Type);
				if (factory == myFactories.end())
				{
					throw std::runtime_error("Unknown component type: " + description.Type);
				}
				const SceneReader fields(description.Properties, assets, size);
				auto* component = factory->second(*actor, description, fields);
				component->SetEnabled(description.Enabled);
				if (description.Pose)
				{
					auto* spatial = dynamic_cast<SceneComponent*>(component);
					if (!spatial || !spatial->GetTransform().SetLocalPose(*description.Pose))
					{
						throw std::runtime_error("Invalid spatial component transform");
					}
				}
			}
			catch (const std::bad_alloc&)
			{
				throw;
			}
			catch (const std::exception& error)
			{
				throw std::runtime_error(record.Name + "/" + description.Name + ": " + error.what());
			}
		}
	}
	if (!scene.ActiveCameraActor.empty())
	{
		auto* actor = world->FindActor(scene.ActiveCameraActor);
		auto* camera = actor ? actor->GetComponent<CameraComponent>() : nullptr;
		if (!camera)
		{
			throw std::runtime_error("Active camera not found: " + scene.ActiveCameraActor);
		}
		world->SetActiveCamera(camera);
	}
	// The host begins play only after every Actor and Component is configured.
	return world;
}
