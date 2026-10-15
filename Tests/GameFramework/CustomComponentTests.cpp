#include "Components/RegisterCustomComponents.h"
#include "Components/CustomComponentRegistry.h"
#include "Components/SpinComponent.h"
#include "GameFramework/UnrealSceneImporter/UnrealSceneImporter.h"
#include "GameFramework/World/World.h"
#include "GameFramework/Scenes/SceneWorldBuilder.h"
#include "GameFramework/AssetHandling/AssetRegistry.h"
#include <cmath>
#include <iostream>

static void Check(bool value, const char* message)
{
	if (!value)
	{
		throw std::runtime_error(message);
	}
}

class SetupOrderComponent final : public CustomComponent
{
public:
	int VariableApplications = 0;

	void InitializeCustomVariables(const CustomComponentVariables& someVariables) override
	{
		Check(GetOwner() != nullptr && !HasBegunPlay(), "Variables must be applied after attachment and before BeginPlay");
		Check(!IsEnabled() && HasTag("SetupTag"), "Common data must be applied before custom variables");
		Check(someVariables.GetOrDefault<bool>("Ready", false), "Custom variables did not reach the component");
		++VariableApplications;
	}

	void BeginPlay() override
	{
		Check(VariableApplications == 1, "Custom variables must be applied once before BeginPlay");
		Check(GetOwner()->FindComponent("LaterComponent") != nullptr, "BeginPlay must run after all components are created");
	}
};

int RunCustomComponentTests()
{
	UnrealSceneImporter importer;
	auto importResult = importer.ImportScene("Content/ExportedScenes/lvl_component_test/Lvl_Component_Test_Level.json");
	Check(importResult.Data.has_value(), "Custom probe scene did not import");
	const CustomComponentData* spinComponentData = nullptr;
	for (const auto& actorData : importResult.Data->Actors)
	{
		for (const auto& componentRecord : actorData.Components)
		{
			const auto* customComponentData = std::get_if<CustomComponentData>(&componentRecord);
			if (customComponentData && customComponentData->CustomType == "SpinComponent_C")
			{
				spinComponentData = customComponentData;
			}
		}
	}
	Check(spinComponentData != nullptr, "Custom type identity was lost");
	const auto& customVariables = spinComponentData->Variables;
	const float spinSpeed = customVariables.GetOrDefault<float>("SpinSpeed", 0.0f);
	Check(std::abs(spinSpeed - 25.00001f) < 0.0001f, "Float property did not survive import");
	Check(customVariables.GetOrDefault<int64_t>("IntProbe", 0) == 7 && customVariables.BoolVariables.at("BoolProbe"),
	      "Integer and boolean properties did not survive import");
	Check(customVariables.StringVariables.at("TargetComponentName").empty(), "String property did not survive import");
	const auto& vector = customVariables.VectorVariables.at("VectorProbe");
	const auto& transform = customVariables.TransformVariables.at("TransformProbe");
	Check(vector.x == 1 && vector.y == 2 && vector.z == 3, "Vector property values changed during import");
	Check(transform.Translation.x == 10 && transform.Translation.y == 20 && transform.Translation.z == 30 && transform.Scale.x == 1 &&
	          transform.Scale.y == 2 && transform.Scale.z == 3 && std::abs(transform.Rotation.z - 0.70710678f) < 0.0001f &&
	          std::abs(transform.Rotation.w - 0.70710678f) < 0.0001f,
	      "Transform property values changed during import");

	CustomComponentRegistry registry;
	RegisterCustomComponents(registry);
	registry.Register<SetupOrderComponent>("SetupOrderComponent");
	SceneData sceneData;
	ActorRecord actorData;
	actorData.Name = "CustomActor";
	actorData.Components.push_back(*spinComponentData);
	CustomComponentData setupData;
	setupData.Common.Name = "SetupProbe";
	setupData.Common.Enabled = false;
	setupData.Common.Tags = {"SetupTag"};
	setupData.CustomType = "SetupOrderComponent";
	setupData.Variables.BoolVariables.emplace("Ready", true);
	actorData.Components.push_back(setupData);
	SceneComponentData laterComponentData;
	laterComponentData.Common.Name = "LaterComponent";
	actorData.Components.push_back(laterComponentData);
	sceneData.Actors.push_back(actorData);
	AssetRegistry assets;
	SceneWorldBuilder worldBuilder;
	auto world = worldBuilder.BuildWorld(sceneData, assets, {}, {1280, 720}, {},
	                                               [&registry](Actor& anActor, const CustomComponentData& aCustomComponentData)
	{
		CustomComponent* component = registry.Create(anActor, aCustomComponentData);
		if (auto* setupProbe = dynamic_cast<SetupOrderComponent*>(component))
		{
			Check(setupProbe->VariableApplications == 0, "Registry must only create and attach components");
		}
		return component;
	});
	Actor* actor = world->FindActor("CustomActor");
	Check(actor && actor->GetComponent<SpinComponent>(), "Registry did not construct SpinComponent");
	SetupOrderComponent* setupProbe = actor->GetComponent<SetupOrderComponent>();
	Check(setupProbe && setupProbe->VariableApplications == 1 && !setupProbe->HasBegunPlay(), "Builder must finish custom setup before BeginPlay");
	world->BeginPlay();
	world->Update(0.2f);
	Check(std::abs(actor->GetTransform().GetLocalRotationDegrees().x - spinSpeed * 0.2f) < 0.0001f,
	      "Imported SpinSpeed did not drive rotation");
	try
	{
		registry.Register<SpinComponent>("SpinComponent");
		Check(false, "Duplicate registration was accepted");
	}
	catch (const std::invalid_argument&)
	{
	}
	try
	{
		customVariables.GetOrDefault<float>("BoolProbe", 0.0f);
		Check(false, "Wrong custom property type was accepted");
	}
	catch (const std::invalid_argument&)
	{
	}
	try
	{
		customVariables.GetOrDefault<float>("IntProbe", 0.0f);
		Check(false, "Integer property was accepted as a float");
	}
	catch (const std::invalid_argument&)
	{
	}
	Check(customVariables.GetOrDefault<float>("MissingProbe", 12.0f) == 12.0f, "Missing property did not return its default");
	CustomComponentData unknownComponentData = *spinComponentData;
	unknownComponentData.CustomType = "MissingComponent_C";
	Check(registry.Create(*actor, unknownComponentData) == nullptr, "Unknown custom component was created");
	std::cout << "PASS: custom property import and SpinComponent creation\n";
	return 0;
}

int main(int argc, char** argv)
try
{
	if (argc == 3)
	{
		UnrealSceneImporter importer;
		const auto importResult = importer.ImportScene(argv[1]);
		Check(!importResult.Data && !importResult.Diagnostics.empty(), "Malformed custom data was accepted");
		Check(importResult.Diagnostics.front().Message.find(argv[2]) != std::string::npos,
		      "Custom diagnostic did not name the invalid field");
		std::cout << "PASS: rejected malformed field " << argv[2] << '\n';
		return 0;
	}
	return RunCustomComponentTests();
}
catch (const std::exception& error)
{
	std::cerr << error.what() << '\n';
	return 1;
}
