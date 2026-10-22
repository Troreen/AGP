#include "GameFramework/AssetHandling/AssetRegistry.h"
#include "GameFramework/AssetHandling/MaterialAsset.h"
#include "GameFramework/AssetHandling/MeshAsset.h"
#include "GameFramework/AssetHandling/TextureAsset.h"
#include "GameFramework/Scenes/SceneWorldBuilder.h"
#include "GameFramework/UnrealSceneImporter/UnrealSceneImporter.h"
#include "GameFramework/World/World.h"
#include "GraphicsEngine/GraphicsEngine.h"
#include "GraphicsEngine/Objects/Vertex.h"
#include <Windows.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
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

	template<class T>
	T ReadShaderBytes(const MaterialInterface& aMaterial, size_t anOffset)
	{
		T value;
		std::memcpy(&value, aMaterial.GetParameterDataBlock() + anOffset, sizeof(T));
		return value;
	}

	void TestImportedBindings()
	{
		AssetRegistry assets;
		auto masterNoise = std::make_shared<Texture>();
		auto instanceNoise = std::make_shared<Texture>();
		auto dunes = std::make_shared<Texture>();
		auto rough = std::make_shared<Texture>();
		auto cliff = std::make_shared<Texture>();
		auto instanceRough = std::make_shared<Texture>();
		auto normal = std::make_shared<Texture>();
		auto legacy = std::make_shared<Texture>();
		assets.Pin("Noise_Map", std::make_shared<TextureAsset>(masterNoise));
		assets.Pin("Instance_BaseColor", std::make_shared<TextureAsset>(instanceNoise));
		assets.Pin("Dunes_BaseColor", std::make_shared<TextureAsset>(dunes));
		assets.Pin("Rough_BaseColor", std::make_shared<TextureAsset>(rough));
		assets.Pin("Cliff_BaseColor", std::make_shared<TextureAsset>(cliff));
		assets.Pin("Override_Normal", std::make_shared<TextureAsset>(instanceRough));
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
			{"Mask Coverage", CommonUtilities::Vector4f{10, 20, 30, 40}},
			{"Projection.Weight", 0.5f},
			{"Projection.Offset", CommonUtilities::Vector4f{5, 6, 7, 8}},
			{"Noise Map", std::string("Noise_Map")},
			{"Dunes BaseColor", std::string("Dunes_BaseColor")},
			{"Rough_BaseColor", std::string("Rough_BaseColor")},
			{"Cliff BaseColor", std::string("Cliff_BaseColor")},
			{"Unmatched Normal Parameter", std::string("Fallback_Normal")},
			{"Old Export Name", std::string("LegacyNoise")}
		};
		MaterialInstanceData instance;
		instance.Name = "ImportedInstance";
		instance.Parent = master.Name;
		instance.Parameters = {
			{"MB_Glow Amount", 0.75f},
			{"Tint Color", CommonUtilities::Vector4f{4, 3, 2, 1}},
			{"Patch Size", 1600.0f},
			{"Rough BaseColor", std::string("Override_Normal")},
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
		Check(instanceMaterial.GetTexture("Noise_Map") == instanceNoise,
			"Explicit texture role did not override the asset suffix");
		Check(masterMaterial.GetTexture("Dunes_BaseColor") == dunes && masterMaterial.GetTexture("Rough_BaseColor") == rough &&
			masterMaterial.GetTexture("Cliff_BaseColor") == cliff,
			"Surface layers with identical asset suffixes did not bind independently");
		Check(instanceMaterial.GetTexture("Dunes_BaseColor") == dunes && instanceMaterial.GetTexture("Rough_BaseColor") == instanceRough &&
			instanceMaterial.GetTexture("Cliff_BaseColor") == cliff,
			"Instance did not override one surface layer while inheriting the other layers");
		Check(instanceMaterial.GetTexture(1) == normal,
			"Standard normal suffix fallback was lost or incorrectly overwritten");
		Check(instanceMaterial.GetTexture("LegacyNoise") == legacy,
			"Legacy asset-name texture fallback was lost");

		// These byte locations come from the fixture's HLSL register layout, not the engine's reflected metadata.
		Check(masterMaterial.GetParameterByName("MB_Tint_Color")->Offset == 16,
			"A float4 following a scalar must start at the next HLSL register");
		Check(ReadShaderBytes<CommonUtilities::Vector4f>(masterMaterial, 16).w == 4,
			"Imported vector was not stored at the shader's byte offset");
		Check(ReadShaderBytes<CommonUtilities::Vector4f>(instanceMaterial, 16).w == 1,
			"Instance vector override was not stored at the shader's byte offset");
		Check(masterMaterial.GetParameterByName("__Padding") == nullptr,
			"Explicit padding must not become an editable material parameter");
		Check(ReadShaderBytes<float>(masterMaterial, 36) == 800.0f,
			"Scalar default after skipped padding was not stored at the shader's byte offset");
		Check(ReadShaderBytes<float>(instanceMaterial, 36) == 1600.0f,
			"Scalar override after skipped padding was not stored at the shader's byte offset");
		Check(ReadShaderBytes<CommonUtilities::Vector4f>(masterMaterial, 48).w == 40,
			"Imported vector after padding was not stored at the shader's byte offset");
		Check(ReadShaderBytes<CommonUtilities::Vector4f>(instanceMaterial, 48).w == 40,
			"Instance did not inherit the vector at the shader's byte offset");
		Check(ReadShaderBytes<float>(masterMaterial, 64) == 0.5f &&
			ReadShaderBytes<CommonUtilities::Vector4f>(masterMaterial, 80).w == 8,
			"Nested material fields did not preserve their compiled member offsets");
	}

	void TestMaterialBufferLimit()
	{
		struct TemporaryShader
		{
			std::filesystem::path Path = std::filesystem::temp_directory_path() /
				("AGP_MaterialBufferLimit_" + std::to_string(GetCurrentProcessId()) + ".hlsli");

			~TemporaryShader()
			{
				std::error_code error;
				std::filesystem::remove(Path, error);
			}
		} shader;

		// Hidden padding keeps this rejection test safe even when the bounds guard is absent.
		std::ofstream source(shader.Path);
		source << "#include \"MaterialParameters.hlsli\"\n"
			"cbuffer MaterialBuffer : register(b4) { float4 __Oversized : packoffset(c8); };\n"
			"void Material_Vertex(inout MaterialVertexParameters p) {}\n"
			"void Material_Pixel(inout MaterialPixelParameters p) { p.PixelColor = __Oversized; }\n";
		source.close();
		Check(source.good(), "Cannot write temporary material buffer limit fixture");

		MaterialDescription description;
		description.Name = "OversizedMaterialBuffer";
		description.Domain = MaterialDomain::Surface;
		description.ShadingModel = ShadingModel::Lit;
		description.BlendMode = BlendMode::Opaque;
		description.MaterialShaderCode = shader.Path;
		Material material;
		Check(!GraphicsEngine::Get().CreateMaterial(description, material),
			"A material buffer larger than 128 bytes must be rejected, including hidden padding");
	}

	void CheckPreparedTerrainParameters(const std::vector<MaterialParameterData>& someParameters)
	{
		const std::array<std::pair<std::string_view, std::string_view>, 11> expectedTextures = {{
			{"DunesBaseColor", "T_8M_Snow_Dunes_BaseColor"},
			{"DunesORM", "T_8M_Snow_Dunes_OcclusionRoughnessMetallic"},
			{"DunesNormal", "T_8M_Snow_Dunes_Normal"},
			{"RoughBaseColor", "T_8M_Snow_Rough_BaseColor"},
			{"RoughORM", "T_8M_Snow_Rough_OcclusionRoughnessMetallic"},
			{"RoughNormal", "T_8M_Snow_Rough_Normal"},
			{"CliffBaseColor", "T_8M_Snow_Cliff_BaseColor"},
			{"CliffORM", "T_8M_Snow_Cliff_OcclusionRoughnessMetallic"},
			{"CliffNormal", "T_8M_Snow_Cliff_Normal"},
			{"SplatMask", "T_Splatmap_LevelWeights"},
			{"BlendNoise", "T_Splatmap_TransitionNoise"}
		}};
		const auto textureCount = std::count_if(someParameters.begin(), someParameters.end(), [](const MaterialParameterData& parameter)
		{
			return std::holds_alternative<std::string>(parameter.Value);
		});
		Check(static_cast<size_t>(textureCount) == expectedTextures.size(), "Prepared terrain must retain exactly eleven texture parameters");
		for (const auto& [role, asset] : expectedTextures)
		{
			const auto parameter = std::find_if(someParameters.begin(), someParameters.end(), [role](const MaterialParameterData& value)
			{
				return value.Name == role;
			});
			Check(parameter != someParameters.end(), "Prepared terrain texture role was lost during import");
			const auto* texture = std::get_if<std::string>(&parameter->Value);
			Check(texture && *texture == asset, "Prepared terrain texture role no longer identifies the expected asset");
		}

		const auto noise = std::find_if(someParameters.begin(), someParameters.end(), [](const MaterialParameterData& parameter)
		{
			return parameter.Name == "NoiseControls";
		});
		Check(noise != someParameters.end(), "Prepared terrain noise controls were lost during import");
		const auto* controls = std::get_if<CommonUtilities::Vector4f>(&noise->Value);
		Check(controls && controls->x == 0.0f && controls->y == 200.0f,
			"Prepared terrain must retain floating-point noise strength zero and patch size 200");
		const auto debug = std::find_if(someParameters.begin(), someParameters.end(), [](const MaterialParameterData& parameter)
		{
			return parameter.Name == "DEBUG";
		});
		Check(debug != someParameters.end() && std::get_if<float>(&debug->Value) && std::get<float>(debug->Value) == 0.0f,
			"Prepared terrain debug control must remain a floating-point scalar");
	}

	void TestPreparedTerrainScene(const std::filesystem::path& aScenePath)
	{
		const UnrealImportResult imported = UnrealSceneImporter().ImportScene(aScenePath);
		for (const ImportDiagnostic& diagnostic : imported.Diagnostics)
		{
			std::cerr << diagnostic.Context << ": " << diagnostic.Message << '\n';
		}
		Check(static_cast<bool>(imported), "Prepared terrain scene failed the real Unreal scene importer");
		const SceneData& scene = *imported.Data;
		Check(scene.Materials.size() == 1 && scene.Materials.front().Name == "MM_Landscape",
			"Prepared scene must retain the landscape master material");
		CheckPreparedTerrainParameters(scene.Materials.front().Parameters);

		std::array<bool, 3> foundMeshes{};
		const std::array<std::string_view, 3> expectedActors = {
			"SM_Landscape_Splatmap", "GroundFloater_Aligned", "GroundFloater_Rotated45"};
		bool foundCamera = false;
		for (const ActorRecord& actor : scene.Actors)
		{
			for (const ComponentRecord& component : actor.Components)
			{
				if (const auto* mesh = std::get_if<StaticMeshData>(&component))
				{
					const auto expected = std::find(expectedActors.begin(), expectedActors.end(), actor.Name);
					Check(expected != expectedActors.end(), "Prepared scene contains an unexpected mesh actor");
					foundMeshes[static_cast<size_t>(expected - expectedActors.begin())] = true;
					Check(mesh->Materials.size() == 1 && mesh->Materials.front().Parent == "MM_Landscape" &&
						mesh->Materials.front().Name == "MI_Landscape_Splatmap_Runtime",
						"Terrain and floaters must retain their landscape material instance");
					CheckPreparedTerrainParameters(mesh->Materials.front().Parameters);
				}
				if (const auto* camera = std::get_if<CameraData>(&component))
				{
					Check(actor.Name == "TerrainVerificationCamera" && camera->FieldOfView == 90.0f,
						"Prepared scene must retain its verification camera and FOV");
					Check(actor.Transform.Position.x == -3500.0f && actor.Transform.Position.y == 3200.0f &&
						actor.Transform.Position.z == 0.0f, "Verification camera position did not convert from Unreal XY/Z to AGP ZX/Y");
					foundCamera = true;
				}
			}
		}
		Check(foundMeshes[0] && foundMeshes[1] && foundMeshes[2] && foundCamera,
			"Prepared scene lost its terrain, two ground floaters or camera during import");

		// Compile the prepared material through the real engine, including its existing storage and texture limits.
		MaterialDescription description;
		description.Name = "MM_Landscape";
		description.Domain = MaterialDomain::Surface;
		description.ShadingModel = ShadingModel::Lit;
		description.BlendMode = BlendMode::Opaque;
		description.MaterialShaderCode = aScenePath.parent_path().parent_path().parent_path() / "Shaders/MM_Landscape.hlsli";
		Material material;
		Check(GraphicsEngine::Get().CreateMaterial(description, material), "Prepared landscape material did not compile within engine limits");
		const MaterialParameterInfo* debug = material.GetParameterByName("MB_DEBUG");
		Check(debug && debug->Offset == 64 && material.GetTextureSlotByName("BlendNoise") == 14,
			"Prepared landscape material must retain its final scalar and noise texture slots");
		std::cout << "Prepared terrain scene import and material compilation passed\n";
	}
}

int main(int argc, char* argv[])
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
		TestMaterialBufferLimit();
		Check(argc <= 2, "Usage: MaterialImportTests.exe [prepared-terrain-scene.json]");
		if (argc == 2)
		{
			TestPreparedTerrainScene(argv[1]);
		}
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
