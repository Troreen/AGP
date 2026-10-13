#include "RegisterGameComponents.h"
#include "GameFramework/UnrealSceneImporter/UnrealSceneImporter.h"
#include "GameFramework/World/World.h"
#include "GameFramework/Scenes/WorldFromSceneData.h"
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
	const auto& customProperties = spinComponentData->Properties;
	const float spinSpeed = customProperties.Get<float>("SpinSpeed", 0.0f);
	Check(std::abs(spinSpeed - 25.00001f) < 0.0001f, "Float property did not survive import");
	Check(customProperties.Get<float>("IntProbe", 0.0f) == 7.0f && customProperties.Booleans.at("BoolProbe"),
	      "Integer and boolean properties did not survive import");
	Check(customProperties.Strings.at("TargetComponentName").empty(), "String property did not survive import");
	const auto& vector = customProperties.Vectors.at("VectorProbe");
	const auto& transform = customProperties.Transforms.at("TransformProbe");
	Check(vector.x == 1 && vector.y == 2 && vector.z == 3, "Vector property values changed during import");
	Check(transform.Translation.x == 10 && transform.Translation.y == 20 && transform.Translation.z == 30 && transform.Scale.x == 1 &&
	          transform.Scale.y == 2 && transform.Scale.z == 3 && std::abs(transform.Rotation.z - 0.70710678f) < 0.0001f &&
	          std::abs(transform.Rotation.w - 0.70710678f) < 0.0001f,
	      "Transform property values changed during import");

	GameComponentRegistry registry;
	RegisterGameComponents(registry);
	SceneData sceneData;
	ActorRecord actorData;
	actorData.Name = "CustomActor";
	actorData.Components.push_back(*spinComponentData);
	sceneData.Actors.push_back(actorData);
	AssetRegistry assets;
	WorldFromSceneConverter converter;
	auto world = converter.BuildWorldFromSceneData(sceneData, assets, {}, {1280, 720}, {},
	                                               [&registry](Actor& anActor, const CustomComponentData& aCustomComponentData)
	{
		return registry.Create(anActor, aCustomComponentData);
	});
	Actor* actor = world->FindActor("CustomActor");
	Check(actor && actor->GetComponent<SpinComponent>(), "Registry did not construct SpinComponent");
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
		customProperties.Get<float>("BoolProbe", 0.0f);
		Check(false, "Wrong custom property type was accepted");
	}
	catch (const std::invalid_argument&)
	{
	}
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
