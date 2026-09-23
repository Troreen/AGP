#include "InputFixture.h"
#include "GameFramework/Scenes/ComponentRegistry.h"
#include "EnumKeyCode.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/SceneComponent.h"
#include "GameFramework/Components/DebugCameraController.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/UnrealSceneImporter/UnrealSceneImporter.h"
#include "GraphicsEngine/Objects/Mesh.h"
#include "GraphicsEngine/Objects/Font.h"
#include "GraphicsEngine/TextWidget.h"
#include "GameFramework/Runtime/GameApplication.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string_view>
#include <type_traits>

struct FontTestAccess
{
	static std::shared_ptr<Font> Make()
	{
		auto font = std::make_shared<Font>();
		font->myLineHeight = 1.25f;
		font->myAscender = 0.0f;
		font->myAtlasWidth = 100;
		font->myAtlasHeight = 100;
		Font::Glyph visible;
		visible.Advance = 0.5f;
		visible.HasGeometry = true;
		visible.PlaneBounds = {0.0f, 0.0f, 1.0f, 0.5f};
		visible.AtlasBounds = {0.0f, 0.0f, 10.0f, 10.0f};
		visible.Unicode = 'A';
		font->myGlyphs['A'] = visible;
		visible.Unicode = '?';
		font->myGlyphs['?'] = visible;
		Font::Glyph space;
		space.Unicode = ' ';
		space.Advance = 0.5f;
		font->myGlyphs[' '] = space;
		return font;
	}
};

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
	InputFixture fixture;
	auto& input = fixture.Input;
	input.BindActionToInputCode("TestAction", EKeyCode::R);
	bool actionReceived = false;
	const unsigned listener = input.AddEventListener("TestAction", [&](const CommonUtilities::InputEvent& event) { actionReceived = event.inputData.isPressed; });
	fixture.Key(EKeyCode::R, true); input.Update();
	World world;
	auto* probe = world.SpawnActor("A")->AddComponent<Probe>("P", count);
	probe->OnUpdate = [&]
	{
		Check(ServiceLocator::GetInstance().GetInputMapper() == &input && actionReceived, "Frame input unavailable");
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
	input.RemoveEventListener(listener);
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
	AssetRegistry assets;
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

	SceneData missingAsset;
	ActorRecord missingActor;
	missingActor.Name = "MissingAssetActor";
	StaticMeshData missingMesh;
	missingMesh.Common.Name = "MissingMesh";
	missingMesh.Mesh = AssetId{"Meshes/DoesNotExist.fbx"};
	missingActor.Components.push_back(missingMesh);
	missingAsset.Actors.push_back(std::move(missingActor));
	auto partialWorld = registry.CreateWorld(missingAsset, assets);
	Check(partialWorld->FindActor("MissingAssetActor") != nullptr &&
	      partialWorld->FindActor("MissingAssetActor")->FindComponent("MissingMesh") == nullptr,
	      "Missing asset did not skip only its component");
}

void AssetRegistrySemantics()
{
	Check(AssetRegistry::NormalizeId("./Meshes\\Props\\SM_Chest.FBX") == "meshes/props/sm_chest.fbx",
	      "Asset ID normalization");

	const std::filesystem::path root = std::filesystem::temp_directory_path() / "agp_asset_registry_tests";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root / "Meshes/A");
	std::filesystem::create_directories(root / "Meshes/B");
	std::ofstream(root / "Meshes/A/Unique.fbx").put('\0');
	std::ofstream(root / "Meshes/A/Shared.fbx").put('\0');
	std::ofstream(root / "Meshes/B/Shared.fbx").put('\0');
	std::ofstream(root / "MissingParent.mat") << R"({"name":"MissingParent","masterMaterial":"Absent"})";
	std::ofstream(root / "Malformed.mat") << "{ not-json";

	AssetRegistry assets;
	assets.Initialize(root);
	int loads = 0;
	assets.SetMeshLoader([&](const std::filesystem::path&)
	{
		++loads;
		return std::make_shared<Mesh>();
	});
	{
		const auto first = assets.ResolveMesh(AssetId{"MESHES/A/UNIQUE.FBX"});
		const auto alias = assets.ResolveMesh(AssetId{"Unique"});
		Check(first && alias && loads == 1, "Case-insensitive exact/unique-alias lookup or cache failed");
	}
	Check(assets.ResolveMesh(AssetId{"Meshes/A/Unique.fbx"}) && loads == 2, "Expired weak asset was not reloaded");
	Check(!assets.ResolveMesh(AssetId{"Shared"}) && assets.GetLastError().find("ambiguous") != std::string::npos,
	      "Ambiguous basename lookup was accepted");
	Check(!assets.ResolveMaterial(AssetId{"Meshes/A/Unique.fbx"}), "Type-mismatched asset lookup was accepted");
	Check(!assets.ResolveMaterial(AssetId{"MissingParent.mat"}) && assets.GetLastError().find("unavailable parent") != std::string::npos,
	      "Missing material parent was accepted");
	Check(!assets.ResolveMaterial(AssetId{"Malformed.mat"}) && assets.GetLastError().find("malformed JSON") != std::string::npos,
	      "Malformed material JSON was accepted");

	auto builtIn = std::make_shared<Mesh>();
	assets.RegisterMesh(AssetId{"Engine/BasicShapes/Test"}, builtIn);
	builtIn.reset();
	Check(bool(assets.ResolveMesh(AssetId{"Engine/BasicShapes/Test"})), "Pinned built-in asset expired");
	assets.Clear();
	std::filesystem::remove_all(root);
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

void InputMapperSemantics()
{
    InputFixture fixture;
    auto& input = fixture.Input;
    input.BindActionToInputCode("Action", EKeyCode::R);
    std::vector<CommonUtilities::InputData> events;
    const unsigned listener = input.AddEventListener("Action", [&](const CommonUtilities::InputEvent& event) { events.push_back(event.inputData); });
    fixture.Key(EKeyCode::R, true); input.Update(); input.Update();
    fixture.Key(EKeyCode::R, false); input.Update(); input.Update();
    Check(events.size() == 3 && events[0].isPressed && events[0].isHeld && events[1].isHeld && !events[1].isPressed && events[2].isReleased, "Mapper press/hold/release or duplicate device update");
    fixture.Key(EKeyCode::R, true); input.Update();
    fixture.Handler.UpdateEvents(WM_KILLFOCUS, 0, 0); input.Update();
    Check(events.size() == 5 && events.back().isReleased, "Focus loss did not release held controls");
    input.RemoveEventListener(listener);
    fixture.Key(EKeyCode::R, true); input.Update();
    Check(events.size() == 5, "Removed mapper listener still receives events");
    input.BindActionToInputCode("Pointer", EPointerCode::MOUSE_DELTA);
    CommonUtilities::Vector2f delta{};
    const unsigned pointer = input.AddEventListener("Pointer", [&](const CommonUtilities::InputEvent& event) { delta = {event.inputData.valueA, event.inputData.valueB}; });
    fixture.Move(12, -7); input.Update();
    Check(delta.x == 12 && delta.y == -7, "Native mouse delta did not reach mapper");
    fixture.Handler.SetMouseDeltaEnabled(false);
    delta = {}; fixture.Move(20, 20); input.Update();
    Check(delta.LengthSqr() == 0, "Disabled mouse look still emitted motion");
    fixture.Handler.SetMouseDeltaEnabled(true);
    fixture.Move(3, 4); input.Update();
    Check(delta.x == 3 && delta.y == 4, "Re-enabled mouse look retained stale motion");
    input.RemoveEventListener(pointer);
    class Listener final : public Component
    {
    public:
        int& Calls; unsigned Id = 0;
        explicit Listener(int& calls) : Calls(calls) {}
        void BeginPlay() override { Id = ServiceLocator::GetInstance().GetInputMapper()->AddEventListener("Action", [this](const CommonUtilities::InputEvent&) { ++Calls; }); }
        void EndPlay() noexcept override { ServiceLocator::GetInstance().GetInputMapper()->RemoveEventListener(Id); }
    };
    int calls = 0;
    World world;
    for (int reload = 0; reload < 3; ++reload)
    {
        world.SpawnActor("Listener")->AddComponent<Listener>("Input", calls);
        world.BeginPlay(); input.Update();
        Check(calls == reload + 1, "Reload accumulated listeners");
        world.Clear(); input.Update();
        Check(calls == reload + 1, "Destroyed component left a dangling listener");
    }
    ServiceLocator::GetInstance().SetAssetRegistry(new AssetRegistry);
	ServiceLocator::GetInstance().KillServices();
	Check(ServiceLocator::GetInstance().GetInputMapper() == nullptr, "Owned input survived KillServices");
	bool assetsCleared = false;
	try { ServiceLocator::GetInstance().GetAssetRegistry(); } catch (const std::logic_error&) { assetsCleared = true; }
	Check(assetsCleared, "Owned assets survived KillServices");
    auto* replacement = new CommonUtilities::InputMapper;
    replacement->Init(&fixture.Handler);
    ServiceLocator::GetInstance().SetInputMapper(replacement);
    ServiceLocator::GetInstance().SetInputMapper(replacement);
    Check(ServiceLocator::GetInstance().GetInputMapper() == replacement, "Same-pointer registration lost owned mapper");
    replacement->Update();
    auto* newer = new CommonUtilities::InputMapper;
    newer->Init(&fixture.Handler);
    ServiceLocator::GetInstance().SetInputMapper(newer);
    newer->Update();
    ServiceLocator::GetInstance().KillServices();
    ServiceLocator::GetInstance().KillServices();
}

void UnrealImportPipeline()
{
	UnrealSceneImporter importer;
	auto imported = importer.ImportScene("Content/ExportedScenes/TestExportMap_Level.json");
	Check(bool(imported) && !imported.Data->Actors.empty(), "Supplied Unreal fixture did not parse");
	const StaticMeshData* importedMesh = nullptr;
	for (const ActorRecord& actor : imported.Data->Actors)
	{
		for (const ComponentRecord& component : actor.Components)
		{
			const auto* candidate = std::get_if<StaticMeshData>(&component);
			if (candidate && !candidate->Materials.empty() && !candidate->Materials.front().Parameters.empty())
			{
				importedMesh = candidate;
				break;
			}
		}
		if (importedMesh) break;
	}
	Check(importedMesh && importedMesh->Materials.size() == 1 && importedMesh->Materials.front().Parameters.size() == 2,
		"Importer duplicated material records or parameters");
	const auto greenActor = std::find_if(imported.Data->Actors.begin(), imported.Data->Actors.end(),
		[](const ActorRecord& actor) { return actor.Name == "PointLight2"; });
	const auto* greenLight = greenActor == imported.Data->Actors.end() || greenActor->Components.empty()
		? nullptr : std::get_if<PointLightData>(&greenActor->Components.front());
	Check(greenLight && greenLight->Color.x == 0 && greenLight->Color.y == 1 && greenLight->Color.z == 0 && greenLight->ColorAlpha == 1,
		"Importer did not preserve all four light color channels");
	const auto& first = imported.Data->Actors.front();
	Check(first.Archetype == "StaticMeshActor" && !first.Components.empty(), "Actor archetype/components were lost");
	Check(importedMesh && !importedMesh->MeshName.empty() && !importedMesh->ContentPath.empty(), "Mesh/material export data was lost");

	auto chestShowcase = importer.ImportScene("Content/ExportedScenes/ChestMaterials_Level.json");
	Check(bool(chestShowcase) && chestShowcase.Data->Actors.size() == 5, "Chest material showcase did not parse");
	const auto amberChest = std::find_if(chestShowcase.Data->Actors.begin(), chestShowcase.Data->Actors.end(),
		[](const ActorRecord& actor) { return actor.Name == "Chest_AmberGlass"; });
	const auto marbleChest = std::find_if(chestShowcase.Data->Actors.begin(), chestShowcase.Data->Actors.end(),
		[](const ActorRecord& actor) { return actor.Name == "Chest_MarbleGlass"; });
	const auto* amberMesh = amberChest == chestShowcase.Data->Actors.end() || amberChest->Components.empty()
		? nullptr : std::get_if<StaticMeshData>(&amberChest->Components.front());
	const auto* marbleMesh = marbleChest == chestShowcase.Data->Actors.end() || marbleChest->Components.empty()
		? nullptr : std::get_if<StaticMeshData>(&marbleChest->Components.front());
	Check(amberMesh && amberMesh->Mesh.Value == "/Game/Meshes/Props/SM_Chest.SM_Chest" &&
	      amberMesh->Materials.size() == 1 && amberMesh->Materials.front().Parent.Value == "Shaders/ChestMaterial_Alpha.mat" &&
	      amberMesh->Materials.front().Parameters.size() == 1,
	      "Chest tint material was not preserved by Unreal adaptation");
	Check(marbleMesh && marbleMesh->Materials.size() == 1 &&
	      marbleMesh->Materials.front().Parent.Value == "Shaders/ChestMaterial_Alpha2.mat",
	      "Chest texture-override material was not preserved by Unreal adaptation");
	const auto sun = std::find_if(chestShowcase.Data->Actors.begin(), chestShowcase.Data->Actors.end(),
		[](const ActorRecord& actor) { return actor.Name == "SunLight"; });
	const auto doubleLight = std::find_if(chestShowcase.Data->Actors.begin(), chestShowcase.Data->Actors.end(),
		[](const ActorRecord& actor) { return actor.Name == "DoubleLight"; });
	const auto* directional = sun == chestShowcase.Data->Actors.end() || sun->Components.empty()
		? nullptr : std::get_if<DirectionalLightData>(&sun->Components.front());
	const auto* centerPoint = doubleLight == chestShowcase.Data->Actors.end() || doubleLight->Components.size() != 2
		? nullptr : std::get_if<PointLightData>(&doubleLight->Components[0]);
	const auto* orbitPoint = doubleLight == chestShowcase.Data->Actors.end() || doubleLight->Components.size() != 2
		? nullptr : std::get_if<PointLightData>(&doubleLight->Components[1]);
	Check(directional && centerPoint && orbitPoint, "Exported directional/double-light components were not preserved");
	Check(centerPoint->Common.Transform.Position.LengthSqr() == 0 && orbitPoint->Common.Transform.Position.LengthSqr() > 0,
	      "DoubleLight component offsets were not preserved");

	auto missing = importer.ImportScene("Tests/GameFramework/does-not-exist.json");
	Check(!missing.Data && !missing.Diagnostics.empty(), "Parser failure became an empty scene");
	auto unknown = importer.ImportScene("Tests/GameFramework/UnknownTypeScene.json");
	Check(!unknown.Data && !unknown.Diagnostics.empty(), "Unknown TypeID was accepted");

	auto blockout = importer.ImportScene("Content/ExportedScenes/lvl_blockout/Lvl_Blockout_Level.json");
	Check(bool(blockout) && blockout.Data->Actors.size() == 16, "Content blockout scene did not parse");
	const auto character = std::find_if(blockout.Data->Actors.begin(), blockout.Data->Actors.end(),
		[](const ActorRecord& actor) { return actor.Name == "BP_TopDownCharacter"; });
	const auto* springArm = character == blockout.Data->Actors.end() || character->Components.size() < 3
		? nullptr : std::get_if<PlaceholderComponentData>(&character->Components[2]);
	const auto* springProperties = springArm ? std::get_if<SpringArmPlaceholderData>(&springArm->Properties) : nullptr;
	Check(springProperties && springProperties->SocketOffset.x == 0 && springProperties->SocketOffset.y == 0 &&
		springProperties->SocketOffset.z == 0 && springProperties->ArmLength == 1800,
		"Blockout spring-arm data was not imported");

}

void DebugCameraActions()
{
	InputFixture fixture; auto& input = fixture.Input; input.BindActionToInputCode("DebugCamera", EKeyCode::F1); World world; DebugCameraService service;
	auto* originalActor = world.SpawnActor("Authored Camera"); auto* original = originalActor->AddComponent<CameraComponent>("View");
	world.SetActiveCamera(original); world.BeginPlay();
	const unsigned listener = input.AddEventListener("DebugCamera", [&](const CommonUtilities::InputEvent& event)
	{
		if (event.inputData.isPressed) service.Toggle(world, {640,360});
	});
	fixture.Key(EKeyCode::F1, true); input.Update();
	Check(world.GetActiveCamera() != original && world.FindActor("__DebugCamera"), "F1 did not lazily spawn/activate debug camera");
	fixture.Key(EKeyCode::F1, false); input.Update(); fixture.Key(EKeyCode::F1, true); input.Update();
	Check(world.GetActiveCamera() == original, "F1 did not restore prior camera");
	fixture.Key(EKeyCode::F1, false); input.Update(); fixture.Key(EKeyCode::F1, true); input.Update();
	originalActor->Destroy(); world.Update(0);
	fixture.Key(EKeyCode::F1, false); input.Update(); fixture.Key(EKeyCode::F1, true); input.Update();
	Check(world.GetActiveCamera() && world.GetActiveCamera()->GetOwner()->GetName() == "__DebugCamera", "Destroyed previous camera displaced debug camera");
	input.RemoveEventListener(listener);
}

void TextGeometryAndNotificationTiming()
{
	auto font = FontTestAccess::Make();
	TextWidget text;
	text.SetFont(font);
	text.SetPixelHeight(20.0f);
	text.SetText("A");
	Check(text.RebuildGeometry() && text.GetVertices().size() == 4 && text.GetIndices().size() == 6,
	      "One text glyph did not create four vertices and six indices");
	text.SetText(" A");
	Check(text.RebuildGeometry() && text.GetGlyphCount() == 1 && text.GetVertices().front().Position.x == 10.0f,
	      "Space did not advance without geometry");
	text.SetText("A\nA");
	Check(text.RebuildGeometry() && text.GetGlyphCount() == 2 && text.GetVertices()[4].Position.y == 25.0f,
	      "Newline did not move the glyph cursor");
	text.SetText("\xCE\xA9");
	Check(text.RebuildGeometry() && text.GetGlyphCount() == 1 && text.GetVertices().size() == 4,
	      "Missing Unicode glyph did not use the '?' fallback");
	text.SetText("");
	Check(text.RebuildGeometry() && text.GetVertices().empty() && text.GetIndices().empty(), "Empty text produced a draw");

	RenderPassNotificationTimer timer;
	timer.Restart();
	Check(timer.GetOpacity() == 1.0f, "Notification was not opaque at 0 seconds");
	timer.Update(1.5f);
	Check(timer.GetOpacity() == 1.0f, "Notification was not opaque at 1.5 seconds");
	timer.Update(0.25f);
	Check(std::abs(timer.GetOpacity() - 0.5f) < 0.0001f, "Notification fade was wrong at 1.75 seconds");
	timer.Restart();
	Check(timer.GetRemaining() == 2.0f && timer.GetOpacity() == 1.0f, "Notification reset did not restart its timer");
	timer.Update(2.0f);
	Check(!timer.IsVisible() && timer.GetOpacity() == 0.0f, "Notification remained visible at 2 seconds");
}

void FontAssetDiagnostics()
{
	const auto root = std::filesystem::temp_directory_path() / "agp_font_asset_tests";
	std::filesystem::create_directories(root);
	{
		std::ofstream malformed(root / "Malformed.font.json");
		malformed << R"({"atlasFile":"Missing.dds","atlas":{},"metrics":{},"glyphs":[]})";
	}
	AssetRegistry assets;
	assets.Initialize(root);
	Check(!assets.ResolveFont(AssetId{"Malformed.font.json"}) && assets.GetLastError().find("metrics") != std::string::npos,
	      "Font loading accepted missing metrics without useful diagnostics");
	{
		std::ofstream missingAtlas(root / "MissingAtlas.font.json");
		missingAtlas << R"({"atlasFile":"Missing.dds","atlas":{"distanceRange":4,"width":32,"height":32},"metrics":{"lineHeight":1,"ascender":-0.8,"descender":0.2},"glyphs":[{"unicode":63,"advance":0.5}]})";
	}
	assets.Initialize(root);
	Check(!assets.ResolveFont(AssetId{"MissingAtlas.font.json"}) && assets.GetLastError().find("requires atlas") != std::string::npos,
	      "Font loading accepted a missing atlas without useful diagnostics");
	assets.Clear();
	std::filesystem::remove_all(root);
}

int main(int argc, char** argv)
{
	try
	{
		if (argc > 1 && std::string_view(argv[1]) == "--input-only")
		{
			FrameTimingAndInput(); InputMapperSemantics(); DebugCameraActions();
			std::cout << "PASS: native mapper input, focus loss, listener/world lifecycle and service ownership\n";
			return 0;
		}
		if (argc > 1 && std::string_view(argv[1]) == "--import-only")
		{
			UnrealImportPipeline();
			std::cout << "PASS: Unreal import pipeline\n";
			return 0;
		}
		OwnershipAndLifecycle();
		TransformSemantics();
		RuntimeMutations();
		FrameTimingAndInput();
		SceneConstruction();
		AssetRegistrySemantics();
		StartupAndCameraSafety();
		InputMapperSemantics();
		UnrealImportPipeline();
		DebugCameraActions();
		TextGeometryAndNotificationTiming();
		FontAssetDiagnostics();
		std::cout << "PASS: MVP ownership, lifecycle, runtime mutations, timing/input, registered scenes/properties and camera cleanup\n";
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
