#include "GameFramework/UnrealSceneImporter/UnrealSceneImporter.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
	void Check(bool aCondition, const char* aMessage)
	{
		if (!aCondition)
		{
			throw std::runtime_error(aMessage);
		}
	}

	class FixtureDirectory
	{
	public:
		FixtureDirectory()
			: myPath(std::filesystem::temp_directory_path() /
				("AGP_NativeMaterialImportTests_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
		{
			Check(std::filesystem::create_directory(myPath), "Cannot create material import fixture directory");
		}

		~FixtureDirectory()
		{
			std::error_code error;
			std::filesystem::remove_all(myPath, error);
		}

		std::filesystem::path Write(const std::string& aName, const std::string& someMasters,
			const std::string& aParent, const std::string& someParameters, const std::string& aMaterialName = "MeshInstance") const
		{
			const std::filesystem::path path = myPath / (aName + ".json");
			std::ofstream output(path);
			output << R"({"MasterMaterials":[)" << someMasters << R"(],"Actors":[{
"Name":"FixtureActor","Archetype":"StaticMeshActor","Tags":[],
"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],
"Components":[{"Name":"Mesh","TypeID":1,"Parent":"","Tags":[],
"Mesh":"FixtureMesh","ContentPath":"Content/Meshes/FixtureMesh.fbx",
"Materials":[{"Name":")" << aMaterialName << R"(","Parent":")" << aParent
				<< R"(","Parameters":[)" << someParameters << R"(]}]}]}]})";
			output.close();
			Check(static_cast<bool>(output), "Cannot write material import fixture");
			return path;
		}

		std::filesystem::path WriteDocument(const std::string& aName, const std::string& aDocument) const
		{
			const std::filesystem::path path = myPath / (aName + ".json");
			std::ofstream output(path);
			output << aDocument;
			output.close();
			Check(static_cast<bool>(output), "Cannot write scene import fixture");
			return path;
		}

	private:
		std::filesystem::path myPath;
	};

	const std::string Root = R"({"Name":"Root","Domain":0,"ShadingModel":1,"BlendMode":0,
"BaseColor":[0.1,0.2,0.3,1],"Metallic":0.4,"Roughness":0.23,"TwoSided":true,
"Parameters":[{"Name":"MasterOnly","Type":0,"Value":1}]})";
	const std::string Mid = R"({"Name":"Mid","Parent":"Root","Parameters":[
{"Name":"Shared","Type":0,"Value":0.2},{"Name":"ParentOnly","Type":0,"Value":2}]})";
	const std::string Child = R"({"Name":"Child","Parent":"Mid","Parameters":[
{"Name":"Shared","Type":0,"Value":0.5},{"Name":"ChildOnly","Type":0,"Value":3}]})";
	const std::string Override = R"({"Name":"Shared","Type":0,"Value":0.9})";

	SceneData Import(const std::filesystem::path& aPath)
	{
		UnrealImportResult result = UnrealSceneImporter().ImportScene(aPath);
		Check(static_cast<bool>(result), "Material fixture import failed");
		return std::move(*result.Data);
	}

	const MaterialInstanceData& FirstMaterial(const SceneData& aScene)
	{
		Check(!aScene.Actors.empty() && !aScene.Actors.front().Components.empty(), "Fixture mesh was not imported");
		const StaticMeshData* mesh = std::get_if<StaticMeshData>(&aScene.Actors.front().Components.front());
		Check(mesh != nullptr && !mesh->Materials.empty(), "Fixture material was not imported");
		return mesh->Materials.front();
	}

	float FinalScalar(const std::vector<MaterialParameterData>& someParameters, const std::string& aName)
	{
		// The builder applies assignments in sequence; later instance overrides win.
		for (auto parameter = someParameters.rbegin(); parameter != someParameters.rend(); ++parameter)
		{
			if (parameter->Name == aName)
			{
				return std::get<float>(parameter->Value);
			}
		}
		throw std::runtime_error("Expected scalar parameter is missing: " + aName);
	}

	void CheckRootDefaults(const SceneData& aScene)
	{
		Check(aScene.Materials.size() == 1, "Child templates must not become shader-owning masters");
		const MasterMaterialRecord& root = aScene.Materials.front();
		Check(root.Name == "Root" && root.Domain == 0 && root.ShadingModel == 1 && root.BlendMode == 0,
			"Root material rendering settings changed");
		Check(root.BaseColor.x == 0.1f && root.BaseColor.w == 1 && root.Metallic == 0.4f && root.Roughness == 0.23f && root.TwoSided,
			"Root material defaults changed");
		Check(FinalScalar(root.Parameters, "MasterOnly") == 1, "Root parameter defaults changed");
	}

	void CheckResolvedInstance(const SceneData& aScene, float anExpectedSharedValue)
	{
		CheckRootDefaults(aScene);
		const MaterialInstanceData& instance = FirstMaterial(aScene);
		Check(instance.Parent == "Root", "Explicit child parent did not resolve to its root");
		Check(FinalScalar(instance.Parameters, "Shared") == anExpectedSharedValue, "Override precedence changed");
		Check(FinalScalar(instance.Parameters, "ParentOnly") == 2, "Inherited parent parameter was lost");
		Check(FinalScalar(instance.Parameters, "ChildOnly") == 3, "Child parameter was lost");
	}

	void TestNativeChildBeforeMaster(const FixtureDirectory& someFixtures)
	{
		const SceneData scene = Import(someFixtures.Write("native-child-first", Child + "," + Mid + "," + Root, "Child", ""));
		CheckResolvedInstance(scene, 0.5f);
	}

	void TestRootFirstOrder(const FixtureDirectory& someFixtures)
	{
		const SceneData scene = Import(someFixtures.Write("root-first", Root + "," + Mid + "," + Child, "Child", ""));
		CheckResolvedInstance(scene, 0.5f);
	}

	void TestComponentOverrides(const FixtureDirectory& someFixtures)
	{
		const SceneData scene = Import(someFixtures.Write("component-overrides", Child + "," + Mid + "," + Root, "Child", Override));
		CheckResolvedInstance(scene, 0.9f);
	}

	void TestSameNamePreservesExplicitParent(const FixtureDirectory& someFixtures)
	{
		std::string phong = Root;
		phong.replace(phong.find("Root"), 4, "Phong");
		const SceneData scene = Import(someFixtures.Write("explicit-parent", Child + "," + Mid + "," + Root + "," + phong,
			"Phong", Override, "Child"));
		const MaterialInstanceData& instance = FirstMaterial(scene);
		Check(instance.Name == "Child" && instance.Parent == "Phong", "Equal instance names changed an explicit component parent");
		Check(instance.Parameters.size() == 1 && FinalScalar(instance.Parameters, "Shared") == 0.9f,
			"Unrelated top-level template parameters leaked into the component instance");
	}

	void ExpectInheritanceError(const std::filesystem::path& aPath, const std::string& anExpectedMessage)
	{
		const UnrealImportResult result = UnrealSceneImporter().ImportScene(aPath);
		Check(!result && !result.Data && result.Diagnostics.size() == 1, "Invalid material inheritance was accepted");
		Check(!result.Diagnostics.front().Context.empty(), "Inheritance diagnostic lost its context");
		Check(result.Diagnostics.front().Message.find(anExpectedMessage) != std::string::npos,
			"Inheritance error did not identify the problem");
	}

	void TestMissingParent(const FixtureDirectory& someFixtures)
	{
		const std::string child = R"({"Name":"Child","Parent":"Missing","Parameters":[]})";
		ExpectInheritanceError(someFixtures.Write("missing-parent", child + "," + Root, "Child", ""), "missing parent 'Missing'");
	}

	void TestCycle(const FixtureDirectory& someFixtures)
	{
		const std::string mid = R"({"Name":"Mid","Parent":"Child","Parameters":[]})";
		ExpectInheritanceError(someFixtures.Write("cycle", Child + "," + mid + "," + Root, "Child", ""), "inheritance cycle");
	}

	void TestDuplicateMaterialNames(const FixtureDirectory& someFixtures)
	{
		ExpectInheritanceError(someFixtures.Write("duplicates", Root + "," + Root, "Root", ""), "Duplicate or empty material name");
	}

	void TestMissingComponentParent(const FixtureDirectory& someFixtures)
	{
		const auto result = UnrealSceneImporter().ImportScene(someFixtures.Write("missing-component-parent", Root, "Missing", ""));
		Check(!result && result.Diagnostics.size() == 1, "Missing component material parent was accepted");
		Check(result.Diagnostics.front().Context.find("FixtureActor/Mesh") != std::string::npos,
			"Missing component material diagnostic lost actor/component context");
	}

	void TestSelfTemplateReference(const FixtureDirectory& someFixtures)
	{
		const SceneData scene = Import(someFixtures.Write("self-template", Root + "," + Mid + "," + Child, "Self", "", "Child"));
		CheckResolvedInstance(scene, 0.5f);
	}

	void TestTextureAndVectorInheritance(const FixtureDirectory& someFixtures)
	{
		const std::string child = R"({"Name":"TextureChild","Parent":"Root","Parameters":[
{"Name":"SplatMask","Type":3,"Value":{"Name":"Atlas","AssetImportPath":"Content/ExportedScenes/Level/Atlas.dds"}},
{"Name":"MaskU","Type":1,"Value":[0.1,0,0,0.5]}]})";
		const SceneData scene = Import(someFixtures.Write("texture-inheritance", Root + "," + child, "TextureChild", ""));
		const auto& parameters = FirstMaterial(scene).Parameters;
		Check(parameters.size() == 2 && parameters[0].Name == "SplatMask" && std::get<std::string>(parameters[0].Value) == "Atlas",
			"Inherited named DDS texture role was lost");
		const auto mapping = std::get<CommonUtilities::Vector4f>(parameters[1].Value);
		Check(parameters[1].Name == "MaskU" && mapping.x == 0.1f && mapping.w == 0.5f,
			"Inherited mask mapping was lost");
	}

	void TestLandscapeMetadataPreservesScene(const FixtureDirectory& someFixtures)
	{
		const std::string document = R"({"MasterMaterials":[],"Actors":[{
"Name":"LandscapeWithBehavior","Archetype":"Landscape","Tags":["Preserved"],
"Components":[
{"Name":"LandscapeComponent","TypeID":12,"Tags":[],"Weightmaps":[]},
{"Name":"Root","TypeID":0,"Tags":[]},
{"Name":"Camera","TypeID":11,"Tags":[],"Parent":"Root","FOV":73},
{"Name":"Light","TypeID":5,"Tags":[],"Intensity":6,"Color":[0.1,0.2,0.3,1],
"Transform":[1,0,0,0,0,1,0,0,0,0,1,0,100,200,300,1]},
{"Name":"Behavior","TypeID":-1,"Tags":[],"CustomType":"FixtureBehavior","Custom":{}}
]}]})";
		const SceneData scene = Import(someFixtures.WriteDocument("landscape-preservation", document));
		Check(scene.Actors.size() == 1 && scene.Actors[0].Name == "LandscapeWithBehavior" &&
			scene.Actors[0].Tags == std::vector<std::string>{"Preserved"}, "Landscape metadata changed its owning actor");
		const auto& actor = scene.Actors[0];
		Check(actor.Transform(1, 1) == 1 && actor.Transform(4, 4) == 1 && actor.Components.size() == 4,
			"Omitted actor transform did not default to identity or Landscape became runtime geometry");
		Check(std::holds_alternative<SceneComponentData>(actor.Components[0]), "Scene root was lost");
		const auto& camera = std::get<CameraData>(actor.Components[1]);
		Check(camera.FieldOfView == 73 && camera.Common.SourceParent == "Root" && camera.Common.Transform(1, 1) == 1,
			"Authored camera or omitted component transform changed");
		const auto& light = std::get<DirectionalLightData>(actor.Components[2]);
		Check(light.Intensity == 6 && light.Color.x == 0.1f && light.Common.Transform(4, 1) == 200 &&
			light.Common.Transform(4, 2) == 300 && light.Common.Transform(4, 3) == 100, "Authored lighting or axis conversion changed");
		Check(std::get<CustomComponentData>(actor.Components[3]).CustomType == "FixtureBehavior", "Gameplay behavior was lost");
	}

	void TestMalformedTransformDiagnostic(const FixtureDirectory& someFixtures)
	{
		const auto path = someFixtures.WriteDocument("malformed-transform", R"({"MasterMaterials":[],"Actors":[{
"Name":"BrokenActor","Archetype":"Actor","Tags":[],"Transform":[1,2,3],"Components":[]}]})");
		const auto result = UnrealSceneImporter().ImportScene(path);
		Check(!result && result.Diagnostics.size() == 1 && !result.Data, "Malformed transform escaped import diagnostics");
		Check(result.Diagnostics[0].Context.find("BrokenActor") != std::string::npos &&
			result.Diagnostics[0].Message.find("16 numbers") != std::string::npos, "Malformed transform lost actor context");
	}

	void TestMissingRootFieldDiagnostic(const FixtureDirectory& someFixtures)
	{
		std::string root = Root;
		root.erase(root.find("\"Domain\":0,"), std::string("\"Domain\":0,").size());
		const auto path = someFixtures.Write("missing-root-field", root, "Root", "");
		const auto result = UnrealSceneImporter().ImportScene(path);
		Check(!result && result.Diagnostics.size() == 1 && !result.Data, "Malformed master escaped import diagnostics");
		Check(result.Diagnostics[0].Context == path.string() && !result.Diagnostics[0].Message.empty(),
			"Malformed master lost source-file context");
	}
}

int main(int anArgumentCount, char** someArguments)
{
	try
	{
		FixtureDirectory fixtures;
		TestNativeChildBeforeMaster(fixtures);
		TestRootFirstOrder(fixtures);
		TestComponentOverrides(fixtures);
		TestSameNamePreservesExplicitParent(fixtures);
		TestMissingParent(fixtures);
		TestCycle(fixtures);
		TestDuplicateMaterialNames(fixtures);
		TestMissingComponentParent(fixtures);
		TestSelfTemplateReference(fixtures);
		TestTextureAndVectorInheritance(fixtures);
		TestLandscapeMetadataPreservesScene(fixtures);
		TestMalformedTransformDiagnostic(fixtures);
		TestMissingRootFieldDiagnostic(fixtures);
		if (anArgumentCount > 1)
		{
			const SceneData scene = Import(someArguments[1]);
			std::cout << "Actual export imported: " << scene.Materials.size() << " masters, " << scene.Actors.size() << " actors\n";
		}
		std::cout << "Native material import tests passed (13 cases)\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
