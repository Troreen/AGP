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
	struct CollisionCounter : Component
	{
		CollisionCounter(int& enters, int& exits) : Enters(enters), Exits(exits) {}
		void OnCollisionEnter(const Collision&) override { ++Enters; }
		void OnCollisionExit(const Collision&) override { ++Exits; }
		int& Enters;
		int& Exits;
	};

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
		CheckImport(box->IsEnabled() && box->GetLayer() == CollisionLayer::Default && box->GetMask() == UINT32_MAX,
		            "Untagged exported shapes keep enabled layer-zero all-layer defaults");
		int enters = 0;
		int exits = 0;
		box->GetOwner()->AddComponent<CollisionCounter>("Counter", enters, exits);
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
		collider->GetOwner()->AddComponent<CollisionCounter>("Counter", enters, exits);
		Actor* probe = world->SpawnActor("Probe");
		probe->AddComponent<ColliderComponent>("ProbeSphere");
		probe->GetTransform().SetWorldPosition({17, 20, 30});
		world->Update(0);
		CheckImport(enters == 1, "Root sphere offset and scale give radius six at converted center ten twenty thirty");
		probe->GetTransform().SetWorldPosition({17.2f, 20, 30});
		world->Update(0);
		CheckImport(exits == 1, "Root sphere scale is applied once");
	}

	void CodeConfiguredTriggerFiltering()
	{
		SyntheticSceneFile file(SyntheticActor(
		    R"({"Name":"Trigger","Parent":"","TypeID":6,"Tags":["CollisionLayer=Trigger","CollisionMask=Player|Enemy"],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"Bounds":{"x":2,"y":2,"z":2}})"));
		UnrealSceneImporter importer;
		auto imported = importer.ImportScene(file.Path);
		CheckImport(imported.Data.has_value() && imported.Diagnostics.empty(), "Collision tags are retained as ordinary component tags");
		AssetRegistry assets;
		SceneWorldBuilder builder;
		auto world = builder.BuildWorld(*imported.Data, assets, {}, {1280, 720});
		auto* trigger = world->FindActor("SyntheticActor")->GetComponent<ColliderComponent>();
		CheckImport(trigger != nullptr && trigger->GetLayer() == CollisionLayer::Default && trigger->GetMask() == UINT32_MAX,
		            "Importer leaves filtering at defaults even when collision tags are present");
		trigger->SetLayer(CollisionLayer::Trigger);
		trigger->SetOverlapsWith({CollisionLayer::Player, CollisionLayer::Enemy});
		int enters = 0;
		int exits = 0;
		trigger->GetOwner()->AddComponent<CollisionCounter>("Counter", enters, exits);
		auto* probe = world->SpawnActor("Probe")->AddComponent<ColliderComponent>("ProbeSphere");
		probe->SetLayer(CollisionLayer::Player);
		probe->SetOverlapsWith({CollisionLayer::Enemy});
		world->Update(0);
		CheckImport(enters == 0, "Trigger accepting Player is insufficient when Player rejects Trigger");
		probe->SetOverlapsWith({CollisionLayer::Trigger});
		world->Update(0);
		CheckImport(enters == 1, "Mutually accepted Player enters imported Trigger");
		probe->SetLayer(CollisionLayer::Default);
		world->Update(0);
		CheckImport(exits == 1, "Trigger mask excludes an unlisted layer");
		probe->SetLayer(CollisionLayer::Enemy);
		world->Update(0);
		CheckImport(enters == 2, "Code-configured mask also accepts Enemy");
	}

	void UnsupportedShapeLayouts()
	{
		const std::string box = R"({"Name":"Box","Parent":"Root","TypeID":6,"Tags":[],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"Bounds":{"x":2,"y":2,"z":2}})";
		const std::string sphere = R"({"Name":"Sphere","Parent":"Root","TypeID":7,"Tags":[],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"Radius":2})";
		const std::string root = R"({"Name":"Root","Parent":"","TypeID":0,"Tags":[],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]})";
		const std::string nested = R"({"Name":"Middle","Parent":"Root","TypeID":0,"Tags":[],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]},{"Name":"NestedBox","Parent":"Middle","TypeID":6,"Tags":[],"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],"Bounds":{"x":2,"y":2,"z":2}})";
		UnrealSceneImporter importer;
		SyntheticSceneFile multipleShapes(SyntheticActor(root + "," + box + "," + sphere));
		auto imported = importer.ImportScene(multipleShapes.Path);
		CheckImport(imported.Data.has_value(), "Importer leaves the one-collider rule to Actor attachment");
		AssetRegistry assets;
		SceneWorldBuilder builder;
		bool rejected = false;
		try
		{
			builder.BuildWorld(*imported.Data, assets, {}, {1280, 720});
		}
		catch (const std::runtime_error& error)
		{
			rejected = std::string(error.what()).find("An Actor can own only one collider") != std::string::npos;
		}
		CheckImport(rejected, "Actor attachment rejects multiple colliders during scene construction");

		SyntheticSceneFile nestedShape(SyntheticActor(root + "," + nested));
		imported = importer.ImportScene(nestedShape.Path);
		CheckImport(!imported.Data && !imported.Diagnostics.empty(), "Nested shape attachments reject the scene during import");
		CheckImport(imported.Diagnostics.front().Context.find("SyntheticActor") != std::string::npos,
		            "Unsupported collider layout diagnostic identifies its Actor");
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
		UnrealSceneImporter importer;
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
		box->GetOwner()->AddComponent<CollisionCounter>("Counter", enters, exits);
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
	CodeConfiguredTriggerFiltering();
	UnsupportedShapeLayouts();
	CapsuleRemainsPlaceholder();
	FullComponentSceneImports();
	SyntheticYawedUnequalBox();
}
