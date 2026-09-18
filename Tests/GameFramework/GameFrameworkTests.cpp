#include "GameFramework/Scenes/ComponentRegistry.h"
#include "EnumKeys.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/SceneComponent.h"
#include "GameFramework/Components/DebugCameraController.h"
#include "GameFramework/UnrealSceneImporter/UnrealSceneAdapter.h"
#include "GameFramework/UnrealSceneImporter/UnrealSceneImporter.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <type_traits>

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
	InputSystem input;
	const InputActionId testAction{"TestAction"};
	input.BindKey(testAction, int(Keys::R));
	bool actionReceived = false;
	auto subscription = input.Subscribe(testAction, [&](const InputActionEvent& event) { actionReceived = event.Phase == InputActionPhase::Started; });
	InputDeviceFrame frame;
	frame.KeysDown[static_cast<size_t>(Keys::R)] = true;
	input.Update(frame);
	World world(&input);
	auto* probe = world.SpawnActor("A")->AddComponent<Probe>("P", count);
	probe->OnUpdate = [&]
	{
		Check(&probe->GetInputSystem() == &input && actionReceived, "Frame input unavailable");
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
	ComponentRegistry registry;
	SceneData scene;
	ActorRecord actor;
	actor.Name = "First";
	actor.Archetype = "FixtureActor";
	actor.Tags = {"Test"};
	actor.Transform.Position = {10, 20, 30};
	PlaceholderComponentData component;
	component.Common.Name = "Collider";
	component.Common.Tags = {"Shape"};
	component.Common.SourceParent = "Root";
	component.Type = PlaceholderComponentType::Box;
	actor.Components.push_back(std::move(component));
	scene.Actors.push_back(actor);
	AssetLibrary assets;
	auto world = registry.CreateWorld(scene, assets);
	auto* built = world->FindActor("First");
	auto* builtComponent = built->FindComponent("Collider");
	Check(built->GetArchetype() == "FixtureActor" && built->HasTag("Test") && built->GetTransform().GetLocalPosition().x == 10,
	      "Actor scene data");
	Check(builtComponent->HasTag("Shape") && builtComponent->GetSourceParent() == "Root", "Component scene data");
	world->BeginPlay();
	Check(builtComponent->HasBegunPlay(), "Typed scene component lifecycle");
	SceneData invalid; ActorRecord badA; badA.Name = "BadA"; badA.Transform.Position.x = std::numeric_limits<float>::infinity(); invalid.Actors.push_back(badA);
	ActorRecord badB; badB.Name = "BadB"; badB.Transform.Scale.y = std::numeric_limits<float>::quiet_NaN(); invalid.Actors.push_back(badB);
	try { registry.CreateWorld(invalid, assets); Check(false, "Invalid candidate scene was accepted"); }
	catch (const std::runtime_error& error) { const std::string message = error.what(); Check(message.find("BadA") != std::string::npos && message.find("BadB") != std::string::npos, "Construction diagnostics were not aggregated"); }
}

void TransformSemantics()
{
	static_assert(!std::is_copy_constructible_v<Transform> && !std::is_move_constructible_v<Transform>);
	static_assert(std::is_copy_constructible_v<TransformData>);
	Transform transform; TransformData authored{{3,4,5}, {725,-95,361}, {-2,3,0}};
	Check(transform.SetData(authored), "Finite transform rejected");
	Check(transform.GetData().RotationDegrees.x == 725 && transform.GetData().RotationDegrees.z == 361, "Authored Euler values were wrapped");
	Check(transform.GetLocalScale().x == -2 && transform.GetLocalScale().z == 0, "Negative/nonuniform/zero scale changed");
	const auto before = transform.GetData(); auto invalid = before; invalid.Position.x = std::numeric_limits<float>::infinity();
	Check(!transform.SetData(invalid) && transform.GetData().Position.x == before.Position.x, "Invalid transform update was not atomic");
	Transform directions; directions.SetLocalRotationDegrees(90, 0, 0);
	Check((directions.GetLocalForward() - CommonUtilities::Vector3f{1,0,0}).Length() < .0001f, "Yaw/direction convention changed");

	World world; auto* actor = world.SpawnActor("Root"); actor->GetTransform().SetLocalPosition({10,0,0});
	auto* child = actor->AddComponent<SceneComponent>("Offset"); child->GetTransform().SetLocalPosition({0,0,5});
	Check(child->GetWorldMatrix()(4,1) == 10 && child->GetWorldMatrix()(4,3) == 5, "componentLocal * actorWorld composition changed");
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

void InputSystemSemantics()
{
	InputSystem input;
	const InputActionId action{"Action"};
	input.BindKey(action, int(Keys::R));
	std::vector<InputActionPhase> phases;
	auto subscription = input.Subscribe(action, [&](const InputActionEvent& event) { phases.push_back(event.Phase); });
	InputDeviceFrame frame;
	frame.KeysDown[size_t(Keys::R)] = true;
	input.Update(frame);
	input.Update(frame);
	frame.KeysDown[size_t(Keys::R)] = false;
	input.Update(frame);
	Check(phases == std::vector{InputActionPhase::Started, InputActionPhase::Ongoing, InputActionPhase::Ended}, "Input phases");

	const InputActionId chord{"Chord"}, plain{"Plain"};
	input.BindKey(chord, int('7'), {int(Keys::SHIFT)});
	input.BindKey(plain, int('7'), {}, {int(Keys::SHIFT), int(Keys::LSHIFT), int(Keys::RSHIFT)});
	bool chordSeen = false, plainSeen = false;
	auto chordSub = input.Subscribe(chord, [&](const InputActionEvent& event) { if (event.Phase == InputActionPhase::Started) chordSeen = true; });
	auto plainSub = input.Subscribe(plain, [&](const InputActionEvent& event) { if (event.Phase == InputActionPhase::Started) plainSeen = true; });
	frame = {}; frame.KeysDown[size_t('7')] = true; frame.KeysDown[size_t(Keys::SHIFT)] = true; input.Update(frame);
	Check(chordSeen && !plainSeen, "Modifier chord also dispatched plain action");

	const InputActionId removal{"Removal"};
	input.BindKey(removal, int(Keys::F1));
	int callbacks = 0;
	InputSubscription later;
	auto first = input.Subscribe(removal, [&](const InputActionEvent&) { ++callbacks; later.Reset(); });
	later = input.Subscribe(removal, [&](const InputActionEvent&) { ++callbacks; });
	frame = {}; frame.KeysDown[size_t(Keys::F1)] = true; input.Update(frame);
	Check(callbacks == 2, "Listener removal changed active dispatch");
	input.Update(frame);
	Check(callbacks == 3, "Removed listener remained subscribed");

	const InputActionId pad{"Pad"};
	input.BindGamepadAxis2D(pad, false);
	bool padSeen = false;
	auto padSub = input.Subscribe(pad, [&](const InputActionEvent& event)
	{
		if (event.Phase == InputActionPhase::Started) padSeen = std::get<CommonUtilities::Vector2f>(event.Value).x == .5f;
	});
	frame = {}; frame.GamepadLeft = {.5f, 0}; input.Update(frame);
	Check(padSeen, "Synthetic gamepad axis unavailable");
	frame.Focused = false; input.Update(frame);

	InputSystem detailed;
	const InputActionId multi{"Multi"}, mouse{"Mouse"}, order{"Order"}, exception{"Exception"};
	detailed.BindKey(multi, int(Keys::A)); detailed.BindKey(multi, int(Keys::D)); detailed.BindMouseDelta(mouse, 2.f);
	int multiEnded = 0; auto multiSub = detailed.Subscribe(multi, [&](const InputActionEvent& event) { if (event.Phase == InputActionPhase::Ended) ++multiEnded; });
	std::vector<int> listenerOrder;
	auto order1 = detailed.Subscribe(order, [&](const InputActionEvent&) { listenerOrder.push_back(1); });
	auto order2 = detailed.Subscribe(order, [&](const InputActionEvent&) { listenerOrder.push_back(2); }); detailed.BindKey(order, int(Keys::W));
	CommonUtilities::Vector2f mouseValue; auto mouseSub = detailed.Subscribe(mouse, [&](const InputActionEvent& event) { mouseValue = std::get<CommonUtilities::Vector2f>(event.Value); });
	frame = {}; frame.KeysDown[size_t(Keys::A)] = true; frame.KeysDown[size_t(Keys::W)] = true; frame.MouseDelta = {2, -3}; detailed.Update(frame);
	Check(listenerOrder == std::vector{1,2} && mouseValue.x == 4 && mouseValue.y == -6, "Input ordering or mouse motion");
	frame.KeysDown[size_t(Keys::D)] = true; detailed.Update(frame); frame.KeysDown[size_t(Keys::A)] = false; detailed.Update(frame);
	Check(multiEnded == 0, "Multi-binding action ended while another binding remained active");
	frame.Focused = false; detailed.Update(frame); Check(multiEnded == 1, "Focus loss did not end active action");

	detailed.BindKey(exception, int(Keys::F2)); InputSubscription throwing;
	throwing = detailed.Subscribe(exception, [&](const InputActionEvent&) { throwing.Reset(); throw std::runtime_error("callback"); });
	frame = {}; frame.KeysDown[size_t(Keys::F2)] = true; try { detailed.Update(frame); } catch (const std::runtime_error&) {}
	frame.KeysDown[size_t(Keys::F2)] = false; detailed.Update(frame);
	InputSubscription survivor;
	{ InputSystem temporary; survivor = temporary.Subscribe(action, [](const InputActionEvent&) {}); }
	survivor.Reset();
}

void UnrealImportPipeline()
{
	UnrealSceneImporter importer;
	auto imported = importer.ImportScene("Content/ExportedScenes/TestExportMap_Level.json");
	Check(bool(imported) && !imported.Data->Actors.empty(), "Supplied Unreal fixture did not parse");
	const auto* importedMesh = std::get_if<ImportedMeshComponent>(&imported.Data->Actors.front().Components.front());
	Check(importedMesh && importedMesh->Materials.size() == 1 && importedMesh->Materials.front().Parameters.size() == 2,
		"Importer duplicated material records or parameters");
	const auto greenActor = std::find_if(imported.Data->Actors.begin(), imported.Data->Actors.end(),
		[](const UnrealActorData& actor) { return actor.Name == "PointLight2"; });
	const auto* greenLight = greenActor == imported.Data->Actors.end() || greenActor->Components.empty()
		? nullptr : std::get_if<ImportedPointLightComponent>(&greenActor->Components.front());
	Check(greenLight && greenLight->Color.x == 0 && greenLight->Color.y == 1 && greenLight->Color.z == 0 && greenLight->Color.w == 1,
		"Importer did not preserve all four light color channels");
	auto converted = UnrealSceneAdapter{}.Convert(*imported.Data);
	if (!converted && !converted.Diagnostics.empty()) std::cerr << converted.Diagnostics.front().Context << ": " << converted.Diagnostics.front().Message << '\n';
	Check(bool(converted), "Supplied Unreal fixture did not convert");
	const auto& first = converted.Scene->Actors.front();
	Check(first.Archetype == "StaticMeshActor" && !first.Components.empty(), "Actor archetype/components were lost");
	const auto* mesh = std::get_if<StaticMeshData>(&first.Components.front());
	Check(mesh && mesh->MeshName == "Plane" && !mesh->ContentPath.empty() && !mesh->Materials.empty() && !mesh->Materials.front().Parameters.empty(), "Mesh/material export data was lost");

	auto missing = importer.ImportScene("Tests/GameFramework/does-not-exist.json");
	Check(!missing.Data && !missing.Diagnostics.empty(), "Parser failure became an empty scene");
	auto unknown = importer.ImportScene("Tests/GameFramework/UnknownTypeScene.json");
	Check(!unknown.Data && !unknown.Diagnostics.empty(), "Unknown TypeID was accepted");

	auto blockout = importer.ImportScene("Content/ExportedScenes/lvl_blockout/Lvl_Blockout_Level.json");
	Check(bool(blockout) && blockout.Data->Actors.size() == 16, "Content blockout scene did not parse");
	const auto character = std::find_if(blockout.Data->Actors.begin(), blockout.Data->Actors.end(),
		[](const UnrealActorData& actor) { return actor.Name == "BP_TopDownCharacter"; });
	const auto* springArm = character == blockout.Data->Actors.end() || character->Components.size() < 3
		? nullptr : std::get_if<ImportedSpringArmComponent>(&character->Components[2]);
	Check(springArm && springArm->SocketOffset.x == 0 && springArm->SocketOffset.y == 0 &&
		springArm->SocketOffset.z == 0 && springArm->ArmLength == 1800,
		"Blockout spring-arm data was not imported");
	auto convertedBlockout = UnrealSceneAdapter{}.Convert(*blockout.Data);
	Check(bool(convertedBlockout) && convertedBlockout.Scene->Actors.size() == blockout.Data->Actors.size(),
		"Content blockout scene did not convert");

}

void DebugCameraActions()
{
	InputSystem input; InstallDefaultInputBindings(input); World world(&input); DebugCameraService service;
	auto* originalActor = world.SpawnActor("Authored Camera"); auto* original = originalActor->AddComponent<CameraComponent>("View");
	world.SetActiveCamera(original); world.BeginPlay();
	auto subscription = input.Subscribe(InputActions::DebugCamera, [&](const InputActionEvent& event)
	{
		if (event.Phase == InputActionPhase::Started) service.Toggle(world, {640,360});
	});
	InputDeviceFrame frame; frame.KeysDown[size_t(Keys::F1)] = true; input.Update(frame);
	Check(world.GetActiveCamera() != original && world.FindActor("__DebugCamera"), "F1 did not lazily spawn/activate debug camera");
	frame.KeysDown[size_t(Keys::F1)] = false; input.Update(frame); frame.KeysDown[size_t(Keys::F1)] = true; input.Update(frame);
	Check(world.GetActiveCamera() == original, "F1 did not restore prior camera");
	frame.KeysDown[size_t(Keys::F1)] = false; input.Update(frame); frame.KeysDown[size_t(Keys::F1)] = true; input.Update(frame);
	originalActor->Destroy(); world.Update(0);
	frame.KeysDown[size_t(Keys::F1)] = false; input.Update(frame); frame.KeysDown[size_t(Keys::F1)] = true; input.Update(frame);
	Check(world.GetActiveCamera() && world.GetActiveCamera()->GetOwner()->GetName() == "__DebugCamera", "Destroyed previous camera displaced debug camera");
}

int main()
{
	try
	{
		OwnershipAndLifecycle();
		TransformSemantics();
		RuntimeMutations();
		FrameTimingAndInput();
		SceneConstruction();
		StartupAndCameraSafety();
		InputSystemSemantics();
		UnrealImportPipeline();
		DebugCameraActions();
		std::cout << "PASS: MVP ownership, lifecycle, runtime mutations, timing/input, registered scenes/properties and camera cleanup\n";
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
