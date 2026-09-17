#include "GameScene.h"
#include "GameOrientation.h"
#include "GameLog.h"
#include "GraphicsEngine/GraphicsEngine.h"
#include "GraphicsEngine/Materials/Material.h"
#include "GraphicsEngine/Objects/Mesh.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace
{
	using Vector3f = CommonUtilities::Vector3f;

	void Aim(LocalPose& pose, Vector3f target)
	{
		const auto forward = (target - pose.Position).GetNormalized();
		pose.Rotation =
		    GameOrientation::CreateUprightRotation(std::atan2(forward.x, forward.z), -std::asin(std::clamp(forward.y, -1.f, 1.f)));
	}

	ComponentRecord ComponentData(const char* name, const char* type, PropertyMap fields = {})
	{
		ComponentRecord c;
		c.Name = name;
		c.Type = type;
		c.Properties = std::move(fields);
		return c;
	}

	ActorRecord ActorData(const char* name, Vector3f position = {})
	{
		ActorRecord a;
		a.Name = name;
		a.Pose.Position = position;
		return a;
	}
}

SceneData GameScene::Load(const std::string& name, SceneLoadContext& context)
{
	if (name != "Game")
	{
		throw std::runtime_error("Unknown Game scene: " + name);
	}
	if (!myInitialized)
	{
		myContentRoot = context.ContentRoot;
		myMeshLibrary.Initialize(myContentRoot);
		myInitialized = true;
	}
	// The sample is an owned C++ source. Existing resource loading remains confined
	// to this integration unit and executes only at the host's safe loading point.
	SceneData scene;
	const auto materialRoot = myContentRoot / "Shaders";
	const Vector3f focus{25, 0, 260};
	auto camera = ActorData("Camera Actor", {0, 260, -950});
	Aim(camera.Pose, focus);
	camera.Components.push_back(ComponentData("Camera", "agp.Camera"));
	camera.Components.push_back(ComponentData("Camera Controls", "CameraControls"));
	scene.Actors.push_back(std::move(camera));
	scene.ActiveCameraActor = "Camera Actor";

	auto directional = ActorData("Directional Light Actor", {-450, 650, -350});
	Aim(directional.Pose, focus);
	directional.Components.push_back(
	    ComponentData("Directional Light", "agp.DirectionalLight", {{"color", Vector3f{1, .96f, .9f}}, {"intensity", 5.0}}));
	scene.Actors.push_back(std::move(directional));
	auto point = ActorData("Warm Character Point Actor", {-90, 180, 150});
	point.Components.push_back(ComponentData("Warm Character Point Light", "agp.PointLight",
	                                         {{"color", Vector3f{1, .42f, .22f}}, {"intensity", 20.0}, {"radius", 760.0}}));
	scene.Actors.push_back(std::move(point));
	auto spot = ActorData("Spot Light Actor", {430, 430, -210});
	Aim(spot.Pose, focus);
	spot.Components.push_back(ComponentData(
	    "Spot Light", "agp.SpotLight",
	    {{"color", Vector3f{.55f, .7f, 1}}, {"intensity", 30.0}, {"radius", 1200.0}, {"innerCone", 18.0}, {"outerCone", 34.0}}));
	scene.Actors.push_back(std::move(spot));

	auto bindMesh = [this, &context](const char* meshName, const std::string& materialId, std::shared_ptr<MaterialInterface> material)
	{
		auto mesh = myMeshLibrary.GetMesh(meshName);
		if (!mesh || !material)
		{
			throw std::runtime_error("Missing sample mesh/material: " + std::string(meshName));
		}
		context.Assets.BindMesh(AssetId{meshName}, mesh);
		context.Assets.BindMaterial(AssetId{materialId}, std::move(material));
		return PropertyMap{{"mesh", AssetId{meshName}},
		                   {"materials", std::vector<AssetId>(mesh->GetNumMaterialSlots(), AssetId{materialId})}};
	};
	auto meshActor = [this, &bindMesh, &materialRoot](const char* id, const char* name, const char* meshName, const char* materialName,
	                                                  Vector3f position, Vector3f rotation, Vector3f scale)
	{
		auto actor = ActorData(id, position);
		Transform pose;
		pose.SetLocalRotationDegrees(rotation.x, rotation.y, rotation.z);
		actor.Pose.Rotation = pose.GetLocalRotation();
		actor.Pose.Scale = scale;
		actor.Components.push_back(
		    ComponentData(name, "agp.StaticMesh", bindMesh(meshName, materialName, GetMaterial(materialRoot / materialName))));
		return actor;
	};
	scene.Actors.push_back(
	    meshActor("Floor Actor", "Floor Mesh Component", "Floor", "FloorMaterial.mat", {0, 0, 260}, {0, -90, 0}, {1100, 1100, 1100}));
	auto chest =
	    meshActor("SM_Chest Actor", "SM_Chest Mesh Component", "SM_Chest", "ChestMaterial.mat", {135, 0, 285}, {0, 0, 0}, {1, 1, 1});
	chest.Components.push_back(ComponentData("Spin", "Spin"));
	scene.Actors.push_back(std::move(chest));
	auto alpha = meshActor("SM_Chest Alpha Actor", "SM_Chest Alpha Mesh Component", "SM_Chest", "ChestMaterial_Alpha.mat", {-200, 0, -100},
	                       {0, 0, 0}, {1, 1, 1});
	// A fresh instance prevents preparation/reload from changing old-scene material contents.
	auto alphaMaterial = MaterialInstance::Create("ChestMaterial_Alpha_Instance", GetMaterial(materialRoot / "ChestMaterial_Alpha.mat"));
	if (!alphaMaterial)
	{
		throw std::runtime_error("Could not create alpha chest material");
	}
	alphaMaterial->SetValue("MB_Tint", CommonUtilities::Vector4f(1, 1, 1, .35f));
	alpha.Components[0].Properties = bindMesh("SM_Chest", "AlphaInstance", alphaMaterial);
	scene.Actors.push_back(std::move(alpha));
	scene.Actors.push_back(meshActor("SM_Color_Checker Actor", "SM_Color_Checker Mesh Component", "SM_Color_Checker",
	                                 "ColorCheckerMaterial.mat", {-145, 40, 365}, {0, -90, 0}, {1, 1, 1}));

	auto character = ActorData("TGA Bro Actor", {0, 0, 250});
	character.Pose.Rotation =
	    CommonUtilities::Quaternion<float>::CreateFromYawPitchRoll(CommonUtilities::Maths::DegreesToRadians(180.f), 0, 0);
	character.Components.push_back(ComponentData("Animation Controls", "AnimationControls"));
	auto characterFields = bindMesh("SK_C_TGA_Bro", "CharacterMaterial.mat", GetMaterial(materialRoot / "CharacterMaterial.mat"));
	characterFields.emplace("partialRoot", std::string("RightShoulder"));
	characterFields.emplace("animation", std::string("Breathing"));
	character.Components.push_back(ComponentData("TGA Bro Mesh Component", "agp.SkeletalMesh", std::move(characterFields)));
	scene.Actors.push_back(std::move(character));
	auto controls = ActorData("Scene Controls");
	controls.Components.push_back(ComponentData("Light Controls", "LightControls",
	                                            {{"camera", std::string("Camera Actor")},
	                                             {"directional", std::string("Directional Light Actor")},
	                                             {"point", std::string("Warm Character Point Actor")},
	                                             {"spot", std::string("Spot Light Actor")}}));
	scene.Actors.push_back(std::move(controls));
	return scene;
}

std::shared_ptr<MaterialInterface> GameScene::GetMaterial(const std::filesystem::path& aMaterialFile)
{
	const std::string cacheKey = aMaterialFile.lexically_normal().string();
	if (const auto materialIt = myMaterialCache.find(cacheKey); materialIt != myMaterialCache.end())
	{
		return materialIt->second;
	}

	MaterialDescription description;
	if (!LoadMaterialDescription(aMaterialFile, description))
	{
		GAMELOG(Warning, "Could not load material description '{}'.", aMaterialFile.string());
		return nullptr;
	}

	std::shared_ptr<Material> material = std::make_shared<Material>();
	if (!GraphicsEngine::Get().CreateMaterial(description, *material))
	{
		GAMELOG(Warning, "Could not create material '{}'.", description.Name);
		return nullptr;
	}

	const auto materialResult = myMaterialCache.emplace(cacheKey, material);
	return materialResult.first->second;
}
