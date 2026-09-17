#include "Scenes/SceneBuilder.h"
#include "Runtime/Internal/WorldAccess.h"
#include <map>
#include <set>

namespace
{
	SceneDiagnostic Location(const SceneData& scene, const ActorRecord* actor = nullptr, const ComponentRecord* component = nullptr)
	{
		SceneDiagnostic result;
		result.File = scene.Source.File;
		result.Actor = scene.Source.Object;
		result.Component = scene.Source.Component;
		result.Property = scene.Source.Field;
		if (actor)
		{
			if (!actor->Source.File.empty())
			{
				result.File = actor->Source.File;
			}
			result.Actor = actor->Source.Object.empty() ? actor->Id : actor->Source.Object;
			if (!actor->Source.Field.empty())
			{
				result.Property = actor->Source.Field;
			}
		}
		if (component)
		{
			if (!component->Source.File.empty())
			{
				result.File = component->Source.File;
			}
			if (!component->Source.Object.empty())
			{
				result.Actor = component->Source.Object;
			}
			result.Component = component->Source.Component.empty() ? component->Id : component->Source.Component;
			if (!component->Source.Field.empty())
			{
				result.Property = component->Source.Field;
			}
			result.Type = component->Type;
		}
		return result;
	}

	std::string Name(const ComponentRecord& component)
	{
		return component.Name.empty() ? component.Id : component.Name;
	}
}

SceneBuildResult SceneBuilder::Build(const SceneData& scene, const ComponentRegistry& registry, const SceneBuildServices& services)
{
	using GameFrameworkInternal::WorldAccess;
	SceneBuildResult result;
	auto error =
	    [&result](SceneDiagnostic source, std::string field, std::string message, std::string code, std::string phase = "preflight")
	{
		source.Property = source.Property.empty() ? field : field.empty() ? source.Property : source.Property + "." + field;
		source.Message = std::move(message);
		source.Code = std::move(code);
		source.Phase = std::move(phase);
		result.Diagnostics.push_back(std::move(source));
	};
	if (!registry.IsFrozen())
	{
		error(Location(scene), "registry", "Registry must be frozen", "registry-not-frozen");
	}
	std::set<std::string> actorIds;
	for (const auto& actor : scene.Actors)
	{
		if (actor.Id.empty() || !actorIds.insert(actor.Id).second)
		{
			error(Location(scene, &actor), "id", "Actor ID must be nonempty and unique", "invalid-id");
		}
		Transform pose;
		if (!pose.SetLocalPose(actor.Pose))
		{
			error(Location(scene, &actor), "pose", "Actor pose is invalid", "invalid-pose");
		}
		std::set<std::string> ids, names;
		for (const auto& component : actor.Components)
		{
			const auto source = Location(scene, &actor, &component);
			if (component.Id.empty() || !ids.insert(component.Id).second)
			{
				error(source, "id", "Component ID must be nonempty and actor-local unique", "invalid-id");
			}
			if (Name(component).empty() || !names.insert(Name(component)).second)
			{
				error(source, "name", "Component instance name must be actor-local unique", "invalid-name");
			}
			if (!registry.Contains(component.Type))
			{
				error(source, "type", "Unknown registered component type: " + component.Type, "unknown-type");
			}
			if (component.Pose && !pose.SetLocalPose(*component.Pose))
			{
				error(source, "pose", "Component pose is invalid", "invalid-pose");
			}
		}
	}
	if (!result.Diagnostics.empty())
	{
		return result;
	}

	auto candidate = WorldAccess::Create(services.Input);
	WorldAccess::BindServices(*candidate, services.Scenes, services.Assets, services.Time);
	std::map<std::string, Actor*> actors;
	std::map<std::pair<std::string, std::string>, Component*> components;
	auto actorLookup = [&actors](const std::string& id) -> Actor*
	{
		const auto it = actors.find(id);
		return it == actors.end() ? nullptr : it->second;
	};
	auto componentLookup = [&components](const ObjectAddress& address) -> Component*
	{
		const auto it = components.find({address.ActorId, address.ComponentId});
		return it == components.end() ? nullptr : it->second;
	};
	try
	{
		// Allocate everything before any registered reader can observe its peers.
		for (const auto& actor : scene.Actors)
		{
			auto* runtimeActor = candidate->SpawnActor(actor.Name.empty() ? actor.Id : actor.Name);
			actors.emplace(actor.Id, runtimeActor);
			for (const auto& component : actor.Components)
			{
				try
				{
					auto* runtimeComponent = registry.Create(component.Type, *runtimeActor, Name(component));
					components.emplace(std::make_pair(actor.Id, component.Id), runtimeComponent);
					auto source = Location(scene, &actor, &component);
					source.Phase = "resolve";
					WorldAccess::SetSource(*runtimeComponent, std::move(source));
				}
				catch (const std::bad_alloc&)
				{
					throw;
				}
				catch (const std::exception& exception)
				{
					error(Location(scene, &actor, &component), {}, exception.what(), "construction-failed", "allocation");
				}
				catch (...)
				{
					error(Location(scene, &actor, &component), {}, "Component factory threw", "construction-failed", "allocation");
				}
			}
		}
		if (!result.Diagnostics.empty())
		{
			return result;
		}

		// Apply hierarchy and basic values before readers, so configured cameras
		// and spatial properties observe the final authored parent graph.
		for (const auto& actor : scene.Actors)
		{
			auto* runtimeActor = actorLookup(actor.Id);
			runtimeActor->GetTransform().SetLocalPose(actor.Pose);
			runtimeActor->SetActive(actor.Active);
			if (!actor.Parent.empty())
			{
				auto* parent = actorLookup(actor.Parent);
				if (!parent || !runtimeActor->SetParent(parent, ReparentMode::KeepLocal))
				{
					error(Location(scene, &actor), "parent", "Missing or cyclic actor parent", "invalid-parent", "configuration");
				}
			}
			for (const auto& component : actor.Components)
			{
				auto* runtimeComponent = componentLookup({actor.Id, component.Id});
				runtimeComponent->SetEnabled(component.Enabled);
				auto* spatial = dynamic_cast<SceneComponent*>(runtimeComponent);
				if (!spatial && (component.Pose || !component.Parent.empty()))
				{
					error(Location(scene, &actor, &component), "pose", "Nonspatial components cannot have pose or attachment fields",
					      "nonspatial-transform", "configuration");
				}
				if (spatial)
				{
					if (component.Pose)
					{
						spatial->GetTransform().SetLocalPose(*component.Pose);
					}
					if (!component.Parent.empty())
					{
						auto* parent = dynamic_cast<SceneComponent*>(componentLookup({actor.Id, component.Parent}));
						if (!parent || !spatial->SetParent(parent, ReparentMode::KeepLocal))
						{
							error(Location(scene, &actor, &component), "parent", "Missing, nonspatial or cyclic component parent",
							      "invalid-parent", "configuration");
						}
					}
				}
			}
		}
		std::vector<std::unique_ptr<SceneReader>> readers;
		for (const auto& actor : scene.Actors)
		{
			for (const auto& component : actor.Components)
			{
				auto* runtimeComponent = componentLookup({actor.Id, component.Id});
				auto reader =
				    std::unique_ptr<SceneReader>(new SceneReader(component, Location(scene, &actor, &component), result.Diagnostics,
				                                                 services.Assets, services.ClientSize, actorLookup, componentLookup));
				try
				{
					WorldAccess::Configure(*runtimeComponent, [&registry, &component, runtimeComponent, &reader]
					{
						registry.Configure(component.Type, *runtimeComponent, *reader);
					});
				}
				catch (const std::bad_alloc&)
				{
					throw;
				}
				catch (const std::exception& exception)
				{
					error(Location(scene, &actor, &component), {}, exception.what(), "configuration-failed", "configuration");
				}
				catch (...)
				{
					error(Location(scene, &actor, &component), {}, "Registered reader threw", "configuration-failed", "configuration");
				}
				reader->Finish();
				readers.push_back(std::move(reader));
			}
		}
		for (auto& reader : readers)
		{
			reader->Resolve();
		}
		readers.clear(); // Drop fixup destinations before a failed candidate can die.

		if (scene.ActiveCamera)
		{
			auto* camera = dynamic_cast<CameraComponent*>(componentLookup(*scene.ActiveCamera));
			if (!camera || !camera->IsEnabled() || !camera->GetOwner()->IsActiveInHierarchy())
			{
				error(Location(scene), "camera", "Active camera must identify an enabled CameraComponent on an active actor",
				      "invalid-camera", "presentation");
			}
			else
			{
				candidate->SetActiveCamera(camera);
				result.Camera = camera->GetRef<CameraComponent>();
			}
		}
		else if (scene.RequireCamera)
		{
			error(Location(scene), "camera", "This scene requires an explicit camera", "missing-camera", "presentation");
		}
		if (result.Diagnostics.empty())
		{
			WorldAccess::Prepare(*candidate, result.Diagnostics);
		}
	}
	catch (const std::bad_alloc&)
	{
		throw;
	}
	catch (const std::exception& exception)
	{
		error(Location(scene), {}, exception.what(), "construction-failed", "construction");
	}
	catch (...)
	{
		error(Location(scene), {}, "Unknown scene construction exception", "construction-failed", "construction");
	}
	if (result.Diagnostics.empty())
	{
		result.Candidate = std::move(candidate);
	}
	return result;
}
