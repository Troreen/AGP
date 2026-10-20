#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AssetHandling/MaterialAsset.h"
#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GameFramework/AssetHandling/TextureAsset.h"
#include "GameFramework/Scenes/SceneWorldBuilder.h"
#include "GameFramework/World/World.h"
#include "GraphicsEngine/GraphicsEngine.h"
#include "GraphicsEngine/Objects/Vertex.h"
#include <Windows.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace
{
	void Check(bool aCondition, const char* aMessage)
	{
		if (!aCondition)
		{
			throw std::runtime_error(aMessage);
		}
	}

	template<class T>
	T ReadParameter(const MaterialInterface& aMaterial, const std::string& aName)
	{
		const MaterialParameterInfo* parameter = aMaterial.GetParameterByName(aName);
		Check(parameter != nullptr && parameter->Size == sizeof(T), "Expected reflected parameter is missing");
		T value;
		std::memcpy(&value, aMaterial.GetParameterDataBlock() + parameter->Offset, sizeof(T));
		return value;
	}

	void TestImportedBindings()
	{
		AssetRegistry assets;
		auto masterNoise = std::make_shared<Texture>();
		auto instanceNoise = std::make_shared<Texture>();
		auto normal = std::make_shared<Texture>();
		auto legacy = std::make_shared<Texture>();
		assets.Pin("Noise_Map", std::make_shared<TextureAsset>(masterNoise));
		assets.Pin("Instance_BaseColor", std::make_shared<TextureAsset>(instanceNoise));
		assets.Pin("Fallback_Normal", std::make_shared<TextureAsset>(normal));
		assets.Pin("LegacyNoise", std::make_shared<TextureAsset>(legacy));
		auto mesh = std::make_shared<Mesh>();
		mesh->Initialize("BindingMesh", {Mesh::Element{}}, {}, {});
		assets.Pin("BindingMesh", std::make_shared<MeshAsset>(mesh));

		MasterMaterialRecord master;
		master.Name = "MaterialImportFixture";
		master.Parameters = {
			{"Glow Amount", 0.25f},
			{"MB_Tint Color", CommonUtilities::Vector4f{1, 2, 3, 4}},
			{"Noise Map", std::string("Noise_Map")},
			{"Unmatched Normal Parameter", std::string("Fallback_Normal")},
			{"Old Export Name", std::string("LegacyNoise")}
		};
		MaterialInstanceData instance;
		instance.Name = "ImportedInstance";
		instance.Parent = master.Name;
		instance.Parameters = {
			{"MB_Glow Amount", 0.75f},
			{"Tint Color", CommonUtilities::Vector4f{4, 3, 2, 1}},
			{"Noise Map", std::string("Instance_BaseColor")}
		};
		StaticMeshData component;
		component.Common.Name = "Mesh";
		component.MeshName = "BindingMesh";
		component.Materials.push_back(instance);
		ActorRecord actor;
		actor.Name = "BindingActor";
		actor.Components.push_back(component);
		SceneData scene;
		scene.Materials.push_back(master);
		scene.Actors.push_back(actor);
		SceneWorldBuilder builder;
		Check(builder.Initialize("Bin/Settings/MaterialNameConversions.json"), "Cannot initialize name conversions");
		auto world = builder.BuildWorld(scene, assets, "Tests/GameFramework", {64, 64});
		auto importedMaster = assets.GetAsset<MaterialAsset>(master.Name);
		auto importedInstance = assets.GetAsset<MaterialAsset>(instance.Name);
		Check(importedMaster && importedInstance, "Imported material was not registered");
		const auto& masterMaterial = *importedMaster->GetMaterial();
		const auto& instanceMaterial = *importedInstance->GetMaterial();
		Check(ReadParameter<float>(masterMaterial, "MB_Glow_Amount") == 0.25f,
			"Master scalar parameter with spaces was not imported");
		Check(ReadParameter<CommonUtilities::Vector4f>(masterMaterial, "MB_Tint_Color").w == 4,
			"Master vector parameter with MB_ prefix and spaces was not imported");
		Check(ReadParameter<float>(instanceMaterial, "MB_Glow_Amount") == 0.75f,
			"Instance scalar parameter normalization differs from master");
		Check(ReadParameter<CommonUtilities::Vector4f>(instanceMaterial, "MB_Tint_Color").w == 1,
			"Instance vector parameter normalization differs from master");
		Check(masterMaterial.GetTexture("Noise_Map") == masterNoise,
			"Custom texture did not bind by asset name");
		Check(instanceMaterial.GetTexture(0) == instanceNoise,
			"Standard BaseColor suffix did not select the albedo slot");
		Check(instanceMaterial.GetTexture("Noise_Map") == masterNoise,
			"Exported parameter name unexpectedly overrode asset-name binding");
		Check(instanceMaterial.GetTexture(1) == normal,
			"Standard normal suffix fallback was lost or incorrectly overwritten");
		Check(instanceMaterial.GetTexture("LegacyNoise") == legacy,
			"Legacy asset-name texture fallback was lost");
	}
}

int main()
{
	// A real device exercises shader compilation/reflection and scene construction.
	HWND window = CreateWindowExW(0, L"STATIC", L"Material import tests", WS_OVERLAPPEDWINDOW,
		0, 0, 64, 64, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
	try
	{
		Check(window != nullptr, "Cannot create hidden graphics window");
		Check(GraphicsEngine::Get().Initialize(window, "Source/Engine/GraphicsEngine/Shaders"),
			"Cannot initialize graphics device (requires a DirectX 11 adapter and debug layer)");
		TestImportedBindings();
		DestroyWindow(window);
		std::cout << "Material import integration tests passed\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		if (window)
		{
			DestroyWindow(window);
		}
		std::cerr << error.what() << '\n';
		return 1;
	}
}
