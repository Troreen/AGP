#include "GameFramework/Scenes/ComponentRegistry.h"
#include "GameFramework/Components/CameraComponent.h"
#include <cmath>
#include <iostream>
#include <limits>

void Check(bool value, const char* message)
{
	if (!value)
	{
		throw std::runtime_error(message);
	}
}

struct Counts
{
	int Begins = 0, Updates = 0, Ends = 0, Destroyed = 0;
	float Delta = 0;
};

class Probe : public Component
{
public:
	Counts* Count = nullptr;
	float Speed = 0;
	std::function<void()> OnBegin, OnUpdate;
	Probe() = default;

	explicit Probe(Counts& count) : Count(&count)
	{
	}

	~Probe() override
	{
		if (Count)
		{
			++Count->Destroyed;
		}
	}

	void BeginPlay() override
	{
		if (Count)
		{
			++Count->Begins;
		}
		if (OnBegin)
		{
			OnBegin();
		}
	}

	void Update(float dt) override
	{
		if (Count)
		{
			++Count->Updates;
			Count->Delta = dt;
		}
		if (OnUpdate)
		{
			OnUpdate();
		}
	}

	void EndPlay() noexcept override
	{
		if (Count)
		{
			++Count->Ends;
		}
	}
};

void OwnershipAndLifecycle()
{
	Counts counts;
	World world;
	auto* actor = world.SpawnActor("Player");
	auto* component = actor->AddComponent<Probe>("Behavior", counts);
	Check(world.FindActor("Player") == actor && component->GetOwner() == actor && &component->GetWorld() == &world, "Ownership");
	Check(actor->GetComponent<Probe>() == component && actor->FindComponent("Behavior") == component, "Add/GetComponent");
	component->SetEnabled(false);
	world.BeginPlay();
	world.BeginPlay();
	Check(counts.Begins == 1 && counts.Updates == 0, "BeginPlay must occur once even when disabled");
	world.Update(.01f);
	Check(counts.Updates == 0, "Disabled component updated");
	component->SetEnabled(true);
	actor->SetActive(false);
	world.Update(.01f);
	Check(counts.Updates == 0, "Inactive Actor updated");
	actor->SetActive(true);
	world.Update(.01f);
	Check(counts.Updates == 1, "Enabled component missed Update");
	actor->Destroy();
	Check(!world.FindActor("Player"), "Destroyed Actor remains findable");
	world.Update(.01f);
	Check(counts.Updates == 1 && counts.Ends == 1 && counts.Destroyed == 1, "Actor destruction lifetime");
	world.Clear();
	world.Clear();
	Check(counts.Ends == 1, "EndPlay repeated");
}

void RuntimeMutations()
{
	Counts sourceCount, spawnedCount, actorCount, victimCount;
	World world;
	auto* sourceActor = world.SpawnActor("Source");
	auto* source = sourceActor->AddComponent<Probe>("Source", sourceCount);
	auto* victim = world.SpawnActor("Victim")->AddComponent<Probe>("Victim", victimCount);
	Probe* spawned = nullptr;
	source->OnUpdate = [&]
	{
		if (sourceCount.Updates != 1)
		{
			return;
		}
		spawned = sourceActor->AddComponent<Probe>("Spawned", spawnedCount);
		world.SpawnActor("New Actor")->AddComponent<Probe>("New", actorCount);
		victim->Destroy();
	};
	world.BeginPlay();
	world.Update(.01f);
	Check(spawnedCount.Begins == 0 && actorCount.Updates == 0, "Additions entered their creation frame");
	Check(victimCount.Updates == 0, "Destroyed later component updated in the same frame");
	world.Update(.01f);
	Check(spawnedCount.Begins == 1 && spawnedCount.Updates == 1 && actorCount.Updates == 1, "Runtime additions not started");
	Check(victimCount.Ends == 1 && victimCount.Destroyed == 1, "Component destruction cleanup");
	spawned->Destroy();
	world.Update(.01f);
	Check(spawnedCount.Updates == 1 && spawnedCount.Ends == 1, "Destroyed component updated");
	world.Clear();
	Check(sourceCount.Ends == 1 && actorCount.Ends == 1, "Clear missed live objects");
}

void FrameTimingAndInput()
{
	Counts count;
	GameInput input;
	input.KeysPressed[static_cast<size_t>(Keys::R)] = true;
	World world(&input);
	auto* probe = world.SpawnActor("A")->AddComponent<Probe>("P", count);
	probe->OnUpdate = [&]
	{
		Check(probe->GetInput().IsKeyPressed(Keys::R), "Frame input unavailable");
	};
	world.Update(.001f);
	Check(count.Begins == 1 && count.Updates == 1 && count.Delta == .001f, "Short frame did not update once");
	world.Update(10.f);
	Check(count.Updates == 2 && count.Delta == .25f, "Long frame must clamp, not catch up");
	for (float dt : {0.f, -1.f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()})
	{
		world.Update(dt);
		Check(count.Delta == 0, "Invalid delta must become zero");
	}
	Check(count.Updates == 6, "Expected exactly one Update per call");
}

void SceneConstruction()
{
	Counts count;
	ComponentRegistry registry;
	registry.Register<Probe>("Move", [&](Probe& component, const SceneReader& fields)
	{
		component.Count = &count;
		component.Speed = fields.OptionalFloat("speed", 1.f);
		component.OnBegin = [&component]
		{
			Check(component.Speed == 7.f && component.GetWorld().FindActor("Second"), "BeginPlay preceded configuration/construction");
		};
	});
	SceneData scene;
	ActorRecord actor;
	actor.Name = "First";
	actor.Pose.Position = {10, 20, 30};
	ComponentRecord component;
	component.Name = "Mover";
	component.Type = "Move";
	component.Properties["speed"] = 7.0;
	actor.Components.push_back(component);
	scene.Actors.push_back(actor);
	scene.Actors.push_back(ActorRecord{"Second"});
	AssetLibrary assets;
	auto world = registry.CreateWorld(scene, assets);
	auto* mover = world->FindActor("First")->GetComponent<Probe>();
	Check(mover->Speed == 7 && world->FindActor("First")->GetTransform().GetLocalPosition().x == 10, "Scene properties/transform");
	Check(count.Begins == 0, "Construction started behavior before host activation");
	world->BeginPlay();
	world->Update(.01f);
	Check(count.Begins == 1 && count.Updates == 1, "Scene component lifecycle");
	world.reset();
	Check(count.Ends == 1 && count.Destroyed == 1, "Scene ownership cleanup");

	scene.Actors[0].Components[0].Type = "MissingType";
	bool failed = false;
	try
	{
		registry.CreateWorld(scene, assets);
	}
	catch (const std::runtime_error& error)
	{
		const std::string message = error.what();
		failed = message.find("First/Mover") != std::string::npos && message.find("MissingType") != std::string::npos;
	}
	Check(failed, "Unknown component did not fail with useful context");
	scene.Actors[0].Components[0].Type = "Move";
	scene.Actors[0].Components[0].Properties["speed"] = std::string("wrong");
	failed = false;
	try
	{
		registry.CreateWorld(scene, assets);
	}
	catch (const std::runtime_error& error)
	{
		failed = std::string(error.what()).find("speed") != std::string::npos;
	}
	Check(failed, "Wrong property type accepted");
}

void StartupAndCameraSafety()
{
	Counts count;
	{
		World world;
		auto* actor = world.SpawnActor("Never started");
		actor->AddComponent<Probe>("P", count);
		actor->Destroy();
		world.Update(.01f);
	}
	Check(count.Begins == 0 && count.Ends == 0 && count.Destroyed == 1, "Unstarted objects received lifecycle hooks");
	{
		World world;
		auto* probe = world.SpawnActor("Throws")->AddComponent<Probe>("P", count);
		probe->OnBegin = []
		{
			throw std::runtime_error("begin failure");
		};
		try
		{
			world.BeginPlay();
		}
		catch (const std::runtime_error&)
		{
		}
	}
	Check(count.Begins == 1 && count.Ends == 1, "Partial BeginPlay was not cleaned up");
	World world;
	auto* actor = world.SpawnActor("Camera");
	auto* camera = actor->AddComponent<CameraComponent>();
	Check(world.SetActiveCamera(camera), "Camera selection");
	world.BeginPlay();
	camera->Destroy();
	world.Update(0);
	Check(!world.GetActiveCamera(), "Destroyed camera remains selected");
	auto* replacement = actor->AddComponent<CameraComponent>();
	world.SetActiveCamera(replacement);
	actor->Destroy();
	world.Update(0);
	Check(!world.GetActiveCamera(), "Destroyed camera Actor remains selected");
}

int main()
{
	try
	{
		OwnershipAndLifecycle();
		RuntimeMutations();
		FrameTimingAndInput();
		SceneConstruction();
		StartupAndCameraSafety();
		std::cout << "PASS: MVP ownership, lifecycle, runtime mutations, timing/input, registered scenes/properties and camera cleanup\n";
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
