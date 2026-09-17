#include "ModelViewerScene.h"
#include "ModelViewerComponents.h"
#include "Application.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Components/StaticMeshComponent.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "GraphicsEngine/GraphicsEngine.h"
#include "GraphicsEngine/Materials/Material.h"
#include "GraphicsEngine/Objects/Mesh.h"
#include <cmath>
#include <stdexcept>

namespace
{
    using Vector3f = CommonUtilities::Vector3f;
    void AssignMaterialToAllSlots(MeshComponentBase& component, const std::shared_ptr<MaterialInterface>& material)
    {
        if (!material || !component.HasMesh()) throw std::runtime_error("Missing mesh or material");
        for (unsigned i = 0; i < component.GetMesh()->GetNumMaterialSlots(); ++i) component.SetMaterial(i,material);
    }
    void Aim(CommonUtilities::Transform& transform, Vector3f target)
    {
        const auto forward = (target-transform.GetPosition()).GetNormalized();
        transform.SetYawPitchRollRadians(std::atan2(forward.x,forward.z),-std::asin(forward.y),0);
    }
    ComponentDescription Behavior(const char* name, const char* type)
    { ComponentDescription c; c.Name = name; c.Type = type; return c; }
}
void ModelViewerScene::Initialize(GameContext& context)
{
    myContentRoot = context.GetContentRoot();
    myMeshLibrary.Initialize(myContentRoot);
    // Engine and game types deliberately use the same extension point.
    myRegistry.Register<SceneComponent>("Scene");
    myRegistry.Register<CameraComponent>("Camera");
    myRegistry.Register<StaticMeshComponent>("StaticMesh");
    myRegistry.Register<SkeletalMeshComponent>("SkeletalMesh");
    myRegistry.Register<DirectionalLightComponent>("DirectionalLight");
    myRegistry.Register<PointLightComponent>("PointLight");
    myRegistry.Register<SpotLightComponent>("SpotLight");
    myRegistry.Register<CameraControlsComponent>("CameraControls");
    myRegistry.Register<AnimationControlsComponent>("AnimationControls");
    myRegistry.Register<SpinComponent>("Spin");
    myRegistry.Register<LightControlsComponent>("LightControls");
    myRegistry.Freeze();
    Reload(context);
}
void ModelViewerScene::Reload(GameContext& context)
{
    const auto resolution = context.GetClientSize();
    context.RequestScene([this,resolution](const GameInput* input) { return Build(input,resolution); });
}
SceneBuildResult ModelViewerScene::Build(const GameInput* input, CommonUtilities::Vector2u resolution)
{
    // These descriptions stand in for imported engine-space data. Resource resolution
    // stays in the game adapter; callbacks configure attached but unstarted components.
    SceneDescription scene;
    const auto materialRoot = myContentRoot / "Shaders";
    const Vector3f focus{25,0,260};
    ActorDescription camera; camera.Id = "Camera Actor";
    camera.LocalTransform.SetPosition({0,260,-950}); Aim(camera.LocalTransform,focus);
    camera.Components.push_back(ComponentDescription::Make<CameraComponent>("Camera","Camera",[resolution](auto& c) { c.SetPerspective(90,1,50000,resolution); }));
    camera.Components.push_back(Behavior("Camera Controls","CameraControls"));
    scene.Actors.push_back(std::move(camera));
    scene.CameraActor = "Camera Actor"; scene.CameraComponent = "Camera";

    ActorDescription directional; directional.Id = "Directional Light Actor";
    directional.LocalTransform.SetPosition({-450,650,-350}); Aim(directional.LocalTransform,focus);
    directional.Components.push_back(ComponentDescription::Make<DirectionalLightComponent>("Directional Light","DirectionalLight",[](auto& c) { c.SetColor({1,.96f,.9f}); c.SetIntensity(5); }));
    scene.Actors.push_back(std::move(directional));
    ActorDescription point; point.Id = "Warm Character Point Actor"; point.LocalTransform.SetPosition({-90,180,150});
    point.Components.push_back(ComponentDescription::Make<PointLightComponent>("Warm Character Point Light","PointLight",[](auto& c) { c.SetColor({1,.42f,.22f}); c.SetIntensity(20); c.SetRadius(760); }));
    scene.Actors.push_back(std::move(point));
    ActorDescription spot; spot.Id = "Spot Light Actor";
    spot.LocalTransform.SetPosition({430,430,-210}); Aim(spot.LocalTransform,focus);
    spot.Components.push_back(ComponentDescription::Make<SpotLightComponent>("Spot Light","SpotLight",[](auto& c) { c.SetColor({.55f,.7f,1}); c.SetIntensity(30); c.SetRadius(1200); c.SetConeAnglesDegrees(18,34); }));
    scene.Actors.push_back(std::move(spot));

    auto meshActor = [&](const char* id, const char* name, const char* meshName, const char* materialName, Vector3f position, Vector3f rotation, Vector3f scale)
    {
        ActorDescription actor; actor.Id = id;
        actor.LocalTransform.SetPosition(position); actor.LocalTransform.SetRotation(rotation); actor.LocalTransform.SetScale(scale);
        auto mesh = myMeshLibrary.GetMesh(meshName); auto material = GetMaterial(materialRoot/materialName);
        actor.Components.push_back(ComponentDescription::Make<StaticMeshComponent>(name,"StaticMesh",[mesh,material](auto& c)
        { c.SetMesh(mesh); AssignMaterialToAllSlots(c,material); }));
        return actor;
    };
    scene.Actors.push_back(meshActor("Floor Actor","Floor Mesh Component","Floor","FloorMaterial.mat",{0,0,260},{0,-90,0},{1100,1100,1100}));
    auto chest = meshActor("SM_Chest Actor","SM_Chest Mesh Component","SM_Chest","ChestMaterial.mat",{135,0,285},{0,0,0},{1,1,1});
    chest.Components.push_back(Behavior("Spin","Spin")); scene.Actors.push_back(std::move(chest));
    auto alpha = meshActor("SM_Chest Alpha Actor","SM_Chest Alpha Mesh Component","SM_Chest","ChestMaterial_Alpha.mat",{-200,0,-100},{0,0,0},{1,1,1});
    auto alphaMaterial = MaterialInstance::Create("ChestMaterial_Alpha_Instance",GetMaterial(materialRoot/"ChestMaterial_Alpha.mat"));
    if (!alphaMaterial) throw std::runtime_error("Could not create alpha chest material");
    alphaMaterial->SetValue("MB_Tint",CommonUtilities::Vector4f(1,1,1,.35f));
    auto alphaMesh = myMeshLibrary.GetMesh("SM_Chest");
    alpha.Components[0].Configure = [alphaMesh,alphaMaterial](Component& c)
    { auto& mesh = dynamic_cast<StaticMeshComponent&>(c); mesh.SetMesh(alphaMesh); AssignMaterialToAllSlots(mesh,alphaMaterial); };
    scene.Actors.push_back(std::move(alpha));
    scene.Actors.push_back(meshActor("SM_Color_Checker Actor","SM_Color_Checker Mesh Component","SM_Color_Checker","ColorCheckerMaterial.mat",{-145,40,365},{0,-90,0},{1,1,1}));

    ActorDescription character; character.Id = "TGA Bro Actor";
    character.LocalTransform.SetPosition({0,0,250}); character.LocalTransform.SetRotation(180,0,0);
    character.Components.push_back(Behavior("Animation Controls","AnimationControls"));
    auto characterMesh = myMeshLibrary.GetMesh("SK_C_TGA_Bro"); auto characterMaterial = GetMaterial(materialRoot/"CharacterMaterial.mat");
    character.Components.push_back(ComponentDescription::Make<SkeletalMeshComponent>("TGA Bro Mesh Component","SkeletalMesh",[characterMesh,characterMaterial](auto& c)
    {
        c.SetMesh(characterMesh); AssignMaterialToAllSlots(c,characterMaterial);
        c.ConfigurePartialLayerFromJointName("RightShoulder"); c.PlayAnimation("Breathing",true);
    }));
    scene.Actors.push_back(std::move(character));
    ActorDescription controls; controls.Id = "Scene Controls"; controls.Components.push_back(Behavior("Light Controls","LightControls"));
    scene.Actors.push_back(std::move(controls));
    return SceneBuilder::Build(scene,myRegistry,input);
}

std::shared_ptr<MaterialInterface> ModelViewerScene::GetMaterial(const std::filesystem::path& aMaterialFile)
{
	const std::string cacheKey = aMaterialFile.lexically_normal().string();
	if (const auto materialIt = myMaterialCache.find(cacheKey); materialIt != myMaterialCache.end())
	{
		return materialIt->second;
	}

	MaterialDescription description;
	if (!LoadMaterialDescription(aMaterialFile, description))
	{
		MVLOG(Warning, "Could not load material description '{}'.", aMaterialFile.string());
		return nullptr;
	}

	std::shared_ptr<Material> material = std::make_shared<Material>();
	if (!GraphicsEngine::Get().CreateMaterial(description, *material))
	{
		MVLOG(Warning, "Could not create material '{}'.", description.Name);
		return nullptr;
	}

	const auto materialResult = myMaterialCache.emplace(cacheKey, material);
	return materialResult.first->second;
}

