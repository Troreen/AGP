#include "GameFramework/UnrealSceneImporter/UnrealSceneImporter.h"
#include "GameFramework/Scenes/SceneWorldBuilder.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/Components/ColliderComponent.h"
#include "GameFramework/World/World.h"
#include <stdexcept>
#include <limits>
#include <fstream>
#include <chrono>

namespace
{
	// Synthetic input isolates unsupported export cases; it is not evidence from Unreal.
	class SyntheticSceneFile
	{
	public:
		explicit SyntheticSceneFile(const std::string& actors)
		{
			Path = std::filesystem::temp_directory_path() /
			    ("AGP_CollisionImport_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".json");
			std::ofstream output(Path);
			output << "{\"MasterMaterials\":[],\"Actors\":[" << actors << "]}";
			if (!output)
			{
				throw std::runtime_error("Could not write synthetic collision import input");
			}
		}
		~SyntheticSceneFile()
		{
			std::error_code error;
			std::filesystem::remove(Path, error);
		}
		std::filesystem::path Path;
	};

	std::string SyntheticActor(const std::string& components)
	{
		return R"({"Name":"SyntheticActor","Archetype":"Actor","Tags":[],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"Components":[)" + components + "]}";
	}
}

namespace
{
	void CheckImport(bool value, const char* message)
	{
		if (!value)
		{
			throw std::runtime_error(message);
		}
	}

	void AuthoredTriggerBox()
	{
		// Geometry is copied from the real component test scene export, without its custom actions.
		UnrealSceneImporter importer;
		auto imported = importer.ImportScene("Tests/GameFramework/CollisionImportScene.json");
		CheckImport(imported.Data.has_value() && imported.Diagnostics.empty(), "Real exported TriggerBox imports without diagnostics");
		AssetRegistry assets;
		SceneWorldBuilder builder;
		auto world = builder.BuildWorld(*imported.Data, assets, {}, {1280, 720});
		Actor* actor = world->FindActor("SE_WorkflowTest");
		CheckImport(actor != nullptr, "Exported trigger Actor exists");
		auto* box = actor->GetComponent<ColliderComponent>();
		CheckImport(box != nullptr, "Exported TriggerBox becomes a working ColliderComponent");
		CheckImport(box->IsEnabled() && box->GetLayer() == 0 && box->GetMask() == UINT32_MAX,
		            "Untagged exported shapes keep enabled layer-zero all-layer defaults");
		int enters = 0;
		int exits = 0;
		auto enter = box->OnEnter([&](const Collision&) { ++enters; });
		auto exit = box->OnExit([&](const Collision&) { ++exits; });
		Actor* probe = world->SpawnActor("Probe");
		probe->AddComponent<ColliderComponent>("ProbeSphere");
		// Unreal's authored box center is (-130,254,-1560) after axis conversion.
		// Its half width is 280.48: 32 units at authored scale 8.765, applied once.
		probe->GetTransform().SetWorldPosition({150, 254, -1560});
		world->Update(0);
		CheckImport(enters == 1, "Authored box offset axes and scale place its outer face at x150.48");
		probe->GetTransform().SetWorldPosition({153, 254, -1560});
		world->Update(0);
		CheckImport(exits == 1, "Probe beyond the authored face exits; box scale is applied once");
	}

	void SyntheticRootSphere()
	{
		SyntheticSceneFile file(SyntheticActor(
		    R"({"Name":"RootSphere","Parent":"","TypeID":7,"Tags":[],"Transform":[3,0,0,0,0,3,0,0,0,0,3,0,30,10,20,1],"Radius":2})"));
		UnrealSceneImporter importer;
		auto imported = importer.ImportScene(file.Path);
		CheckImport(imported.Data.has_value() && imported.Diagnostics.empty(), "Synthetic root sphere imports");
		AssetRegistry assets;
		SceneWorldBuilder builder;
		auto world = builder.BuildWorld(*imported.Data, assets, {}, {1280, 720});
		auto* collider = world->FindActor("SyntheticActor")->GetComponent<ColliderComponent>();
		CheckImport(collider != nullptr, "Exported sphere becomes a working ColliderComponent");
		int enters = 0;
		int exits = 0;
		auto enter = collider->OnEnter([&](const Collision&) { ++enters; });
		auto exit = collider->OnExit([&](const Collision&) { ++exits; });
		Actor* probe = world->SpawnActor("Probe");
		probe->AddComponent<ColliderComponent>("ProbeSphere");
		probe->GetTransform().SetWorldPosition({17, 20, 30});
		world->Update(0);
		CheckImport(enters == 1, "Root sphere offset and scale give radius six at converted center ten twenty thirty");
		probe->GetTransform().SetWorldPosition({17.2f, 20, 30});
		world->Update(0);
		CheckImport(exits == 1, "Root sphere scale is applied once");
	}

	void TaggedTriggerFiltering()
	{
		SyntheticSceneFile file(SyntheticActor(
		    R"({"Name":"Trigger","Parent":"","TypeID":6,"Tags":["CollisionLayer=Trigger","CollisionMask=Player|Enemy"],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"Bounds":{"x":2,"y":2,"z":2}})"));
		UnrealSceneImporter importer({{"Player", 1}, {"Trigger", 2}, {"Enemy", 3}});
		auto imported = importer.ImportScene(file.Path);
		CheckImport(imported.Data.has_value() && imported.Diagnostics.empty(), "Named collision tags import");
		AssetRegistry assets;
		SceneWorldBuilder builder;
		auto world = builder.BuildWorld(*imported.Data, assets, {}, {1280, 720});
		auto* trigger = world->FindActor("SyntheticActor")->GetComponent<ColliderComponent>();
		CheckImport(trigger != nullptr && trigger->GetLayer() == 2 && trigger->GetMask() == ((1u << 1) | (1u << 3)),
		            "Shape tags configure the injected gameplay layer names");
		int enters = 0;
		int exits = 0;
		auto enter = trigger->OnEnter([&](const Collision&) { ++enters; });
		auto exit = trigger->OnExit([&](const Collision&) { ++exits; });
		auto* probe = world->SpawnActor("Probe")->AddComponent<ColliderComponent>("ProbeSphere");
		probe->SetLayer(1);
		probe->SetMask(1u << 3);
		world->Update(0);
		CheckImport(enters == 0, "Trigger accepting Player is insufficient when Player rejects Trigger");
		probe->SetMask(1u << 2);
		world->Update(0);
		CheckImport(enters == 1, "Mutually accepted Player enters imported Trigger");
		probe->SetLayer(0);
		world->Update(0);
		CheckImport(exits == 1, "Trigger mask excludes an unlisted layer");
		probe->SetLayer(3);
		world->Update(0);
		CheckImport(enters == 2, "Pipe-delimited Enemy also enters imported Trigger");
	}

	void InvalidAuthoredDimensions()
	{
		UnrealSceneImporter importer;
		for (const auto& shape : {
		         R"({"Name":"InvalidBox","Parent":"","TypeID":6,"Tags":[],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"Bounds":{"x":0,"y":2,"z":2}})",
		         R"({"Name":"InvalidSphere","Parent":"","TypeID":7,"Tags":[],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"Radius":-1})"})
		{
			SyntheticSceneFile file(SyntheticActor(shape));
			auto imported = importer.ImportScene(file.Path);
			CheckImport(!imported.Data && !imported.Diagnostics.empty(), "Invalid authored shape dimensions reject the scene with diagnostics");
			CheckImport(imported.Diagnostics.front().Context.find("Invalid") != std::string::npos,
			            "Invalid dimensions diagnostic identifies its shape component");
		}
	}

	void UnsupportedShapeLayouts()
	{
		const std::string box = R"({"Name":"Box","Parent":"Root","TypeID":6,"Tags":[],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"Bounds":{"x":2,"y":2,"z":2}})";
		const std::string sphere = R"({"Name":"Sphere","Parent":"Root","TypeID":7,"Tags":[],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"Radius":2})";
		const std::string root = R"({"Name":"Root","Parent":"","TypeID":0,"Tags":[],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]})";
		const std::string nested = R"({"Name":"Middle","Parent":"Root","TypeID":0,"Tags":[],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]},{"Name":"NestedBox","Parent":"Middle","TypeID":6,"Tags":[],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"Bounds":{"x":2,"y":2,"z":2}})";
		UnrealSceneImporter importer;
		for (const auto& components : {root + "," + box + "," + sphere, root + "," + nested})
		{
			SyntheticSceneFile file(SyntheticActor(components));
			auto imported = importer.ImportScene(file.Path);
			CheckImport(!imported.Data && !imported.Diagnostics.empty(),
			            "Multiple supported shapes and nested shape attachments reject the scene");
			CheckImport(imported.Diagnostics.front().Context.find("SyntheticActor") != std::string::npos,
			            "Unsupported collider layout diagnostic identifies its Actor");
		}
	}

	void CollisionTagDiagnostics()
	{
		UnrealSceneImporter importer({{"Player", 1}, {"Trigger", 2}, {"Enemy", 3}});
		for (const std::string tags : {
		         R"("CollisionLayer=Unknown")", R"("CollisionLayer=Default")",
		         R"("CollisionMask=Unknown")", R"("CollisionLayer=Player","CollisionLayer=Enemy")",
		         R"("CollisionMask=Player","CollisionMask=Enemy")", R"("CollisionLayer=")",
		         R"("CollisionMask=Player|")", R"("CollisionMask=Player||Enemy")",
		         R"("CollisionMask=Player| Enemy")", R"("CollisionLayer")", R"("CollisionMask")"})
		{
			SyntheticSceneFile file(SyntheticActor(
			    R"({"Name":"TaggedSphere","Parent":"","TypeID":7,"Tags":[)" + tags +
			    R"(],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"Radius":2})"));
			auto imported = importer.ImportScene(file.Path);
			CheckImport(!imported.Data && !imported.Diagnostics.empty(), "Unknown duplicate and malformed collision tags reject the scene");
			const auto& diagnostic = imported.Diagnostics.front();
			CheckImport(diagnostic.Context.find("TaggedSphere") != std::string::npos &&
			                diagnostic.Message.find("Collision") != std::string::npos,
			            "Collision tag diagnostic identifies its component and setting");
		}
	}

	void ExplicitEmptyMask()
	{
		SyntheticSceneFile file(SyntheticActor(
		    R"({"Name":"SilentSphere","Parent":"","TypeID":7,"Tags":["CollisionMask="],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"Radius":2})"));
		UnrealSceneImporter importer;
		auto imported = importer.ImportScene(file.Path);
		CheckImport(imported.Data.has_value() && imported.Diagnostics.empty(), "Explicit empty mask imports as accepting no layers");
		AssetRegistry assets;
		SceneWorldBuilder builder;
		auto world = builder.BuildWorld(*imported.Data, assets, {}, {1280, 720});
		auto* collider = world->FindActor("SyntheticActor")->GetComponent<ColliderComponent>();
		CheckImport(collider && collider->GetMask() == 0, "Empty mask differs from absent all-layer mask");
		int enters = 0;
		auto enter = collider->OnEnter([&](const Collision&) { ++enters; });
		world->SpawnActor("Probe")->AddComponent<ColliderComponent>("ProbeSphere");
		world->Update(0);
		CheckImport(enters == 0, "Explicit empty mask suppresses overlaps");
	}

	void CapsuleRemainsPlaceholder()
	{
		SyntheticSceneFile file(SyntheticActor(
		    R"({"Name":"Capsule","Parent":"","TypeID":8,"Tags":[],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"Radius":2,"HalfHeight":4})"));
		UnrealSceneImporter importer;
		auto imported = importer.ImportScene(file.Path);
		CheckImport(imported.Data.has_value() && imported.Diagnostics.empty(), "Capsule input retains existing import support");
		AssetRegistry assets;
		SceneWorldBuilder builder;
		auto world = builder.BuildWorld(*imported.Data, assets, {}, {1280, 720});
		auto* actor = world->FindActor("SyntheticActor");
		CheckImport(actor && actor->FindComponent("Capsule") && !actor->GetComponent<ColliderComponent>(),
		            "Deferred capsule is a placeholder rather than an active collider");
	}

	void FullComponentSceneImports()
	{
		UnrealSceneImporter importer({{"Player", 1}, {"Trigger", 2}, {"Enemy", 3}});
		auto imported = importer.ImportScene("Content/ExportedScenes/lvl_component_test/Lvl_Component_Test_Level.json");
		CheckImport(imported.Data.has_value() && imported.Diagnostics.empty(), "Full current component test scene imports without graphics resources");
	}

	void SyntheticYawedUnequalBox()
	{
		// Synthetic yaw and unequal dimensions independently check the Unreal-to-AGP axis mapping.
		SyntheticSceneFile file(R"({"Name":"YawedBoxActor","Archetype":"Actor","Tags":[],
		    "Transform":[1,0,0,0,0,1,0,0,0,0,1,0,30,10,20,1],"Components":[
		    {"Name":"Root","Parent":"","TypeID":0,"Tags":[],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]},
		    {"Name":"YawedBox","Parent":"Root","TypeID":6,"Tags":[],
		    "Transform":[0,1,0,0,-1,0,0,0,0,0,1,0,0,0,0,1],"Bounds":{"x":4,"y":1,"z":2}}]})");
		UnrealSceneImporter importer;
		auto imported = importer.ImportScene(file.Path);
		CheckImport(imported.Data.has_value() && imported.Diagnostics.empty(), "Synthetic yawed unequal box imports");
		AssetRegistry assets;
		SceneWorldBuilder builder;
		auto world = builder.BuildWorld(*imported.Data, assets, {}, {1280, 720});
		auto* box = world->FindActor("YawedBoxActor")->GetComponent<ColliderComponent>();
		CheckImport(box != nullptr, "Yawed box becomes a working ColliderComponent");
		int enters = 0;
		int exits = 0;
		auto enter = box->OnEnter([&](const Collision&) { ++enters; });
		auto exit = box->OnExit([&](const Collision&) { ++exits; });
		auto* probe = world->SpawnActor("Probe");
		probe->AddComponent<ColliderComponent>("ProbeSphere")->SetSphereRadius(0.1f);
		// After conversion and yaw the box has world half-extents (4,2,1), centered at (10,20,30).
		probe->GetTransform().SetWorldPosition({13.5f, 20, 30});
		world->Update(0);
		CheckImport(enters == 1, "Converted yaw puts the unequal box long axis along AGP X");
		probe->GetTransform().SetWorldPosition({10, 20, 33.5f});
		world->Update(0);
		CheckImport(exits == 1, "Converted yaw leaves the unequal box short axis along AGP Z");
	}
}

void RunCollisionImportTests()
{
	AuthoredTriggerBox();
	SyntheticRootSphere();
	TaggedTriggerFiltering();
	InvalidAuthoredDimensions();
	UnsupportedShapeLayouts();
	CollisionTagDiagnostics();
	ExplicitEmptyMask();
	CapsuleRemainsPlaceholder();
	FullComponentSceneImports();
	SyntheticYawedUnequalBox();
}
